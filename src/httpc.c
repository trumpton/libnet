//
// httpc.c
//
// These functions are designed to provide a simple
// httpd client, which can accept send requests and
// received responses.
//
// int httpencode(char *src, char *dst, int maxdst)
// int httpsend(enum http_type type, char *url, char *oauth2token,
//              char *encoding, char *request, char *response, int maxlen)
//

#define _GNU_SOURCE
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../httpc.h"
#include "../net.h"


int _httpc_httpsend( enum http_type type, char *url, char *oa2token, char *encoding,
                     char *request, char **response, int *responselen ) ;
char *_httpc_parsehost( char *url, int *ishttps, int *port, char **uri ) ;
int _httpc_getresponse( NET *session, char **response, int *responselen ) ;
int _httpc_appendbuf( char *line, int len, char **response, int *responselen ) ;
int _httpc_htoi( char *h ) ;

//
// @brief Performs HTTP GET/PUT/POST etc.
// @param[in] type Type of submission (GET/PUT/POST...)
// @param[in] url Url including query string for connection
// @param[in] oa2token OAuth2 token, or NULL if not required
// @param[in] encoding Encondig type (e.g. text/plain)
// @param[in] request Request payload
// @param[out] response Pointer to buffer to store response
// @param[in] responselen Pointer to integer to store response length
// @return http response code (200=OK, 999=Internal Fault)
//

int httpsend(enum http_type type, char *url, char *oa2token, char *encoding, char *request, char **response, int *responselen)
{
  int n=0 ;
  int r=0 ;
  char *urlbuf = NULL ;

  // Copy URL into buffer

  urlbuf = malloc(strlen(url)+1) ;
  if (!urlbuf) goto fail ;
  strcpy(urlbuf, url) ;

  do {

    // Reset response buffer

    if (*response) free(*response) ;
    *response = NULL ;
    *responselen = 0 ;

    // Fetch page

    r = _httpc_httpsend(type, urlbuf, oa2token, encoding, request, response, responselen) ;
    free(urlbuf) ; urlbuf=NULL ;

    // If re-direct, capture new URL

    if (r>=300 && r<=399 && (*response) && (*responselen)>0) {
      urlbuf = malloc((*responselen)+1) ;
      if (!urlbuf) goto fail ;
printf("Copying response '%s'\n", (*response)) ;
      strcpy(urlbuf, (*response)) ;
    }

    // Redirect recursion protection
    n++ ;

  } while (r>=300 && r<=399 && n<5) ;

  if (n==5) goto fail ;

  if (urlbuf) free(urlbuf) ;
  return r ;

fail:

  if (urlbuf) free(urlbuf) ;
  return 500 ;

}


int _httpc_httpsend(enum http_type type, char *url, char *oa2token, char *encoding, char *request, char **response, int *responselen)
{
  char *host=NULL, *uri ;
  int port, ishttps=0 ;
  NET *session=NULL ;
  char contentlength[16] ;

  if (!url || !response) goto fail ;

  // Get type string

  char types[16] ;
  int hasbody=0 ;
  switch (type) {
  case GET: strcpy(types, "GET ") ; hasbody=0 ; break ;
  case HEAD: strcpy(types, "HEAD ") ; hasbody=0 ; break ;
  case OPTIONS: strcpy(types, "OPTIONS ") ; hasbody=0 ; break ;
  case PUT: strcpy(types, "PUT ") ; hasbody=1 ; break ;
  case POST: strcpy(types, "POST ") ; hasbody=1 ; break ;
  case PATCH: strcpy(types, "PATCH ") ; hasbody=1 ; break ;
  case DELETE: strcpy(types, "DELETE ") ; hasbody=1 ; break ;
  default: goto fail ; break ;
  }

  // Calculate content length

  if (request) {
    sprintf(contentlength, "%ld", strlen(request)) ;
  } else {
    strcpy(contentlength, "0") ;
  }

  host = _httpc_parsehost(url, &ishttps, &port, &uri) ;
  if (!host) goto fail;

  enum netflags flags = 0 ;

  if (ishttps) {
    flags |= (SSL2|SSL3) ;
  } else {
    flags |= (OPEN) ;
  }

  #ifdef DEBUG
  flags |= DEBUGDATADUMP ;
  #endif // DEBUG

  session = netconnect(host, port, flags) ;
  if (!session) goto fail ;

  int ok=1 ;

  ok &= (netsend(session, types, strlen(types)) >=0 ) ;
  ok &= (netsend(session, uri, strlen(uri)) >=0 ) ;
  ok &= (netsend(session, " HTTP/1.1\r\n", 11) >=0 ) ;

  ok &= (netsend(session, "Host: ", 6) >=0 ) ;
  ok &= (netsend(session, host, strlen(host)) >=0 ) ;
  ok &= (netsend(session, "\r\n", 2) >=0 ) ;

  ok &= (netsend(session, "Connection: Close\r\n", 19) >=0 ) ;


  if (hasbody) {

    if (!encoding || !request) goto fail ;

    ok &= (netsend(session, "Content-Type: ", 14) >=0 ) ;
    ok &= (netsend(session, encoding, strlen(encoding)) >=0 ) ;
    ok &= (netsend(session, "\r\n", 2) >=0 ) ;

    ok &= (netsend(session, "Content-Length: ", 14) >=0 ) ;
    ok &= (netsend(session, contentlength, strlen(contentlength)) >=0 ) ;
    ok &= (netsend(session, "\r\n", 2) >=0 ) ;

  }

  if (oa2token) {

    ok &= (netsend(session, "Token: ", 7) >=0 ) ;
    ok &= (netsend(session, oa2token, strlen(oa2token)) >=0 ) ;
    ok &= (netsend(session, "\r\n", 2) >=0 ) ;

  }

//  ok &= (netsend(session, "Connection: close", 17) >=0 ) ;
//  ok &= (netsend(session, "\r\n", 2) >=0 ) ;

  ok &= (netsend(session, "\r\n", 2) >=0 ) ;

  if (hasbody && request) {
    ok &= (netsend(session, request, strlen(request)) >=0 ) ;
  }

  if (!ok) goto fail ;

  int code = _httpc_getresponse(session, response, responselen) ;

  netclose(session) ;
  free(host) ;

  return code ;

fail:

  if (host) free(host) ;
  if (session) netclose(session) ;
  return 999 ;
}

