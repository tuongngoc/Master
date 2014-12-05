#include "rvm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

//#define DEBUG

/* always print msg when used */
#define GTPRT(msg,...) fprintf(stderr, " " msg, ##__VA_ARGS__);
#define GTFATAL_ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
/* set dbg to 1 to enable tracing just before using the GTDBG macro */
#ifdef DEBUG
int dbg = 1; 
#define GTDBG(msg,...) if (dbg) fprintf(stderr, " " msg, ##__VA_ARGS__)
#define GTLOG(msg,...) fprintf(stderr, " " msg, ##__VA_ARGS__)
#else
#define GTDBG(msg, ...)
#define GTLOG(msg, ...)
#endif

/* redo log*/
static redo_t redo_log;

char redo_file_path[512];
static int num_undo_records = 0;
struct flock lock;


void write_redolog_to_file(struct segentry_t* segentry);


int keycmpfunc(seqsrchst_key a, seqsrchst_key b) {
  return a == b;
}

/* Place a write lock on the file. */
int lock_file (int fd) { 
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  return fcntl(fd,F_SETLKW,&lock);
}

/* Place a write unlock on the file. */
int unlock_file(int fd)  {
  lock.l_type = F_UNLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  return fcntl(fd,F_SETLKW,&lock);
}


seqsrchst_value find_segment_data(seqsrchst_t* st, char *segnamep) {
  seqsrchst_node* node;

  for( node = st->first; node != NULL; node = node->next) {
    segment_t segment = (segment_t)node->value;
    GTDBG("\nsegment->segname:%s size:%d\n",segment->segname,segment->size);
    if(strcmp(segment->segname,segnamep) == 0)
      return node->value;
  }

  return NULL;
}

/*
  Initialize the library with the specified directory as backing store.
*/
rvm_t rvm_init(const char *directory){
  rvm_t new_rvm_t = NULL;
  new_rvm_t = (rvm_t) malloc(sizeof(struct _rvm_t));
  strcpy(new_rvm_t->prefix, directory);
  /* Create the directory. If the directory is already there,
     it will simply return. */
  mkdir(directory, 0755);
  seqsrchst_init(&new_rvm_t->segst, keycmpfunc);

  // Allocate the redo log
  redo_log = malloc(sizeof(struct _redo_t));
  redo_log->numentries = 0;
  /* Initialize the flock structure. */
  memset (&lock, 0, sizeof(lock));

  // Need to build the redo file path
  strcat(redo_file_path,"./");	
  strcpy(redo_file_path, new_rvm_t->prefix);
  strcat(redo_file_path, "/");
  strcat(redo_file_path, "redo.log");

  // Now let's make sure the redo file exists
  new_rvm_t->redofd = open(redo_file_path, O_RDONLY | O_CREAT, S_IRWXU);
  if (new_rvm_t->redofd == -1) {
    perror("Error: ");
    GTFATAL_ERROR("<rvm_init> *ERROR can't open redo file\n");
  }
  
  num_undo_records = 0;

  // Close the file as we don't need it right now
  close(new_rvm_t->redofd);
  
  return new_rvm_t;
}


