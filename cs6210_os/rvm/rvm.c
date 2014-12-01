#include"rvm.h"
#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include "seqsrchst.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#define DEBUG

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

#define MAX_SEGMENTS 100

/* redo log*/
static redo_t redo_log;

char redo_file_path[512];
/* keep track for undo record */
static int num_undo_records = 0;
static steque_t *stack_mods;

/* segements mapped queue*/
static steque_t *mapped;



/*utility wire redo log*/
void write_redolog_to_file(int trans_id);


/* Place a write lock on the file. */
int lock_file (int fd) {
struct flock    lock;
lock.l_type = F_WRLCK;
lock.l_start = 0;
lock.l_whence = SEEK_SET;
lock.l_len = 0;
return fcntl(fd,F_SETLKW,&lock);
}

/* Place a write unlock on the file. */
int unlock_file(int fd)  {
struct flock    lock;
lock.l_type = F_UNLCK;
lock.l_start = 0;
lock.l_whence = SEEK_SET;
lock.l_len = 0;
return fcntl(fd,F_SETLKW,&lock);
}

void print_transactions( trans_t trans)
{
  int i;
  int size;
  mod_t* pmod;
  GTPRT("============TRANS OUTPUT==========\n");
  for (i = 0; i < trans->numsegs; ++i){  
    GTPRT("=> At index = %d \n", i )
      GTPRT("=> segname: %s\n",trans->segments[i]->segname);
    GTPRT("=> size: %d\n", trans->segments[i]->size);
    GTPRT("=> segname: %s\n", (char*)trans->segments[i]->segbase);

    size = steque_size(stack_mods);
    GTPRT("--------Read mod_t ARRAYS size: %d-------\n", size);
    int idx = 0;
    while (steque_size(&trans->segments[i]->mods) > 0 ) {
      GTPRT("--------mod_t index: %d\n", i);
      pmod = (mod_t *) steque_pop(stack_mods);
      if (pmod != NULL) {
        GTPRT("-mod_t[%d]-offset: %d\n", idx,pmod->offset);
        GTPRT("-mod_t[%d] -size: %d\n", idx,pmod->size);
        GTPRT("-mod_t[%d]-undo: %s\n", idx,(char*)pmod->undo);
      }
      idx++;
    }
    GTPRT("-----------------------------\n");	
  }
}

int keycmpfunc(seqsrchst_key a, seqsrchst_key b) 
{
  if (a == b) {
    return 1;
  } 
  else {
    return 0;
   }
}

