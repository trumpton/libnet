//
// httpc.h
//
// These functions are designed to provide a simple
// httpd client, which can accept send requests and
// received responses.
//
// int httpencode(char *src, char *dst, int maxdst)
// int httpsend(enum http_type type, char *url, OAUTH2 *oa,
//              char *encoding, char *request, char *response, int maxlen)
//

#ifndef _HTTPC_DEFINED
#define _HTTPC_DEFINED

#include "oauth2.h"

enum http_type {
  HEAD, GET, POST, PUT, DELETE, PATCH, OPTIONS
} ;

//
// @brief Performs HTTP GET/PUT/POST etc.
// @param[in] type Type of submission (GET/PUT/POST...)
// @param[in] url Url including query string for connection
// @param[in] oa2token OAuth2 token, or NULL if not required
// @param[in] encoding Encondig type (e.g. text/plain)
// @param[in] request Request payload
// @param[out] response Pointer to response buffer to store response
// @param[in] responselen Pointer to location to restore length of response
// @return http response code (200=OK, 999=Internal Fault)
//

int httpsend( enum http_type type, char *url, char *oauth2token, char *encoding,
              char *request, char **response, int *responselen ) ;

//
// @brief Perform percent encoding on string
// @param[in] src Null terminated source string
// @param[out] dst Destination buffer
// @param[in] maxdst maximum length of dst buffer
// @return true on success
//

int httpencode( char *src, char *dst, int maxdst ) ;


#endif
