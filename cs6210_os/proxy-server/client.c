#include <stdlib.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include "proxy_rpc.h" 

int main(int argc, char* argv[]){
  CLIENT *cl;
  char** result;
  char* url;
  char* server;
  
  if (argc < 3) {
    fprintf(stderr, "usage: %s host url\n", argv[0]);
    exit(1);
  }
  /*
   * Save values of command line arguments
   */
  server = argv[1];
  url = argv[2];

  /*
   * Your code here.  Make a remote procedure call to
   * server, calling httpget_1 with the parameter url
   * Consult the RPC Programming Guide.
   */
  cl = clnt_create(server, HTTPCACHEPROG, HTTPCACHEVERS, "tcp");
  if (cl == NULL) {
    /* Could't establish connection with server.
     * print error message and die
     */
    clnt_pcreateerror(server);
    exit(1);
  }
  /*
   * Call the remote procedure "httpget" on the server
   */
  result = httpget_1(&url, cl);
  if (result == NULL) {
    /* An error occured while calling the server.
     * print error msg and die
     */
    clnt_perror(cl, server);
    exit(1);
  }
  /* successfully called the remote procedure*/
  if (*result == NULL)
  {
    /* unable to access the server url*/
    fprintf(stderr, "%s: %s couldn't process your url request \n",url, server);
    exit(1);
  }
  //printf("Message delivered to %s!\n", server);
  //fprintf(stdout, "%s\n", *result);
   printf("%s\n", *result);
  
  return 0;
}
