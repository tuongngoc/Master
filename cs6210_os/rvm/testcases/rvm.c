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

#define DEBUG 1
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
struct flock lock; 


// Constants
int my_pid;
int num_transactions = 1000;
char redo_file_path[512];

int equals(seqsrchst_key a, seqsrchst_key b) {
	int x, y;
	x = *(int *) a;
	y = *(int *) b;
	if (x == y) {
		return 1;
	} else {
		return 0;
	}
}

rvm_t rvm_init(const char *directory){
	my_pid = getpid();
	GTDBG("*<rvm_init> %d: Beginning rvm_init\n", my_pid);
	//seqsrchst_init(&rvm->segst, equals);
	rvm_t rvm;
	rvm = malloc(sizeof(struct _rvm_t));
	strcpy(rvm->prefix, directory);
	mkdir(directory, 0777);
	// Allocate the redo log
	redo_log = malloc(sizeof(struct _redo_t));
	redo_log->numentries = 0;
	redo_log->entries = malloc(sizeof(segentry_t) * num_transactions);

	// Need to build the redo file path
	strcpy(redo_file_path, rvm->prefix);
	strcat(redo_file_path, "/");
	strcat(redo_file_path, "rvm.log");
	GTDBG("<rvm_init>- pid: %d, directory: %s\n", my_pid, directory);
	seqsrchst_init(&rvm->segst, equals);

	// Now let's make sure the redo file exists
	int r_fd;
	r_fd = open(redo_file_path, O_RDONLY | O_CREAT, S_IRWXU);
	if (r_fd == -1) {
		GTFATAL_ERROR("<rvm_init> *ERROR can't open redo file\n");
	}
	// Close the file as we don't need it right now
	close(r_fd);
	
	/* Initialize the flock structure. */
  memset (&lock, 0, sizeof(lock));

	GTDBG("<rvm_init> %d: completed rvm_init\n", my_pid);
	return rvm;

}

/*
  map a segment from disk into memory.
  If the segment does not already exist, then create it and give it size size_to_create. 
  If the segment exists but is shorter than size_to_create, then extend it until it is long enough. 
  It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create){
	GTDBG("<rvm_map> %d: size_to_create: %d\n", my_pid, size_to_create);
	rvm_truncate_log(rvm);
	// Need a place to hold the seg file path
	char seg_file_path[512];
	strcpy(seg_file_path, rvm->prefix);
	strcat(seg_file_path, "/");
	strcat(seg_file_path, segname);

	// Let's open that file!
	GTDBG("<rvm_map> id:%d - path=%s\n", my_pid, seg_file_path);
	int seg_file;
	int result;
	seg_file = open(seg_file_path, O_RDWR | O_CREAT, S_IRWXU);
	if (seg_file == -1) {
		GTFATAL_ERROR("<rvm_map>ERROR open file\n");
	}

	// Now we need to check file size and see if it is large enough
	struct stat st;
	stat(seg_file_path, &st);
	int size_diff;
	size_diff = size_to_create - st.st_size;
	// If there is a difference, we gotta expand the file
	if (size_diff > 0) {
		GTDBG("-<rvm_map> %d: Truncating segment file, needed %d more\n", my_pid, size_diff);
		int trun_res;
		trun_res = truncate(seg_file_path, size_diff);
		if (trun_res == -1) {
			GTFATAL_ERROR("-<rvm_map>*Error truncating file\n");
		}
	}

	// Now that we have a blank file, we need to create a segment struct to represent it
	segment_t seg;
	seg = malloc(sizeof(struct _segment_t));
	// Now the area of memory that is going to map to what is on disk
	seg->segbase = malloc(size_to_create);
	strcpy(seg->segname, segname);
	seg->size = size_to_create;
	GTDBG("<rvm_map> segname: %s, size: %d \n", segname,size_to_create);
	steque_init(&seg->mods);
	result = pread(seg_file, seg->segbase, size_to_create, 0);
	GTDBG("<rvm_map> pid:%d Result of fread segbase: %d \n", my_pid, result);
	close(seg_file);
	seqsrchst_put(&rvm->segst, seg->segbase, seg);
	GTDBG("<rvm_map> pid: %d: Completed rvm_map\n\n", my_pid);
	return seg->segbase;
}

/*
  unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase){
	GTDBG("<rvm_unmap> %d: Beginning unmap\n", my_pid);
	segment_t seg_to_unmap;
	seg_to_unmap = (segment_t) seqsrchst_get(&rvm->segst, segbase);
	seqsrchst_delete(&rvm->segst, segbase);
	free(seg_to_unmap);
	GTDBG("<rvm_unmap> pid:%d  - Done unmap\n\n", my_pid);
}

/*
  destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
 */
