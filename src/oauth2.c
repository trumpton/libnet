//
// oauth2.c
//
// These functions allow an oauth2 access token to be managed
//
// RFC6749 - OAuth2
//
// OAUTH2 *oauth2open(char *cachefile)
// int oauth2registerapplication(OAUTH2 *oh, char *accessurl, char *authoriseurl,
//             char *refreshurl, char *usernameurl, char *scope, char *id, char *secret)
// int oauth2authorise(OAUTH2 *oh, char *callbackurl)
// char *oauth2token(OAUTH2 *oh)
// int oauth2close(OAUTH2 *oh)
//
// Example URLs:
//
// access token (post): https://accounts.google.com/o/oauth2/token
// authorise (post): https://accounts.google.com/o/oauth2/device/code
// refresh token (post): https://www.googleapis.com/oauth2/v4/token
// username (get): https://www.googleapis.com/oauth2/v3/userinfo
//

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

typedef struct {
  char *cachebase, *cacheext ;
  char *id ;
  char *secret ;
  char *code ;
  char *refreshtoken ;
  char *accesstoken ;
  time_t accesstoken_expiry ;
} IOAUTH2 ;

#define OAUTH2 IOAUTH2

#include "../oauth2.h"
#include "../json.h"
#include "../httpc.h"


// Local functions

int _oauth2_save(IOAUTH2 *oh, char *ext, char *string, char **destcopy) ;
int _oauth2_load(IOAUTH2 *oh, char *ext, char **string) ;

//
// @brief Open an OAuth2 session
// @param(in) cachefile File path (without extension) used to store Oauth info
// @return Handle to OAUTH2 structure, or NULL on failure (and sets errno)
//

IOAUTH2 *oauth2open(char *cachefile)
{
  if (!cachefile) return NULL ;

  IOAUTH2 *oh = malloc(sizeof(IOAUTH2)) ;
  if (!oh) return NULL ;

  oh->cachebase=NULL ;
  oh->cacheext=NULL ;
  oh->id=NULL ;
  oh->secret=NULL ;
  oh->refreshtoken=NULL ;
  oh->accesstoken=NULL ;

  oh->cachebase = malloc(strlen(cachefile)+4) ;
  if (!(oh->cachebase)) goto fail ;
  strcpy(oh->cachebase, cachefile) ;
  oh->cacheext=&(oh->cachebase[strlen(oh->cachebase)]) ;

  // Attempt to load data if it is available

  _oauth2_load(oh, ".id", &(oh->id)) ;
  _oauth2_load(oh, ".se", &(oh->secret)) ;
  _oauth2_load(oh, ".rt", &(oh->refreshtoken)) ;

  // Flag access token as expired

  oh->accesstoken_expiry=0 ;

  return oh ;

fail:
  if (oh) {
    oauth2close(oh) ;
    free(oh) ;
  }
  return NULL ;
}


//
// @brief Authorise use, and get refresh_token
// @param(in) oh Handle to OAUTH2 structure
// @param(in) accessurl URL to get access token
// @param(in) authoriseurl URL to request authorisation
// @param(in) refreshurl URL to request refresh token
// @param(in) usernameurl URL to request logged in username
// @param(in) scope Scope of oauth2 session
// @param(in) id ID of application
// @param(in) secret Secret for application
// @return True on success, false on failure (and sets errno)
//