/*
  map a segment from disk into memory. 
  If the segment does not already exist, then create it and give it size size_to_create. 
  If the segment exists but is shorter than size_to_create, then extend it until it is long enough. 
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){
  GTDBG("*ENTER--<rvm_map> segname= %s, size_to_create: %d\n", 
           segname, size_to_create);


  /* Note:
     When a process maps to an existing segment, there may be some updates 
     present in the Redofile, pending to be applied to the Segment and hence 
     we need to "replay" (aka "recover") the updates from the Redofile to 
     the appropriate Segment, so that the Segment can see the latest changes
     and modify them if the client application wants to, 
     using the RVM API. Hence we need to call rvm_truncate_log() in rvm_map().
  */
  rvm_truncate_log(rvm);
  segment_t segment;
  char segment_name[128];
  int size_diff = 0;

  strcpy(segment_name,segname);

  segment = (segment_t)find_segment_data(&rvm->segst,(char*)segment_name);
  /*check if segment exists */
  if (segment != NULL) {
    GTPRT("\n The same segment :<%s > not allow to map twice!! \n",segment->segname);
    return NULL;
  }

  /*create new segment file*/
  segment = malloc(sizeof(struct _segment_t));
  strcpy(segment->segname,segment_name);
  segment->segbase = malloc(size_to_create);
  segment->size = size_to_create;
  steque_init(&segment->mods);

  /*create new segment file*/
  char seg_file_path[512];
  memset(seg_file_path, 0x00 , sizeof(seg_file_path));
  strcat(seg_file_path,"./");
  strcpy(seg_file_path, rvm->prefix);
  strcat(seg_file_path, "/");
  strcat(seg_file_path, segname);
  strcat(seg_file_path,".seg");

  int fd = open(seg_file_path, O_RDWR);

  if (fd == -1){
    GTPRT("\n<map>Error opening segment file:%s\n",segname);			
  }
  else {
    /*need to check file size and see if it is large enough*/
    struct stat st;
    stat(seg_file_path, &st);
    GTDBG("-<rvm_map> -size_to_create: %d, st_size=%d <---\n", size_to_create, (int) st.st_size );
 
    size_diff = size_to_create - st.st_size;
    // If there is a difference, we gotta expand the file
    if (size_diff > 0) {
      GTDBG("--<rvm_map>truncate++ Need more space %d\n", size_diff);
      int trun_res;
      trun_res = truncate(seg_file_path, size_diff);
      if (trun_res == -1) {
        GTDBG("---<rvm_map> : @Error truncating file\n");
      }
    }

    int bytes_read = pread(fd, segment->segbase, size_to_create, 0);
    if (bytes_read != size_to_create) {
      GTPRT("\n <rvm_map> read segment byes_read diff than expected! \n");
    }
    GTDBG("\n<map>-reading segname-bytes_read= %d\n",bytes_read);
    
    close(fd);
  }
  seqsrchst_put(&rvm->segst,(seqsrchst_key)segment->segbase,(void*)segment);
  GTDBG("----<rvm_map> -<*segname: %s , size : %d---\n",segment->segname,  segment->size);

  return segment->segbase;
}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase){
  segment_t seg = (segment_t) seqsrchst_get(&rvm->segst, segbase);
  if (seg != NULL) {
    seqsrchst_delete(&rvm->segst, segbase);
  }
}

/*
  destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
*/
void rvm_destroy(rvm_t rvm, const char *segname){
  char file_to_destroy[512];
  strcpy(file_to_destroy, rvm->prefix);
  strcat(file_to_destroy, "/");
  strcat(file_to_destroy, segname);

  GTDBG("<rvm_destroy>-- file_to_destroy: %s --\n", file_to_destroy);
 
  /* If the file is there it will delete it, else it will simply return. */
  remove(file_to_destroy);
  seqsrchst_destroy(&rvm->segst);
}


/*
  begin a transaction that will modify the segments listed in segbases. 
  If any of the specified segments is already being modified by a transaction, 
  then the call should fail and return (trans_t) -1. 
  Note that trant_t needs to be able to be typecasted to an integer type.
*/
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){

  trans_t transaction = malloc(sizeof(struct _trans_t));
  transaction->rvm = rvm;
  transaction->numsegs = numsegs;

  /* hold arrays of segments*/
  transaction->segments = malloc(sizeof(struct _segment_t) * numsegs);

  int i;
  for (i = 0; i < numsegs; i++){
    void *segbase = (void*)segbases[i];
    if (segbase == NULL) continue;
    segment_t segment = seqsrchst_get(&rvm->segst,(seqsrchst_key)segbase);
    if (segment->cur_trans != NULL) {
      GTDBG("@<rvm_begin_trans> **Fail and return (trans_t) -1.@@\n\n");
      return (trans_t) -1;
    }
    segment->cur_trans = transaction;
    transaction->segments[i] = segment;
  }

  return transaction;
}


/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans. 
  Your library needs to ensure that the old memory has been saved, in case an abort is executed. 
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
  GTDBG("*ENTER-<rvm_about_to_modify>-->[[offset: %d, size: %d ]]--*\n", 
         offset, size);

  mod_t *mod = malloc(sizeof(struct mod_t));
  mod->offset = offset;
  mod->size = size;
  mod->undo = malloc(size);
  memcpy(mod->undo,segbase + offset,size);

  //segment_t segment = get_segment_from_trans(tid, segbase);
  segment_t segment;
  segment = (segment_t)seqsrchst_get(&tid->rvm->segst,(seqsrchst_key)segbase);

  if (segment->cur_trans != tid) {
    GTPRT("\n<ERROR*>tid mismatch in the rvm_about_to_modify\n");
    return;
  }

  //add to the mods steque
  steque_enqueue(&segment->mods,(steque_item)mod);
  num_undo_records ++;
  GTDBG("*END- <rvm_about_to_modify> \n");

}