void rvm_destroy(rvm_t rvm, const char *segname){
	GTDBG("<rvm_destroy> pid %d: Beginning rvm_destroy\n", my_pid);
	char file_to_destroy[512];
	strcpy(file_to_destroy, rvm->prefix);
	strcat(file_to_destroy, "/");
	strcat(file_to_destroy, segname);
	remove(file_to_destroy);
	GTDBG("<rvm_destroy> pid:%d -detroy_file: %s- Done\n\n",
		    my_pid, file_to_destroy);
}

/*
  begin a transaction that will modify the segments listed in segbases. 
  If any of the specified segments is already being modified by a transaction, then the call should fail and return (trans_t) -1. 
  Note that trant_t needs to be able to be typecasted to an integer type.
 */
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
	GTDBG("<rvm_begin_trans>--->numsegs=%d  \n", numsegs);
	// Let's allocate a trans_t struct to hold our stuff
	trans_t transaction;
	transaction = malloc(sizeof(struct _trans_t));
	transaction->rvm = rvm;
	transaction->numsegs = numsegs;
	// We need to have an array of pointers to segments
	transaction->segments = malloc(sizeof( struct _segment_t) * numsegs);
	int i, j;
	// So this loop iterates over each segment we are passed in and assigns it to the transaction
	for (i = 0; i < numsegs; i++) {
		GTDBG("<rvm_begin_trans>--->seqsrchst_size=%d  \n", seqsrchst_size(&rvm->segst));
		for (j = 0; j < seqsrchst_size(&rvm->segst); j++) {
			transaction->segments[i] = (segment_t) seqsrchst_get(&rvm->segst, segbases[i]);
			if (transaction->segments[i]->cur_trans != NULL) {
				GTDBG("@@<rvm_begin_trans> **Fail and return (trans_t) -1.@@\n\n");
				return (trans_t) -1;
			}
			transaction->segments[i]->cur_trans = transaction;
			GTDBG("<rvm_begin_trans>@@numsegs %d: Assigned: <%s> current transaction\n", 
				numsegs, transaction->segments[i]->segname);
		}
	}
	GTDBG("@@<rvm_begin_trans> pid %d: Done with rvm_begin_trans@@\n\n", my_pid);
	return transaction;
}