//
// @brief Perform percent encoding on string
// @param[in] src Null terminated source string
// @param[out] dst Destination buffer
// @param[in] maxdst maximum length of dst buffer
// @return true on success
//

int httpencode(char *src, char *dst, int maxdst)
{
  if (!src || !dst) return 0 ;
  *dst=0 ;
  int di=0 ;
  while (di<(maxdst-5) && *src!='\0') {
    if (isalnum(*src)) {
      dst[di++]=(*src) ;
    } else {
      char buf[4] ;
      sprintf(buf, "%%%02X", *src) ;
      strcpy(&(dst[di]), buf) ;
      di+=3 ;
    }
    src++ ;
  }
  dst[di]='\0' ;
  return 1 ;
}

char *_httpc_parsehost(char *url, int *ishttps, int *port, char **uri)
{
  int buflen=strlen(url)+2 ;
  char *buf = malloc(buflen) ;
  if (!buf) return NULL ;
  int httplen=0 ;

  (*buf)='\0' ;
  (*ishttps)=0 ;
  (*port)=0 ;
  (*uri)=NULL ;

  // Get http type

  if (strncasecmp(url, "http://", 7)==0) {
    httplen=7 ;
    (*port)=80 ;
    (*ishttps)=0 ;
  } else if (strncasecmp(url, "https://", 8)==0) {
    httplen=8 ;
    (*port)=443 ;
    (*ishttps)=1 ;
  }
  if ((*port)==0) goto fail ;

  // Terminate hostname and extract port if present
  // buf will become hostname \0 /uripath \0

  // Copy hostname into buf

  int bp=0 ;
  int up=httplen ;

  while (url[up]!=':' && url[up]!='/' && url[up]!='\0') {
    buf[bp++] = url[up++] ;
  }
  buf[bp++]='\0' ;

  // Extract port

  if (url[up]==':') {
    up++ ;
    (*port)=atoi(&url[up]) ;
    while (url[up]!='/' && url[up]!='\0') up++ ;
  }

  // Copy uri into buf

  (*uri)=&buf[bp] ;

  if (url[up]=='\0') {

    // No path provided
    buf[bp++]='/' ;

  } else {

    // Copy path
    while (url[up]!='\0') {
      buf[bp++]=url[up++] ;
    }

  }

  // Terminate path
  buf[bp]='\0' ;

  return buf ;

fail:
  if (buf) free(buf) ;
  return NULL ;
}

#define ST_RESPONSE 0
#define ST_HEADER 1
#define ST_BODY 2
#define ST_CHUNKEDBODYLEN 3
#define ST_CHUNKEDBODY 4
#define ST_END 5
#define ST_ERROR 6

