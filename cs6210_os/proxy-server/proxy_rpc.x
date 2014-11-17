/*
 * Specify an interface with one procedure named httpget_1 that
 * takes takes a string (the url) as an input and returns a string.
 *
 * Your RPCL code here.
*/
program HTTPCACHEPROG {
  version HTTPCACHEVERS {
    string HTTPGET(string) = 1;
  } = 1;
} = 99;