/*
  declare that the library is about to modify a specified range of memory in the specified segment. 
  The segment must be one of the segments specified in the call to rvm_begin_trans. 
  Your library needs to ensure that the old memory has been saved, in case an abort is executed. 
  It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
	GTDBG("*---<rvm_about_to_modify> pid %d -->[[offset: %d, size: %d ]]--*\n", 
		my_pid, offset, size);
	
	segment_t tar_seg;
	tar_seg = (segment_t) seqsrchst_get(&tid->rvm->segst, segbase);
	GTDBG("<rvm_about_to_modify> pid: %d: Found tar_seg: <%s>\n", 
		 my_pid, tar_seg->segname);
	mod_t* mods;
	mods = malloc(sizeof(mod_t));
	mods->undo = malloc(size);
	mods->offset = offset;
	mods->size = size;
	GTDBG("<rvm_about_to_modify>-->mod_t-*AT- offset=[%d] and size=<%d>\n", offset, size);
	memcpy(mods->undo, segbase+offset, size);
	steque_enqueue(&tar_seg->mods, mods);
	GTDBG("@@<rvm_about_to_modify>Completed@@\n\n");
}

/*
commit all changes that have been made within the specified transaction. 
When the call returns, then enough information should have been saved 
to disk so that, even if the program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid){
	int i; 
	GTDBG("*---<rvm_commit_trans>-> trans_t *numsegs: %d Begin---**\n", tid->numsegs); 
	// This loop iterates through each segment in a transaction and applies the changes
	for (i = 0; i < tid->numsegs; i++) {
		GTDBG("<rvm_commit_trans>**Loop-trans_t[%d]-segment:<%s> of transaction\n", 
			i, tid->segments[i]->segname);
		int trans_id;
		trans_id = redo_log->numentries;
		strcpy(redo_log->entries[trans_id].segname, tid->segments[i]->segname);
		redo_log->entries[trans_id].segsize = tid->segments[i]->size;
		redo_log->entries[trans_id].numupdates = steque_size(&tid->segments[i]->mods);
		redo_log->entries[trans_id].data = calloc(tid->segments[i]->size, 1);
		redo_log->entries[trans_id].offsets = malloc(sizeof(int) * redo_log->entries[trans_id].numupdates);
		redo_log->entries[trans_id].sizes = malloc(sizeof(int) * redo_log->entries[trans_id].numupdates);
		int c = 0;
		redo_log->entries[trans_id].updatesize = 0;
		while (steque_size(&tid->segments[i]->mods) > 0) {
			GTDBG("<rvm_commit_trans>**Inside arrys mods[%d]**\n", steque_size(&tid->segments[i]->mods));
			mod_t* temp_mod;
			temp_mod = (mod_t *) steque_pop(&tid->segments[i]->mods);
			redo_log->entries[trans_id].offsets[c] = temp_mod->offset;
			redo_log->entries[trans_id].sizes[c] = temp_mod->size;
			redo_log->entries[trans_id].updatesize += temp_mod->size;
			memcpy(redo_log->entries[trans_id].data+temp_mod->offset, tid->segments[i]->segbase+temp_mod->offset, temp_mod->size);
			c++;
			GTDBG("<rvm_commit_trans>++data wrote to [memory:%s ] <-\n", (char *) redo_log->entries[trans_id].data);
			free(temp_mod);
		}
		GTDBG("<rvm_commit_trans>->Got [updatesize:%d] \n",redo_log->entries[trans_id].updatesize );
		redo_log->numentries++;
		GTDBG("<rvm_commit_trans> pid %d:--[entry:%d ]= %d :redo_numentries \n", 
			my_pid, i, redo_log->numentries);
		
		int redo_fh, offset;
		redo_fh = open(redo_file_path, O_WRONLY);
		if (redo_fh == -1) {
			GTFATAL_ERROR("<rvm_commit_trans>**ERROR! open redo file \n");
		}
		GTDBG("<rvm_commit_trans>**Locking <%s> redo_file_path \n", redo_file_path);
		
		/* Place a write lock on the file. */		
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		fcntl (redo_fh, F_SETLKW, &lock);

		GTDBG("<rvm_commit_trans>--file descriptor redo_fh: %d\n", redo_fh);
		offset = lseek(redo_fh, 0, SEEK_END);
		GTDBG("<rvm_commit_trans>-->Offset: %d \n", offset);
		int result;
		/* start write serialization of redo log*/
		result = pwrite(redo_fh, redo_log->entries[trans_id].segname, 
			          strlen(redo_log->entries[trans_id].segname), offset);
		offset += 128;
		GTDBG("<rvm_commit_trans>-Wrote size=%d, [offset:%d]\n", result, offset);
		if (result == -1) {
			GTDBG("<rvm_commit_trans>-- pid: %d: <**Errno was %d>\n", my_pid, result);
			perror("pwrite()");
			exit(1);
		}
		pwrite(redo_fh, &redo_log->entries[trans_id].segsize, sizeof(int), offset);
		offset += sizeof(int);
		GTDBG("<rvm_commit_trans>--Wrote <segsize: %d >,[offset:%d] \n", 
			redo_log->entries[trans_id].segsize,offset);
    /* wrote updatesize*/
		pwrite(redo_fh, &redo_log->entries[trans_id].updatesize, sizeof(int), offset);
		offset += sizeof(int);
		GTDBG("<rvm_commit_trans>- Wrote [updatesize: %d],[offset:%d] \n",
			    redo_log->entries[trans_id].updatesize, offset);
		
		
		pwrite(redo_fh, &redo_log->entries[trans_id].numupdates, sizeof(int), offset);
		offset += sizeof(int);
		GTDBG("<rvm_commit_trans>- Wrote ->[numupdates: %d ]- at [offset:%d] \n",
			    redo_log->entries[trans_id].numupdates, offset);
		int j;
		for (j = 0; j < redo_log->entries[trans_id].numupdates; j++) {
			GTDBG("<rvm_commit_trans> pid: %d: Processing size entry[ %d/%d] \n", 
				my_pid, j, redo_log->entries[trans_id].numupdates);
			
			GTDBG("<rvm_commit_trans>--[size_%d= %d ]\n", j, 
				redo_log->entries[trans_id].sizes[j]);
			pwrite(redo_fh, &redo_log->entries[trans_id].sizes[j], sizeof(int), offset);
			offset += sizeof(int);
		}
		GTDBG("<rvm_commit_trans> pid:%d: [offset: %d --Write size] \n", my_pid, offset);
		for (j = 0; j < redo_log->entries[trans_id].numupdates; j++) {
			GTDBG("<rvm_commit_trans>*segmententry=>[entry_offset_%d = %d] \n", 
				    j, redo_log->entries[trans_id].offsets[j]);
			
			pwrite(redo_fh, &redo_log->entries[trans_id].offsets[j], sizeof(int), offset);
			offset += sizeof(int);
		}
		GTDBG("<rvm_commit_trans>-segsize: %d  \n", 
			 redo_log->entries[trans_id].segsize);
		
		pwrite(redo_fh, redo_log->entries[trans_id].data, redo_log->entries[trans_id].segsize, offset);
		offset += redo_log->entries[trans_id].segsize;
		GTDBG("<rvm_commit_trans>**segment_t->index= %d Void ptr to [data shows %s ] \n", 
			  trans_id, (char *) redo_log->entries[trans_id].data);

		/* Release the lock. */
		lock.l_type = F_UNLCK;
		lock.l_whence = SEEK_SET;
		fcntl (redo_fh, F_SETLKW, &lock);
		GTDBG("<rvm_commit_trans> **UnLocking <%s> redo_file_path \n", redo_file_path);

		close(redo_fh);
	}
	GTDBG("@@---<rvm_commit_trans>--Completed---@@\n\n");
}

