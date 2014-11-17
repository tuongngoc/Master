/*
 * Implement an LRUMIN replacement cache policy
 *
 * See the comments in gtcache.h for API documentation.
 *
 * The LRUMIN eviction should be equivalent to the 
 * following psuedocode.
 *
 * while more space needs to be cleared
 *   Let n be the smallest integer such that 2^n >= space needed
 *   If there is an entry of size >= 2^n,
 *      delete the least recently used entry of size >= 2^n
 *   Otherwise, let m be the smallest inetger such that there an entry of size >= 2^m
 *     delete the least recently used item of size >=2^m
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


/*
 * Include headers for data structures as you see fit
 */
typedef struct gtcache_data_entry {
		int id;
		char *key;
		int level;
		struct timeval* timeofday;
		size_t data_size;
		void *data; 	 
	}gtcache_data_entry_t;
	
	/*Global data */
	static hshtbl_t *hash_table;
	static steque_t *stack_of_ids;
	/* caches entries*/
	static gtcache_data_entry_t **cache_entries;
	static int max_entries = 0;
	static size_t max_capacity = 0;
	/* the sum of size of entries in the cache.  */
	static int sum_caches_entries_used = 0;
	static int num_bands = 0;
	static size_t num_min_entry = 0;
	/*keep track the level and lru time */
	static indexminpq_t **priority_queue;
	/* store the max size of caches each level*/
	static indexminpq_t *level_queue;
	static int*size_cache_entries;

static inline int my_pow (int base, int power) 
{
  int n = 1;
  int i;
  for (i=0; i < power; i++) {
    n *= base;
  }
  return n;
}	


int find_idx_priority_level( size_t size)
{
 return 0;
}


/*-----------------------------------------
 * Function: evict_cache_action() 
 * To remove the least frequently use
 * item in cache
 */
	void evict_cache_action(size_t size_needed) {
		int index = 0;
		gtcache_data_entry_t *entry= NULL;
		unsigned int data_length = 0;
		int level = find_idx_priority_level(size_needed);
		
		
		/* Keep remove LFU item from the cache*/
		while ((sum_caches_entries_used + size_needed) > max_capacity){
				/* get index for the minimum value in PQ */
				index = indexminpq_minindex(priority_queue[level]);
				entry = cache_entries[index];
				data_length = entry->data_size;
				GTDBG("\n-->EVICT [id= %d] - removing key:%s data_size:%d\n",
				index, entry->key, data_length);							
				/* delete item from hastabe*/
				hshtbl_delete(hash_table,entry->key);
	
				/*delete LRU item from index priority queue*/
				indexminpq_delmin(priority_queue[level]);
				
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
		int cache_size_level = 0;
		int total = 0;
		
		max_entries = (int)capacity/min_entry_size;
		max_capacity = capacity;
		num_bands = num_levels;
		num_min_entry = min_entry_size;
		
		GTDBG( "\n<gtcache_init. with -capacity:%zu, entry_size: %zu, num_levels:%d\n",
		 capacity,min_entry_size, num_levels);
		GTDBG("\n**max_entries: <%d > \n", max_entries);
			
		/* hastable with id+key */
		hash_table = malloc(sizeof(hshtbl_t));
		if (hash_table == NULL) {
			GTFATAL_ERROR( "\n*<ERROR>can't allocate memmory for hash_table\n");
		}
		hshtbl_init(hash_table,max_entries);
    
		level_queue = malloc (sizeof(indexminpq_t));
		indexminpq_init(level_queue,num_bands,keycmpfunc);
	
		/*indexed priority queue with id + #of hits*/
		size_cache_entries = (int *)malloc(num_levels *sizeof(int));
		priority_queue = malloc(sizeof(indexminpq_t)*num_levels);
		for (i = 0; i < num_levels  ; i++ ) {
		  priority_queue[i] = malloc(sizeof(indexminpq_t));
		  if (priority_queue == NULL) {
			  GTFATAL_ERROR( "\n*<ERROR>can't allocate memmory for index_priority_queue\n");
		  }

		 if (i == (num_levels - 1)) {
				cache_size_level = capacity - total;
			}
			else {
			  cache_size_level = my_pow(2,i+1) * min_entry_size;
				total += cache_size_level;
			}
			/*save the max_cache size of each level*/
			size_cache_entries[i]= cache_size_level;
			indexminpq_insert (level_queue,i,(indexminpq_key *)&size_cache_entries[i]);
			
			GTDBG("\ngtcache_init. [level:%d]-cache size:%d \n", i,cache_size_level );
			indexminpq_init(priority_queue[i],cache_size_level,keycmpfunc);
    }
	
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
    indexminpq_increasekey(priority_queue[entry->level],
                         entry->id,(indexminpq_key)entry->timeofday);
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
		int level = 0;
		if(val_size > max_capacity) {
			status = -1;
			return (status);
		}
		
		/*check the cache size*/
		if (sum_caches_entries_used + val_size > max_capacity) {
			GTDBG("\n ---Cache full-> replacement");
			GTDBG("\n gtcache_set. REPLACE need: %zu, used:%d, max: %zu ", val_size, 
			sum_caches_entries_used, max_capacity);
			evict_cache_action(val_size);
		}
	
		/* Read the hash table */
		item = hshtbl_get(hash_table,key);
		if (item != NULL) { /* It's a hit*/
			/*	works as adding it 1st time*/
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
		level = find_idx_priority_level(val_size);
		entry->level = level;
		gettimeofday(entry->timeofday, NULL);
		cache_entries[entry->id] = entry;
		if (new_key) {
			/*add entry into the hashtable*/
			hshtbl_put(hash_table,entry->key,(void*)entry);
			/* add entry into indexed priority queue*/
			indexminpq_insert(priority_queue[entry->level],entry->id,entry->timeofday);
		}
		else {
			indexminpq_changekey(priority_queue[entry->level],entry->id,entry->timeofday);
		}
	
		sum_caches_entries_used += val_size;
		GTDBG("\n--> current cache size used: %d <--\n", sum_caches_entries_used);
	
		return (status);
}



int gtcache_memused(){
  GTDBG("\ngtcache_memused.--<sum size used: %d >\n",
	sum_caches_entries_used);

  return (sum_caches_entries_used);
}

void gtcache_destroy(){
  GTDBG("\n++getcache_detroy()++\n");
	int i = 0;
  
  if (stack_of_ids ) {
    steque_destroy(stack_of_ids);
    free(stack_of_ids);
  }
  if (*priority_queue) {
		for (i = 0; i < num_bands ; i++) {
      indexminpq_destroy(priority_queue[i]);
      free(priority_queue[i]);
		}
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
	num_bands = 0;
  
  GTDBG("\n++getcache_detroy.--Completed\n");
}

#if 0 /* Remove test code*/
int main ()
{
	size_t min_entry_size = 1024;
	size_t max_capacity= 32768;
	int nlevels = 4;
	gtcache_init(max_capacity, min_entry_size, nlevels);

	return 0;	
}
#endif
