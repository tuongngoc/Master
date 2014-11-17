#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <rpc/rpc.h>

/*
 * Sends a request to *url and returns an address to
 * an array containing the response.  Your implementation
 * should work when req is null.
 *
 * Consult the libcurl library
 *
 * http://curl.haxx.se/libcurl/
 *
 * and the example getinmemory.c in particular
 *
 * http://curl.haxx.se/libcurl/c/getinmemory.html
 */
struct DataMemoryStruct {
  char *data;
  size_t size;
  size_t len;
};

static size_t write_data_callback(void *buffer, size_t size,
                         size_t nmemb, void *userp) {
 
  size_t realsize = size * nmemb;
  struct DataMemoryStruct *wdi = (struct DataMemoryStruct *)userp;

  while(wdi->len + (size * nmemb) >= wdi->size) {
    /* check for realloc failing in real code. */
    wdi->data = realloc(wdi->data, wdi->size*2);
    wdi->size*=2;
  }
  if(wdi->data == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(wdi->data + wdi->len, buffer, size * nmemb);
  wdi->len += realsize;

  return (realsize);
}

char ** httpget_1_svc(char** url, struct svc_req* req) {  
  CURL *curl_handle;
  CURLcode res;
  struct DataMemoryStruct wdi;
  static char *result = NULL;
  
  memset(&wdi, 0, sizeof(wdi));
  curl_global_init(CURL_GLOBAL_ALL);
  /* init the curl session */
  curl_handle = curl_easy_init();

  if(NULL != curl_handle) {
    wdi.size = 1024;
    wdi.data = malloc(wdi.size);

    curl_easy_setopt(curl_handle, CURLOPT_URL, *url);    
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
    /* pass data to callback function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&wdi);
    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */ 
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    /* get it! */ 
    res = curl_easy_perform(curl_handle);
    /* check for errors */ 
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
	      curl_easy_strerror(res));
    }
    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);
  }
  else {
    fprintf(stderr, "Error: could not get CURL handle.\n");
    exit(EXIT_FAILURE);
  }

  result = wdi.data;

  if (wdi.data) {
    free(wdi.data);
  }
  
  return (&result);
}
