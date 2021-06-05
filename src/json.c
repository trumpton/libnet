//
// json.c
//
// These functions allow JSON data to be managed
//
// JSON *jsonnew()
// int jsonset(JSON *jh, char *jsonsource)
// char *jsonget(JSON *jh, char *tagpath)
// int jsondelete(JSON *jh)
//

#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>

struct tagentry {
  struct tagentry *next ;
  struct tagentry *child ;
  int childisarray ;
  char *label ;
  char *data ;
} ;

typedef struct {
  struct tagentry *root ;
  char *returnbuf ;
} IJSON ;

#define JSON IJSON
#include "../json.h"


// Local Functions
int _jsondeleteentry(struct tagentry *te) ;
struct tagentry *_jsonbuild(char *json, int *pos, int depth) ;
struct tagentry *_jsonnewentry() ;
int _jsonfieldlen(char *start) ;
void _jsondumpentry(struct tagentry *te, int level) ;



//
// @brief Create a JSON parser object
// @return Handle to JSON structure, or NULL on failure (and sets errno)
//

IJSON *jsonnew()
{
  IJSON *jh = malloc(sizeof(IJSON)) ;
  if (!jh) return NULL ;

  jh->root=NULL ;
  jh->returnbuf=NULL ;

  return jh ;
}


//
// @brief Assigns json data to parser object
// @param(in) jh Handle of JSON object
// @param(in) jsonsource Pointer to source of JSON data
// @return true on success, false on failure (and sets errno)
//

int jsonset(IJSON *jh, char *jsonsource)
{
  if (!jh) return 0 ;
  _jsondeleteentry(jh->root) ;
  jh->root=NULL ;
  int len=0 ;
  jh->root = _jsonbuild(jsonsource, &len, 0) ;
  return (len>=0) ;
}


//
// Extract requested entry from JSON.  Tagpath is provided
// in the form /label1/label2/0/label3, where numbers are provided
// for array entries. Note numbers must not be padded, i.e. '00' is
// invalid.
//
// @brief Returns parameter at requested position
// @param(in) jh Handle of JSON object
// @param(in) tagpath Path in JSON object of required parameter
// @return pointer to parameter or NULL if not found
//

char *jsonget(IJSON *jh, char *tagpath) 
{
  if (!jh || !jh->root || !tagpath) return NULL ;

  char *src = tagpath ;
  if (*src=='/') src++ ;

  struct tagentry *te=jh->root ;
  if (strcmp(te->label, "0")==0) te=te->child ;

  if (te) do {

    // Search for matching label

    int found=0 ;
    
    do {

      int lablen = strlen(te->label) ;
      if (strncmp(src, te->label, lablen)==0 && (src[lablen]=='\0' || src[lablen]=='/')) {
        found=1 ;
      } else {
        te=te->next ;
      }

    } while (te && !found) ;

    // Update search string and tag pointer

    if (te) {

      src += strlen(te->label) ;

      if (*src=='/') {
        src++ ;
        te=te->child ;
      } else if (*src!='\0') {
        te=NULL ;
      }

    }

  } while (te && *src!='\0') ;

  // And return match

  if (!te) {
    return NULL ;
  } else {
    return te->data ;
  }

}

//
// @brief Dump JSON Data structure
// @param[in] jh handle for JSON data
// @return nothing
//

void jsondump(IJSON *jh)
{
  if (!jh || !jh->root) return ;
  _jsondumpentry(jh->root, 0) ;
}

void _jsondumpentry(struct tagentry *te, int level)
{
  if (!te) return ;

  for (int i=0; i<level; i++)  { printf("  ") ; }
  if (te->label) printf("%s:", te->label) ;
  if (te->data) printf(" %s", te->data) ;
  else if (te->childisarray) printf("[") ;
  else printf("{") ;
  printf("\n") ;
  if (te->child) _jsondumpentry(te->child, level+1) ;
  for (struct tagentry *lp=te->next; lp; lp=lp->next) _jsondumpentry(lp, level) ;
}


//
// @brief Deletes JSON parser object and frees memory
// @param(in) jh Handle of JSON object
// @return true on success, false on failure (and sets errno)
//

int jsondelete(IJSON *jh)
{
  if (!jh) return 0 ;

  _jsondeleteentry(jh->root) ;
  if (jh->returnbuf) free(jh->returnbuf) ;
  free(jh) ;
  return 1 ;
}


// counter=0
// maintain depth counter
// root link object = null
// link object pointer = null
// Loop
//   skip white spaces
//   Create link object and store in chain
//   If it is first, save 'root'
//   try to get label and update position
//   If no label, use next counter in sequence
//   skip white spaces or colons
//   Check character
//   if { call self and store as child
//   if [ call self and store as child (array)
//   else capture record data and update position
//   Skip to end of record
// Loop if comma
// If not eof or } or ] there is an error
// upate position to just after end of record
// return root link object