/*
  undo all changes that have happened within the specified transaction.
 */
void rvm_abort_trans(trans_t tid){
	GTDBG("<rvm_abort_trans> pid:%d: Begin....\n", my_pid);
	GTDBG("<rvm_abort_trans> pid:%d ->numsegs: %d ....\n", 
		my_pid, tid->numsegs);
	
  //Go through the segments to be used  
  int i;
  for (i = 0; i < tid->numsegs; i++){
		GTDBG("<rvm_abort_trans> pid: %d: Processing segment: <%s> of transaction\n", 
			my_pid, tid->segments[i]->segname);
		segment_t segment = (segment_t)seqsrchst_get (&tid->rvm->segst, tid->segments[i]->segbase);
		if (segment != NULL) {
			GTDBG("<rvm_abort_trans> pid: %d: FOUND segname: <%s>==> size: <%d>\n", my_pid,
				segment->segname, segment->size);
			steque_pop(&tid->segments[i]->mods);
			seqsrchst_delete(&tid->rvm->segst, tid->segments[i]->segbase);
			free(segment);
			GTDBG("<rvm_abort_trans> pid: %d: Complete remove-> size: %d \n ", my_pid,
				 seqsrchst_size(&tid->rvm->segst));
		}
		
  }/*for loop*/
  
}

