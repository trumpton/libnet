//
// oauth2.h
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

#ifndef _OAUTH2_DEFINED
#define _OAUTH2_DEFINED

#ifndef OAUTH2
typedef struct {} OAUTH2 ;
#endif


//
// @brief Open an OAuth2 session
// @param(in) cachefile File path (without extension) used to store Oauth info
// @return Handle to OAUTH2 structure, or NULL on failure (and sets errno)
//

OAUTH2 *oauth2open(char *cachefile) ;

//
// @brief Register an OAuth2 application
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

int oauth2registerapplication( OAUTH2 *oh, char *accessurl, char *authoriseurl, 
                               char *refreshurl, char *usernameurl, char *scope, 
                               char *id, char *secret ) ;


//
// @brief Authorise an OAuth2 application
// @param(in) oh Handle to OAUTH2 structure
// @param(in) callbackurl URL to for the browser to be directed to on success
// @return True on success, false on failure (and sets errno)
//

int oauth2authorise(OAUTH2 *oh, char *callbackurl) ;


//
// @brief Retrieve valid access token to use in requests
// @param(in) oh Handle to OAUTH2 structure
// @return Pointer to token, or NULL on error
//

char *oauth2token(OAUTH2 *oh) ;


//
// @brief Close Oauth2 and release memory
// @param(in) oh Handle to OAUTH2 structure
// @return True on success
//

int oauth2close(OAUTH2 *oh) ;


#endif