/*
  Initialize the library with the specified directory as backing store.
*/
rvm_t rvm_init(const char *directory){
  //GTDBG("*ENTER--<rvm_init> directory= %s\n", directory);
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
  /*++Todo: find the size to allocated instead fixed value*/
  redo_log->entries = malloc(sizeof(segentry_t) * MAX_SEGMENTS);

  // Need to build the redo file path
  strcpy(redo_file_path, new_rvm_t->prefix);
  strcat(redo_file_path, "/");
  strcat(redo_file_path, "rvm.log");

  // Now let's make sure the redo file exists
  int logfd;
  logfd = open(redo_file_path, O_RDONLY | O_CREAT, S_IRWXU);
  if (logfd == -1) {
    GTFATAL_ERROR("<rvm_init> *ERROR can't open redo file\n");
  }
  new_rvm_t->redofd = logfd;
  num_undo_records = 0;
  
  /* build the stack mods table*/
  stack_mods = malloc (sizeof(steque_t));
  if (stack_mods == NULL) {
    GTFATAL_ERROR("<rvm_init> *ERROR allocate memory for stack_mods\n");
  }   
  steque_init(stack_mods);

   /* build the stack segements mapped */
  mapped= malloc (sizeof(steque_t));
  if (mapped == NULL) {
    GTFATAL_ERROR("<rvm_init> *ERROR allocate memory for mapped \n");
  }   
  steque_init(mapped);

  // Close the file as we don't need it right now
  close(logfd);
  //GTDBG("--*END-<rvm_init>---\n\n");
  
  return new_rvm_t;
}
/*
  map a segment from disk into memory. 
  If the segment does not already exist, then create it and give it size size_to_create. 
  If the segment exists but is shorter than size_to_create, then extend it until it is long enough. 
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){
  GTDBG("*ENTER--<rvm_map> segname= %s, size_to_create: %d\n", segname, size_to_create);
  int size_diff = 0;
  int seg_file;
  int result;

  char seg_file_path[512];
  strcpy(seg_file_path, rvm->prefix);
  strcat(seg_file_path, "/");
  strcat(seg_file_path, segname);

  /*
    When a process maps to an existing segment, there may be some updates present in the Redofile, 
    pending to be applied to the Segment and hence we need to "replay" (aka "recover") 
    the updates from the Redofile to the appropriate Segment, so 
    that the Segment can see the latest changes and modify them if the client application wants to, 
    using the RVM API. Hence we need to call rvm_truncate_log() in rvm_map().
  */
  
  rvm_truncate_log(rvm);
  GTDBG("-<rvm_map> --OPEN seg_file_path: %s <---\n", seg_file_path);
  seg_file = open(seg_file_path, O_RDWR | O_CREAT, S_IRWXU);
  if (seg_file == -1) {
    GTFATAL_ERROR("<rvm_map>ERROR open file\n");
  }

  // Now we need to check file size and see if it is large enough
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

  // Now that we have a blank file, we need to create a segment struct to represent it
  segment_t seg;
  seg = malloc(sizeof(struct _segment_t));
  // Now the area of memory that is going to map to what is on disk
  seg->segbase = malloc(size_to_create);
  strcpy(seg->segname, segname);
  seg->size = size_to_create;
  //reads from the specified position 0
  result = pread(seg_file, seg->segbase, size_to_create, 0);
  GTDBG("----<rvm_map> -<*segname: %s >READ result : %d---\n",seg->segname,  result);  
  seqsrchst_put(&rvm->segst, seg->segbase, seg);
  
  GTDBG("----<rvm_map> -Add segname %s to QUEUE \n", segname);  
  steque_enqueue(mapped, seg);
  
  close(seg_file);
  GTDBG("*END-<rvm_map> -\n");
  return seg->segbase;
}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase){
  GTDBG("*-ENTER--<rvm_unmap>- rvm->prefix: %s, segbase: %s--\n", rvm->prefix, (char*)segbase);
  segment_t seg_to_unmap;
  /* Find it in the rvm list. */
  seg_to_unmap = (segment_t) seqsrchst_get(&rvm->segst, segbase);
  seqsrchst_delete(&rvm->segst, segbase);
  free(seg_to_unmap);
  GTDBG("*-END--<rvm_unmap>---\n");
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
 
}

/*
  begin a transaction that will modify the segments listed in segbases. 
  If any of the specified segments is already being modified by a transaction, then the call should fail and return (trans_t) -1. 
  Note that trant_t needs to be able to be typecasted to an integer type.
 */
 trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
   GTDBG("*ENTER-<rvm_begin_trans>--numsegs: %d rvm->prefix = %s --\n", numsegs, rvm->prefix);
   int i;
  trans_t new_transaction = malloc(sizeof(struct _trans_t));  
  new_transaction->rvm = rvm;
  new_transaction->numsegs = numsegs;
  /* hold arrays of segments*/
  new_transaction->segments = malloc(sizeof(segment_t) * numsegs); 
  segment_t *segs = new_transaction->segments;

  GTDBG("-<rvm_begin_trans>--seqsrchst_size=%d  --\n", seqsrchst_size(&rvm->segst));

    for (i = 0; i < numsegs; i++) {
    if (segbases[i] == NULL) continue;
      segs[i] =  (segment_t) seqsrchst_get(&rvm->segst, segbases[i]);
      if (segs[i]->cur_trans != NULL) {
        GTDBG("@<rvm_begin_trans> **Fail and return (trans_t) -1.@@\n\n");
        return (trans_t) -1;
      }
      segs[i]->cur_trans = new_transaction;
    }

  GTDBG("*END--<rvm_begin_trans>-.. \n\n");
  return new_transaction;
 }