/*
  commit all changes that have been made within the specified transaction. 
  When the call returns, then enough information should have been saved to disk so that, 
  even if the program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid){

  int i;
  char segment_name[128];	
  char seg_file_path[512];
  rvm_t rvm;
  segment_t segment;
  rvm = tid->rvm;

  GTDBG("*ENTER---<rvm_commit_trans>- numsegs = %d \n", tid->numsegs);

  //reset the undo record count
  num_undo_records = 0; 

  int seg_count = tid->numsegs;
  redo_log->numentries = seg_count;
  redo_log->entries = malloc(sizeof(struct segentry_t) * seg_count);

  GTDBG("\n**redo_log has:%d entries\n", redo_log->numentries );

  for (i = 0; i < tid->numsegs; i++) {
    segment = tid->segments[i];
    strcpy(segment_name,segment->segname);
    GTDBG("\n-->> WRITE segment %s >>\n", segment_name);

    memset(seg_file_path, 0x00 , sizeof(seg_file_path));
    strcat(seg_file_path,"./");
    strcat(seg_file_path,rvm->prefix);
    strcat(seg_file_path,"/");
    strcat(seg_file_path,segment_name);
    strcat(seg_file_path,".seg");
    GTDBG("\nseg_file_path is:%s\n", seg_file_path);
    int fd = open(seg_file_path, (O_RDWR | O_CREAT | O_SYNC), S_IRUSR | S_IWUSR | S_IXUSR);

    if (fd == -1){
      GTPRT("\nError opening segment map file in rvm_commit_trans:%s\n",segment_name);
    }
    else
    {

      GTDBG("\n->> New segentry:%s\n",segment_name);
      segentry_t segentry;
      strcpy(segentry.segname,segment_name);
      int updatesize = 0;
      int mod_offset;
      int mod_size;

      mod_t *mod;
      int mod_count = steque_size(&segment->mods);
      segentry.offsets = malloc(mod_count * sizeof(int));
      segentry.sizes = malloc(mod_count * sizeof(int));
      segentry.data = malloc(segment->size);
      segentry.segsize = segment->size;
      segentry.numupdates = 0;

      GTDBG("\nThere are %d mods in this segment\n",mod_count);		

      int j;
      for (j = 0; j < mod_count; j++) {
        if (!steque_isempty(&segment->mods)) {
          mod = (mod_t*)steque_pop(&segment->mods);
          segentry.numupdates++;
          mod_size = mod->size;
          mod_offset = mod->offset;
          segentry.sizes[j] = mod_size;
          segentry.offsets[j] = mod_offset;
          updatesize += mod_size;
          memcpy(segentry.data + mod_offset,mod->undo,mod_size);
        }
        else {
          GTDBG("<Error> More mods than expected!! \n");
          break;
        }
      }

      redo_log->entries[i] = segentry;

      int bytesWritten = write(fd,segment->segbase,segment->size);
      if (bytesWritten == -1)
      {
        GTPRT("\n<commit> Failed to write segment file \n");
        perror("Error: ");
      }
      close(fd);
      segment->cur_trans = NULL;
      write_redolog_to_file(&redo_log->entries[i]);
    }			
  }

}

/*
  undo all changes that have happened within the specified transaction.
*/
void rvm_abort_trans(trans_t tid){
  GTDBG("-*-<rvm_abort_trans> ->num_undo_records=%d -\n", num_undo_records);
  int i;
  mod_t *pmod;
  segment_t segment;

  for (i = 0; i < tid->numsegs; i++) {
    GTDBG("-*tid->size=%d -\n", tid->segments[i]->size);
    segment = tid->segments[i];
    while (steque_size(&segment->mods) > 0 ) {
      pmod = (mod_t*)steque_pop(&segment->mods);
      int size = pmod->size;
      int offset = pmod->offset;
      memcpy(segment->segbase + offset,pmod->undo,size);
      free(pmod);
    }
  }
  num_undo_records = 0;
  
  GTDBG("*END- rvm_abort_trans()--\n");

}