struct tagentry *_jsonbuild(char *json, int *pos, int depth)
{

  if (!json || !pos) return NULL ;

  // counter=0

  int entry=0 ;

  // maintain depth counter

  if (depth>10) goto fail ;
 
  // root link object = null
  // link object pointer = null

  struct tagentry *teroot = NULL;
  struct tagentry *te = NULL ;

  // Loop

  int commadetected ;

  do {

    // skip white spaces

    while (isspace(json[*pos])) (*pos)++ ;
    if (json[*pos]=='\0' || json[*pos]=='}' || json[*pos]==']') break ;

    // Create link object and store in chain

    struct tagentry *nte = malloc(sizeof(struct tagentry)) ;
    if (!nte) goto fail ;
    explicit_bzero(nte, sizeof(struct tagentry)) ;

    if (!teroot) {
      te=nte ;
      teroot=nte ;
    } else {
      te->next=nte ;
      te=te->next ;
    }

    // try to get label and update position

    if (json[*pos]=='\"') {

        int labellen = _jsonfieldlen(&(json[*pos])) ;
        te->label = malloc(labellen-1) ;
        if (!te->label) goto fail ;
        strncpy(te->label, &(json[(*pos)+1]), labellen-2) ;
        te->label[labellen-2]='\0' ;
        (*pos)+=labellen ;

    }

    // If no label, use next counter in sequence

    else {

      char counter[16] ;
      sprintf(counter, "%d", entry++) ;
      te->label = malloc(strlen(counter)+1) ;
      if (!te->label) goto fail ;
      strcpy(te->label, counter) ;

    }

    // skip white spaces or colons

    while (isspace(json[*pos]) || json[*pos]==':') (*pos)++ ;

    // Check character
    // if { call self and store as child

     if (json[*pos]=='{') {

      (*pos)++ ;
      te->childisarray=0 ;
      te->child = _jsonbuild(json, pos, depth+1) ;
      if ((*pos)<0) goto fail ;

    }

    // if [ call self and store as child (array)

    else if (json[*pos]=='[') {

      (*pos)++ ;
      te->childisarray=1 ;
      te->child = _jsonbuild(json, pos, depth+1) ;
      if ((*pos)<0) goto fail ;

    }

    // else capture record data and update position

    else {

        int datalen = _jsonfieldlen(&(json[*pos])) ;

        if (json[*pos]=='\"') {
          te->data = malloc(datalen-1) ;
          if (!te->data) goto fail ;
          strncpy(te->data, &(json[(*pos)+1]), datalen-2) ;
          te->data[datalen-2]='\0' ;
        } else {
          te->data = malloc(datalen+1) ;
          if (!te->data) goto fail ;
          strncpy(te->data, &(json[(*pos)]), datalen) ;
          te->data[datalen]='\0' ;
        }
        (*pos)+=datalen ;

    }

    // Skip to end of record

    commadetected=0 ;
    while (isspace(json[*pos]) || json[*pos]==',') {
      if (json[*pos]==',') commadetected=1 ;
      (*pos)++ ;
    }

  // Loop if comma

  } while (commadetected) ;

  // If not eof or } or ] there is an error

  if (json[*pos]!='}' && json[*pos]!=']' && json[*pos]!='\0') {
    goto fail ;
  }

  // upate position to just after end of record

  if (json[*pos]!='\0') (*pos)++ ;

  // return root link object

  return teroot ;

fail:
  _jsondeleteentry(teroot) ;
  (*pos)=-1 ;
  return NULL ;
}

int _jsondeleteentry(struct tagentry *te)
{
  if (!te) return 0 ;
  _jsondeleteentry(te->next) ;
  _jsondeleteentry(te->child) ;
  if (te->label) free(te->label) ;
  if (te->data) free(te->data) ;
  free(te) ;
  return 1 ;
}


struct tagentry *_jsonnewentry()
{
  struct tagentry *te = malloc(sizeof(struct tagentry)) ;
  if (!te) return NULL ;
  explicit_bzero(te, sizeof(struct tagentry)) ;
}


int _jsonfieldlen(char *str)
{
  int instring ;
  int escape=0 ;
  int len=0 ;
  
  if (*str=='\"') {
    instring=1 ;
    len++ ;
  } else {
    instring=0 ;
  }
  do {

    escape = (str[len]=='\\') ;
    len++ ;

  } while ( str[len]!='\0' && ( escape ||
            (instring && str[len]!='\"') ||
            (!instring && isalnum(str[len])) ) ) ;

  return len+instring ;
}