/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans. 
  Your library needs to ensure that the old memory has been saved, in case an abort is executed. 
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
  GTDBG("*ENTER---<rvm_about_to_modify>-->[[offset: %d, size: %d ]]--*\n", 
             offset, size);
  segment_t p_seg = NULL;

  GTDBG("-rvm_about_to_modify>-numsegs: %d\n", tid->numsegs );

  if (tid->numsegs >= MAX_SEGMENTS) {
    GTFATAL_ERROR("<rvm_about_to_modify>ERROR: Modifying too many segments \n ");
  } 
  
  /* Create a new redo_log put it in. */
  p_seg = (segment_t) seqsrchst_get(&tid->rvm->segst, segbase);
  if (p_seg == NULL) {
    GTDBG("<rvm_about_to_modify> ##ERROR found modify trans not add yet!! \n" );
    return;
  }
  GTDBG("-<rvm_about_to_modify>- creating mod_t trans \n");
  
  //Creating mod_t for transaction
  mod_t* mods;
  mods = malloc(sizeof(mod_t));
  mods->offset = offset;
  mods->size = size;
  mods->undo = malloc(size);
  memcpy(mods->undo, segbase+offset, size);
  steque_enqueue(stack_mods, mods);
  num_undo_records ++;  

  GTDBG("*END- <rvm_about_to_modify> \n");

}

/*
commit all changes that have been made within the specified transaction. When the call returns, then enough information should have been saved to disk so that, even if the program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid){
  int i;
  GTDBG("*ENTER---<rvm_commit_trans>  \n");
  //reset the undo record count
  num_undo_records = 0; 
  
  // This loop iterates through each segment in a transaction and applies the changes
  for (i = 0; i < tid->numsegs; i++) {
    GTDBG("-<rvm_commit_trans> -->@ i=%d  parsing segment %s of transaction\n", i, tid->segments[i]->segname);
    int trans_id;
    trans_id = redo_log->numentries;
    strcpy(redo_log->entries[trans_id].segname, tid->segments[i]->segname);
    redo_log->entries[trans_id].segsize = tid->segments[i]->size;
    redo_log->entries[trans_id].numupdates = steque_size(stack_mods);
    redo_log->entries[trans_id].data = calloc(tid->segments[i]->size, 1);
    redo_log->entries[trans_id].offsets = malloc(sizeof(int) * redo_log->entries[trans_id].numupdates);
    redo_log->entries[trans_id].sizes = malloc(sizeof(int) * redo_log->entries[trans_id].numupdates);
    int idx = 0;
    redo_log->entries[trans_id].updatesize = 0;
    while (steque_size(stack_mods) > 0) {
      mod_t* temp_mod;
      temp_mod = (mod_t *) steque_pop(stack_mods);
      
#if 1 /*print mod_t data */      
      GTDBG("-@@mod offset =%d ---\n", temp_mod->offset );
      GTDBG("-@mod-size= %d ---\n",temp_mod->size );
      GTDBG("-@mod undo= %s ---\n", (char*)(temp_mod->undo));
      GTDBG("-@mod SEGBASE= %s ---\n", (char*)(tid->segments[i]->segbase+temp_mod->offset));
#endif

      redo_log->entries[trans_id].offsets[idx] = temp_mod->offset;
      redo_log->entries[trans_id].sizes[idx] = temp_mod->size;
      redo_log->entries[trans_id].updatesize += temp_mod->size;
      memcpy(redo_log->entries[trans_id].data+temp_mod->offset, tid->segments[i]->segbase+temp_mod->offset, temp_mod->size);
      GTDBG("-<rvm_commit_trans> @mod_idx =%d, wrote data=< %s> \n", idx, (char *) redo_log->entries[trans_id].data);
      idx++;
      free(temp_mod);
    }
    //Reset cur_trans field of _segment_t 
    tid->segments[i]->cur_trans = NULL;
    redo_log->numentries++;
    GTDBG("-<rvm_commit_trans> ##Done processing modifications of each segment in transaction\n");
    GTDBG("-<rvm_commit_trans>  Begin writing redo_file to disk\n");
    /* write redo_log data to disk*/
    write_redolog_to_file(trans_id);
  }
}