int _httpc_getresponse(NET *session, char **response, int *responselen)
{
/*
  int error=0, done=0 ;
  int newlinecount=0 ;
  int bp=0 ;
*/

  int chunkedencoding=0 ;
  int responsecode=999 ;
  int state=ST_RESPONSE;
  int bodylength=0 ;
  char line[1024] ;
  int lp=0 ;
  int len=0 ;

  if (!session || !response|| !responselen) return 999 ;

  do {

    // Receive Character

// TODO: call with non-blocking and wrap in select
// introduce timeout

    char ch ;
    switch (netrecv(session, &ch, 1)) {

      case 0:

        // No data available
        break ;

      case 1:

        // Receive character
        line[lp++]=ch ;
        line[lp]='\0' ;

        if (ch=='\n' || lp>=(sizeof(line)-1)) {

          char *x ;

          switch (state) {

            case ST_RESPONSE:

              // Handle HTTP response line

              x = strstr(line, " ") ;
              if (!x) {
                state = ST_ERROR ;
              } else {
                responsecode=atoi(&x[1]) ;
                state = ST_HEADER ;
             }
             break ;

            case ST_HEADER:

              // Check header parameters as they go past

              x=strcasestr(line, "Content-Length:") ;
              if (x) {
                bodylength = atoi(&x[15]) ;
              }

              x=strcasestr(line, "Transfer-Encoding: Chunked") ;
              if (x) {
                chunkedencoding=1 ;
              }

              x=strcasestr(line, "Location: ") ;
              if (responsecode>=300 && responsecode<=399 && x) {

                // Capture redirection location in the response

                for ( (*responselen)=0; !isspace(x[(*responselen)+10]); (*responselen)++) ;

                (*response) = malloc((*responselen)+1) ;
                if (!(*response)) goto fail ;

                strncpy((*response), &x[10], (*responselen)) ;
                (*response)[(*responselen)]='\0' ;

                state = ST_END ;

              }

              if (strcmp(line, "\r\n")==0) {
                if (chunkedencoding) state = ST_CHUNKEDBODYLEN ;
                else if (bodylength>0) state = ST_BODY ;
                else state = ST_END ;
              }


              break ;

            case ST_BODY:

              _httpc_appendbuf(line, lp, response, responselen) ;
              bodylength -= lp ;
              if (bodylength<=0) {
                state = ST_END ;
              }
              break ;

            case ST_CHUNKEDBODYLEN:

              bodylength = _httpc_htoi(line) ;
              if (bodylength<=0) {
                state = ST_END ;
              } else {
                state = ST_CHUNKEDBODY ;
              }
              break ;

            case ST_CHUNKEDBODY:

              _httpc_appendbuf(line, lp<=bodylength?lp:bodylength, response, responselen) ;
              bodylength -= lp ;
              if (bodylength<=0) {
              bodylength = _httpc_htoi(line) ;
                state = ST_CHUNKEDBODYLEN ;
              }
              break ;

          }

          lp=0 ;

        }

        break ;

      default:

        // End of file or file closed
        state = ST_END ;
        break ;

    }

  } while (state!=ST_ERROR && state!=ST_END) ;

  if (state==ST_ERROR) responsecode=500 ;

  if (responsecode<200 || responsecode>399) {
    if (*response) free(*response) ;
    (*response) = NULL ;
    (*responselen) = 0 ;
  }

  return responsecode ;

fail:
  if (*response) free(*response) ;
  (*response) = NULL ;
  (*responselen) = 0 ;
  return 999 ;
}


int _httpc_appendbuf(char *line, int len, char **response, int *responselen)
{

  if (!line || len<0 || !response || !responselen) return 0 ;

  // Nothing to do
  if (len==0) return 1 ;

  // Increase buffer
  char *newresponse = realloc(*response, (*responselen)+len+1) ;
  int newlen = (*responselen) + len ;

  // Allocation failure
  if (!newresponse) return 0 ;

  // Append data and terminate
  memcpy(&newresponse[(*responselen)], line, len) ;
  newresponse[newlen]='\0' ;

  // Update pointers
  (*response) = newresponse ;
  (*responselen) = newlen ;

  return 1 ;

}

int _httpc_htoi(char *h)
{
  int i=0 ;
  while (isspace(*h)) h++ ;
  int ch ;
  do {
    ch=-1 ;
    if (tolower(*h)>='a' && tolower(*h)<='f') ch=tolower(*h) - 'a' + 10 ;
    else if ((*h)>='0' && (*h)<='9') ch = (*h) - '0' ;
    if (ch>=0) {
      i = i * 16 + ch ;
    }
    h++ ;
  } while (ch>=0) ;
  return i ;
}


