/*
 * Implement an LRU replacement cache policy
 *
 * See the comments in gtcache.h for API documentation.
 *
 * The least recently used element from the cache should be evicted.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "gtcache.h"
#include "steque.h"
#include "hshtbl.h"
#include "indexminpq.h"

/* always print msg when used */
#define GTPRT(msg,...) fprintf(stderr, " " msg, ##__VA_ARGS__);
#define GTFATAL_ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
/* set dbg to 1 to enable tracing just before using the GTDBG macro */
#ifdef DEBUG
int dbg = 0; 
#define GTDBG(msg,...) if (dbg) fprintf(stderr, " " msg, ##__VA_ARGS__)
#define GTLOG(msg,...) fprintf(stderr, " " msg, ##__VA_ARGS__)
#else
#define GTDBG(msg, ...)
#define GTLOG(msg, ...)
#endif

/*
 * Include headers for data structures as you see fit
 */
typedef struct gtcache_data_entry {
  int id;
  char *key;
  struct timeval* timeofday;
  size_t data_size;
  void *data;    
}gtcache_data_entry_t;

/*Global data */
static hshtbl_t *hash_table;
static steque_t *stack_of_ids;
static indexminpq_t *index_priority_queue;
/* caches entries*/
static gtcache_data_entry_t **cache_entries;
static int max_entries = 0;
static size_t max_capacity = 0;
/* the sum of size of entries in the cache.  */
static int sum_caches_entries_used = 0;

/*-----------------------------------------
 * Function: evict_cache_action() 
 * To remove the least frequently use
 * item in cache
 */
void evict_cache_action(size_t size_needed) {
  int index = 0;
  gtcache_data_entry_t *entry= NULL;
  unsigned int data_length = 0;

  if (indexminpq_isempty(index_priority_queue)){
    GTDBG("-EVICT.-- empty-- \n");
    return;
  }
  
  /* Keep remove LFU item from the cache*/
  while ((sum_caches_entries_used + size_needed) > max_capacity){
      /* get index for the minimum value in PQ */
      index = indexminpq_minindex(index_priority_queue);
      entry = cache_entries[index];
      data_length = entry->data_size;
      GTDBG("\n-->EVICT [id= %d] - removing key:%s data_size:%d\n",
	    index, entry->key, data_length);              
      /* delete item from hastabe*/
      hshtbl_delete(hash_table,entry->key);

      /*delete LRU item from index priority queue*/
      indexminpq_delmin(index_priority_queue);
      
      /* now, adds an element to the "front" of the steque*/
      steque_push(stack_of_ids,index);
      /*update the size of current cache size*/
      sum_caches_entries_used -= data_length;
      GTDBG( "--EVICT==>current_mem:%d\n",sum_caches_entries_used);
    }
}

int keycmpfunc(indexminpq_key a, indexminpq_key b) 
{
  struct timeval time1 = *((struct timeval *) a);
  struct timeval time2 = *((struct timeval *) b);
  
  long time1_val = time1.tv_sec + time1.tv_usec;
  long time2_val = time2.tv_sec + time2.tv_usec;

  if (time1_val == time2_val) {
    return 0;
  } 
  else {
    if (time1_val < time2_val) {
      return -1;
    } 
    else {
      return 1;
    }
  }
}

int gtcache_init(size_t capacity, size_t min_entry_size, int num_levels){
  int i = 0;  
  int status = -1; /* status 0 = success, other: error*/
  
  max_entries = (int)capacity/min_entry_size;
  max_capacity = capacity;
  
  GTDBG( "\n<gtcache_init. with -capacity:%zu, entry_size: %zu, num_levels:%d\n",
	 capacity,min_entry_size, num_levels);
  GTDBG("\n**max_entries: <%d > \n", max_entries);
    
  /* hastable with id+key */
  hash_table = malloc(sizeof(hshtbl_t));
  if (hash_table == NULL) {
    GTFATAL_ERROR( "\n*<ERROR>can't allocate memmory for hash_table\n");
  }
  hshtbl_init(hash_table,max_entries);

  /*indexed priority queue with id + #of hits*/
  index_priority_queue = malloc(sizeof(indexminpq_t));
  if (index_priority_queue == NULL) {
    GTFATAL_ERROR( "\n*<ERROR>can't allocate memmory for index_priority_queue\n");
  }
  indexminpq_init(index_priority_queue,max_entries,keycmpfunc);

  /*build stack of ids*/
  stack_of_ids = malloc(sizeof(steque_t));
  if (stack_of_ids == NULL) {
    GTFATAL_ERROR( "\n*<ERROR>can't allocate memory for stack_of_ids\n");
  }
  steque_init(stack_of_ids);
  for (i = 0; i < max_entries ; i++ ) {
    /* Adds an element to the "back" of the steque */
    steque_enqueue(stack_of_ids, i);
  }
  /* cache entries from 0->N*/
  cache_entries = malloc(sizeof(gtcache_data_entry_t) * max_entries);
  if (cache_entries == NULL ) {
    GTFATAL_ERROR("\n*<ERROR> Failed to allocated memory for cache_entried\n");
  }
  sum_caches_entries_used = 0;
  
  status = 0; /* successful initialize */
  
  return status;
}