/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid){
  GTDBG("-*ENTER-<rvm_abort_trans> -\n");
  GTDBG("-*-<rvm_abort_trans> ->num_undo_records=%d -\n", num_undo_records);
  int i;
  for (i = 0; i < tid->numsegs; i++) {
    GTDBG("-*tid->size=%d -\n", tid->segments[i]->size);
    mod_t *temp;
    while (steque_size(stack_mods) > 0 ) {
      temp = (mod_t *) steque_pop(stack_mods);
#if 0 /*print mod_t data */      
      GTDBG("-@@mod offset =%d ---\n", temp->offset );
      GTDBG("-@mod-size= %d ---\n",temp->size );
      GTDBG("-@mod undo= %s ---\n", (char*)(temp->undo));
      GTDBG("-@BEFORE<<segbase = %s >>\n", (char*)(tid->segments[i]->segbase+temp->offset));
#endif      
      /* Undo all the memory contents of the regions. */
      memcpy (tid->segments[i]->segbase+temp->offset, temp->undo, temp->size);
      //GTDBG("-@^^^AFTER<<segbase = %s >>\n", (char*)(tid->segments[i]->segbase+temp->offset));
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
  
  GTDBG("--COMPLETED---<rvm_truncate_log> ---\n");
}

/*-------------------------------------------------------------
 * Function write_redolog_to_file()
 * Utility to write data from rvm_commit_trans() use to write each trans
 * into the redo_log to file
------------------------------------------------------------*/
void write_redolog_to_file(int trans_id) {
  int fd_redo;
  int offset;
  int result;
  int j;

  GTDBG("*ENTER- <write_redolog_to_file> id= %d\n", trans_id);
  
  fd_redo = open(redo_file_path, O_WRONLY);
  if (fd_redo == -1) {
    GTFATAL_ERROR("<write_redolog_to_file>**ERROR opening the redo file!\n");
  }
  /* Place a write lock on the file. */
  lock_file(fd_redo);

  GTDBG("-<write_redolog_to_file> Open redo file result= %d\n", fd_redo);
  offset = lseek(fd_redo, 0, SEEK_END);
  GTDBG("-<write_redolog_to_file>@@-->[Offset = %d ]\n", offset);
  result = pwrite(fd_redo, redo_log->entries[trans_id].segname, strlen(redo_log->entries[trans_id].segname), offset);
  offset += 128;
  if (result == -1) {
    perror("pwrite()");
    exit(1);
  }
  pwrite(fd_redo, &redo_log->entries[trans_id].segsize, sizeof(int), offset);
  offset += sizeof(int);
  /* write the total of data blocks*/
  pwrite(fd_redo, &redo_log->entries[trans_id].updatesize, sizeof(int), offset);
  offset += sizeof(int);
  /*number of updates*/
  pwrite(fd_redo, &redo_log->entries[trans_id].numupdates, sizeof(int), offset);
  offset += sizeof(int);
  for (j = 0; j < redo_log->entries[trans_id].numupdates; j++) {
    GTDBG("-<write_redolog_to_file> ==> entries[%d]-<numupdates=%d>, <size: %d>\n", 
     j, redo_log->entries[trans_id].numupdates, redo_log->entries[trans_id].sizes[j]);
    pwrite(fd_redo, &redo_log->entries[trans_id].sizes[j], sizeof(int), offset);
    offset += sizeof(int);
  }
  for (j = 0; j < redo_log->entries[trans_id].numupdates; j++) {
    pwrite(fd_redo, &redo_log->entries[trans_id].offsets[j], sizeof(int), offset);
    offset += sizeof(int);
  }
  GTDBG("-<write_redolog_to_file>Writing data of size %d\n", redo_log->entries[trans_id].segsize);
  pwrite(fd_redo, redo_log->entries[trans_id].data, redo_log->entries[trans_id].segsize, offset);
  offset += redo_log->entries[trans_id].segsize;
  GTDBG("-<write_redolog_to_file> @@==> [data = %s]\n", (char *) redo_log->entries[trans_id].data);
  /* release lock file*/
  unlock_file(fd_redo);
  close(fd_redo);

  GTDBG("*END- <write_redolog_to_file> id= %d\n", trans_id);

}