int oauth2authorisation( IOAUTH2 *oh, char *accessurl, char *scope,
                         char *id, char *secret, char *redirecturl )
{
  if (!oh || !accessurl || !scope || !id || !secret || !redirecturl)
    return 0 ;

  if (oh->accesstoken) {
    free(oh->accesstoken) ;
    oh->accesstoken=NULL ;
  }

  oh->accesstoken_expiry=0 ;
  char scbuf[2048], rebuf[2048], buf[2048] ;
  int ok=1 ;

  ok &= httpencode(id, buf, sizeof(buf)) & _oauth2_save(oh, ".id", buf, &(oh->id)) ;
  ok &= httpencode(secret, buf, sizeof(buf)) & _oauth2_save(oh, ".se", buf, &(oh->secret)) ;

  ok &= httpencode(redirecturl, rebuf, sizeof(rebuf)) ;
  ok &= httpencode(scope, scbuf, sizeof(scbuf)) ;
  if (!ok) goto fail ;

  // Access tokens can be large, e.g. google = 2048 bytes
  char query[8192] ;
  char *response = NULL ;
  int responselen = 0 ;
  int r ;

  snprintf( query, sizeof(query), "%s?grant_type=authorization_code&code=%s"
                                  "&client_id=%s&client_secret=%s"
                                  "&scope=%s&redirect_uri=%s",
            accessurl, oh->code,
            oh->id, oh->secret,
            scbuf, rebuf) ;

  char *oa2token = oauth2token(oh) ;
  r=httpsend(POST, query, oa2token, "application/x-www-form-urlencoded", query, &response, &responselen) ;

  if (r==200) {

    JSON *jh = jsonnew() ;
    jsonset(jh, response) ;

    char *refreshtoken = jsonget(jh, "/refresh_token") ;
    if (refreshtoken)
        _oauth2_save(oh, ".rt", refreshtoken, &(oh->refreshtoken)) ;

    char *expires = jsonget(jh, "/expires_in") ;
    if (expires) {
      time_t t = time(NULL) + atoi(expires) - 60 ;
      oh->accesstoken_expiry = t ;
    }
    char *accesstoken = jsonget(jh, "/access_token") ;
    if (accesstoken) {
      if (oh->accesstoken) free(oh->accesstoken) ;
      oh->accesstoken=NULL ;
      oh->accesstoken=malloc(strlen(accesstoken)+1) ;
      if (oh->accesstoken) strcpy(oh->accesstoken, accesstoken) ;
    }
    jsondelete(jh) ;

    ok &= (accesstoken && refreshtoken && expires) ;

  } else {

    ok=0 ;

  }

  if (response) free(response) ;
  return ok ;

fail:
  return 0 ;

}



//
// @brief Retrieve valid access token to use in requests
// @param(in) oh Handle to OAUTH2 structure
// @return Pointer to token, or NULL on error
//

char *oauth2token(IOAUTH2 *oh)
{
  if (!oh) return NULL ;

  if (time(NULL) > oh->accesstoken_expiry) {

    // Access token has expired, get a new one

  }

  return oh->accesstoken ;
}


//
// @brief Close Oauth2 and release memory
// @param(in) oh Handle to OAUTH2 structure
// @return True on success
//

int oauth2close(IOAUTH2 *oh)
{
  if (!oh) return 0 ;
  if (oh->cachebase) free(oh->cachebase) ;
  if (oh->secret) free(oh->secret) ;
  if (oh->refreshtoken) free(oh->refreshtoken) ;
  if (oh->accesstoken) free(oh->accesstoken) ;
  free(oh) ;
  return 1 ;
}

//
//
//
//

int _oauth2_save(IOAUTH2 *oh, char *ext, char *string, char **destcopy)
{
  if (!oh) return 0 ;
  if (!string || !destcopy) return 0 ;

  if (*destcopy) free(*destcopy) ;
  (*destcopy) = NULL ;

  strcpy(oh->cacheext, ext) ;
  int fd=open(oh->cachebase, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR) ;
  if (fd<1) return 0 ;
  int len = write(fd, string, strlen(string)) ;
  close(fd) ;
  if (len<0) return 0 ;

  (*destcopy) = malloc(len+1) ;
  if (!(*destcopy)) return 0 ;
  strncpy(*destcopy, string, len) ;
  return 1 ;
}

int _oauth2_load(IOAUTH2 *oh, char *ext, char **string)
{
  char buf[1024] ;

  if (!oh) return 0 ;
  if (!string) return 0 ;

  if (*string) free(*string) ;
  (*string) = NULL ;

  strcpy(oh->cacheext, ext) ;
  int fd=open(oh->cachebase, O_RDONLY) ;
  if (fd<1) return 0 ;
  int len = read(fd, buf, sizeof(buf)) ;
  close(fd) ;
  if (len<0) return 0 ;

  (*string) = malloc(len+1) ;
  if (!(*string)) return 0 ;
  strncpy(*string, buf, len) ;
  return 1 ;
}