void* gtcache_get(char *key, size_t *val_size){
  gtcache_data_entry_t* entry= NULL;
  void* data = NULL;
  int data_length = 0;

  hshtbl_item item = hshtbl_get(hash_table,key);
  if (item != NULL ) {   /*found it from cache*/
    entry = (gtcache_data_entry_t *)item;
    gettimeofday(entry->timeofday, NULL);
    /*time update the index priority queue*/
    indexminpq_increasekey(index_priority_queue,entry->id,(indexminpq_key)entry->timeofday);
    GTDBG("\ngtcache_get. <id:%d>- key%s\n", entry->id, key );
    data_length = entry->data_size;
    data = malloc(data_length);
    if (data != NULL) {
      memcpy (data, (void *)entry->data, data_length);
    }
  }
  /* assign length of return data size*/
  if (val_size != NULL) {
    *val_size = data_length;
  }

  return data;
    
}
 
int gtcache_set(char *key, void *value, size_t val_size){

  gtcache_data_entry_t * entry = NULL;
  hshtbl_item item = NULL;
  int status = 0;
  int data_len = 0;
  int new_key = 0;
  if(val_size > max_capacity) {
    status = -1;
    return (status);
  }
	
  /*check the cache size*/
  if (sum_caches_entries_used + val_size > max_capacity) {
    GTDBG("\n ---Cache full-> LFU replacement");
    GTDBG("\n gtcache_set. REPLACE need: %zu, used:%d, max: %zu ", val_size, 
	  sum_caches_entries_used, max_capacity);
    evict_cache_action(val_size);
  }

  /* Read the hash table */
  item = hshtbl_get(hash_table,key);
  if (item != NULL) { /* It's a hit*/
    /*  works as adding it 1st time*/
    entry = (gtcache_data_entry_t *)item;		
    sum_caches_entries_used -= entry->data_size;
  }
  else {
    new_key = 1;
    /* assign data to store into cache*/
    entry = malloc(sizeof(gtcache_data_entry_t));
    data_len = strlen(key) + 1;
    entry->key = malloc(data_len);
    strncpy(entry->key,key, data_len);
		
    /*Get ID from the queue*/
    entry->id = (int)steque_pop(stack_of_ids);	
    entry->timeofday = malloc (sizeof (struct timeval)); 	  
  } 
	
  /* check null data*/
  if (value != NULL) {
    entry->data = malloc(val_size);
    memcpy(entry->data, value, val_size);
  }  
  entry->data_size = val_size;
  gettimeofday(entry->timeofday, NULL);
  cache_entries[entry->id] = entry;
  if (new_key) {
    /*add entry into the hashtable*/
    hshtbl_put(hash_table,entry->key,(void*)entry);
    /* add entry into indexed priority queue*/
    indexminpq_insert(index_priority_queue,entry->id,entry->timeofday);
  }
  else {
    indexminpq_changekey(index_priority_queue,entry->id,entry->timeofday);
  }

  sum_caches_entries_used += val_size;
  GTDBG("\n--> current cache size used: %d <--\n", sum_caches_entries_used);

  return (status);
}


/*
 * returns the sum of the sizes of the entries in the cache
 */
int gtcache_memused(){
  GTDBG("\ngtcache_memused.--<sum size used: %d >\n",
	sum_caches_entries_used);

  return (sum_caches_entries_used);
}

void gtcache_destroy(){
  GTDBG("\n++getcache_detroy()++\n");
  
  if (stack_of_ids ) {
    steque_destroy(stack_of_ids);
    free(stack_of_ids);
  }
  if (index_priority_queue) {
    indexminpq_destroy(index_priority_queue);
    free(index_priority_queue);
  }
  if (hash_table) {
    hshtbl_destroy(hash_table);
    free (hash_table);
  }
  if (cache_entries) {
    free(cache_entries);
  }
  /* reset all the data */
  sum_caches_entries_used = 0;
  max_entries = 0;
  max_capacity = 0;
  
  GTDBG("\n++getcache_detroy.--Completed\n");
}