/*
  play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
*/
void rvm_truncate_log(rvm_t rvm){
  int i;
  int fd_redo;
  int offset;

  GTDBG("-*ENTER-<restore_seg_from_log>  \n");
  
  fd_redo = open(redo_file_path, O_RDONLY);
  if (fd_redo == -1) {
    GTFATAL_ERROR("<rvm_truncate_log>** Error open redo file..\n");
  }
  offset = lseek(fd_redo, 0, SEEK_CUR);
  //GTDBG("<rvm_truncate_log>- lseek return--<offset= %d>\n", offset);
  int more = 1;
  while (more == 1) {
    segentry_t *p_seg;
    p_seg = malloc(sizeof(segentry_t));
    int read_result;
    read_result = pread(fd_redo, &p_seg->segname, 128, offset);
    //GTDBG("<rvm_truncate_log> read truncate log <result= %d>\n", read_result);
    if (read_result == 0) {
      close(fd_redo);
      //GTDBG("<rvm_truncate_log> +++EOF in redo log ++++\n");
      remove(redo_file_path);
      fd_redo = open(redo_file_path, O_WRONLY | O_CREAT, S_IRWXU);
      close(fd_redo);
      more = 0;
      break;
    }
    offset += 128;
    pread(fd_redo, &p_seg->segsize, sizeof(int), offset);
    offset += sizeof(int);
    /*updatesize = total of data blocks updates*/
    pread(fd_redo, &p_seg->updatesize, sizeof(int), offset);
    offset += sizeof(int);
    pread(fd_redo, &p_seg->numupdates, sizeof(int), offset);
    offset += sizeof(int);
    p_seg->offsets = malloc(sizeof(int) * p_seg->numupdates);
    p_seg->sizes = malloc(sizeof(int) * p_seg->numupdates);
 
    for (i = 0; i < p_seg->numupdates; i++) {
      pread(fd_redo, &p_seg->sizes[i], sizeof(int), offset);
      offset += sizeof(int);
    }
    for (i = 0; i < p_seg->numupdates; i++) {
      pread(fd_redo, &p_seg->offsets[i], sizeof(int), offset);
      offset += sizeof(int);
    }
  
    p_seg->data = malloc(p_seg->segsize);
 
    pread(fd_redo, p_seg->data, p_seg->segsize, offset);
    offset += p_seg->segsize;
    
    char seg_file_path[512];
    strcpy(seg_file_path, rvm->prefix);
    strcat(seg_file_path, "/");
    strcat(seg_file_path, p_seg->segname);
    int fd_seg;
    GTDBG("<rvm_truncate_log> Open seg_filef %s\n", seg_file_path);
    fd_seg = open(seg_file_path, O_WRONLY);
    for (i = 0; i < p_seg->numupdates; i++) {
      pwrite(fd_seg, p_seg->data+p_seg->offsets[i], p_seg->sizes[i], p_seg->offsets[i]);
    }
    close(fd_seg);
  }
  
  //GTDBG("--COMPLETED---<rvm_truncate_log> ---\n");
}

/*-------------------------------------------------------------
 * Function write_redolog_to_file()
 * Utility to write data from rvm_commit_trans() use to write each trans
 * into the redo_log to file
 ------------------------------------------------------------*/
void write_redolog_to_file(struct segentry_t* segentry) {
  int fd_redo;
  int offset;
  int result;
  int j;
  
  fd_redo = open(redo_file_path, O_WRONLY);
  if (fd_redo == -1) {
    GTFATAL_ERROR("<write_redolog_to_file>**ERROR opening the redo file!\n");
  }
  /* Place a write lock on the file. */
  lock_file(fd_redo);

  GTDBG("-<write_redolog_to_file> Open redo file result= %d\n", fd_redo);
  offset = lseek(fd_redo, 0, SEEK_END);
  GTDBG("-<write_redolog_to_file>@@-->[Offset = %d ]\n", offset);
  result = pwrite(fd_redo, segentry->segname, strlen(segentry->segname), offset);
  offset += sizeof(segentry->segname);  
  if (result == -1) {
    perror("pwrite()");
    exit(1);
  }
  pwrite(fd_redo, &segentry->segsize, sizeof(int), offset);
  offset += sizeof(int);
  /* write the total of data blocks*/
  pwrite(fd_redo, &segentry->updatesize, sizeof(int), offset);
  offset += sizeof(int);
  /*number of updates*/
  pwrite(fd_redo, &segentry->numupdates, sizeof(int), offset);
  offset += sizeof(int);
  for (j = 0; j < segentry->numupdates; j++) {
    pwrite(fd_redo, &segentry->sizes[j], sizeof(int), offset);
    offset += sizeof(int);
  }
  for (j = 0; j < segentry->numupdates; j++) {
    pwrite(fd_redo, &segentry->offsets[j], sizeof(int), offset);
    offset += sizeof(int);
  }
  GTDBG("-<write_redolog_to_file>Writing data of size %d\n", segentry->segsize);
  pwrite(fd_redo, segentry->data, segentry->segsize, offset);
  offset += segentry->segsize;
  GTDBG("-<write_redolog_to_file> @@==> [data = %s]\n", (char *) segentry->data);
  /* release lock file*/
  unlock_file(fd_redo);
  close(fd_redo);


}