/*
 play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
*/
void rvm_truncate_log(rvm_t rvm){
	//GTDBG("<rvm_truncate_log> pid:%d: Begin....\n", my_pid);
	int redo_fh, offset;
	redo_fh = open(redo_file_path, O_RDONLY);
	if (redo_fh == -1) {
		GTFATAL_ERROR("<rvm_truncate_log> Error opening redo file!\n");
	}	
		 
	offset = lseek(redo_fh, 0, SEEK_CUR);
	GTDBG("<rvm_truncate_log> pid: %d: Result of lseek = %d\n", my_pid, offset);
	int more = 1;
	GTDBG("<rvm_truncate_log> pid:%d: READ segments from log file\n", my_pid);
	
	while (more == 1) {
		segentry_t *s;
		s = malloc(sizeof(segentry_t));
		int read_result;
		read_result = pread(redo_fh, &s->segname, 128, offset);
		//GTDBG("<rvm_truncate_log> pid %d: truncate read_result= %d\n", my_pid, read_result);
		if (read_result == 0) {
			close(redo_fh);
			//GTDBG("<rvm_truncate_log> pid %d: EOF in redo log reached!\n", my_pid);
			remove(redo_file_path);
			redo_fh = open(redo_file_path, O_WRONLY | O_CREAT, S_IRWXU);
			close(redo_fh);
			more = 0;
			break;
		}
		offset += 128;
		pread(redo_fh, &s->segsize, sizeof(int), offset);
		offset += sizeof(int);
		
		pread(redo_fh, &s->updatesize, sizeof(int), offset);
		offset += sizeof(int);
		
		pread(redo_fh, &s->numupdates, sizeof(int), offset);
		offset += sizeof(int);
		//GTDBG("<rvm_truncate_log> pid %d --> segsize %d and [numupdates %d ] \n", 
		//	my_pid, s->segsize, s->numupdates);
		s->offsets = malloc(sizeof(int) * s->numupdates);
		s->sizes = malloc(sizeof(int) * s->numupdates);
		//GTDBG("<rvm_truncate_log> pid %d: Done allocating offsets and sizes\n", my_pid);
		int i;
		for (i = 0; i < s->numupdates; i++) {
			pread(redo_fh, &s->sizes[i], sizeof(int), offset);
			offset += sizeof(int);
		}
		for (i = 0; i < s->numupdates; i++) {
			pread(redo_fh, &s->offsets[i], sizeof(int), offset);
			offset += sizeof(int);
		}
		//GTDBG("<rvm_truncate_log> pid %d: Done reading in offsets and sizes\n", my_pid);
		s->data = malloc(s->segsize);
		//GTDBG("<rvm_truncate_log> pid %d: Done malloc'ing s->data\n", my_pid);
		
		pread(redo_fh, s->data, s->segsize, offset);
		offset += s->segsize;
		//GTDBG("<rvm_truncate_log> pid %d: Read in <data of %s > \n",
		//	my_pid, (char *) s->data);
		char seg_file_path[512];
		strcpy(seg_file_path, rvm->prefix);
		strcat(seg_file_path, "/");
		strcat(seg_file_path, s->segname);
		int seg_file;
		seg_file = open(seg_file_path, O_WRONLY);
		//GTDBG("<rvm_truncate_log> pid %d: **Locking <%s> seg_file \n", my_pid,seg_file_path);
		/* Place a write lock on the file. */ 	
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		fcntl (seg_file, F_SETLKW, &lock);

		for (i = 0; i < s->numupdates; i++) {
			pwrite(seg_file, s->data+s->offsets[i], s->sizes[i], s->offsets[i]);
		}
		
		/* Release the lock. */
		lock.l_type = F_UNLCK;
		lock.l_whence = SEEK_SET;
		fcntl (seg_file, F_SETLKW, &lock);
		GTDBG("<rvm_truncate_log> **UnLocking <%s> seg_file \n", seg_file_path);

		close(seg_file);
	}

	GTDBG("<rvm_truncate_log> pid %d: Done with log truncation\n", my_pid);
}
