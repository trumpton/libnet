//
// json.h
//
// These functions allow JSON data to be managed
//
// JSON *jsonnew(char *jsonsource)
// int jsonset(JSON *jh, char *jsonsource)
// char *jsonget(JSON *jh, char *tagpath)
// int jsondelete(JSON *jh)
//

#ifndef _JSON_DEFINED
#define _JSON_DEFINED

#ifndef JSON
typedef struct {} JSON ;
#endif


//
// @brief Create a JSON parser object
// @return Handle to JSON structure, or NULL on failure (and sets errno)
//

JSON *jsonnew() ;


//
// @brief Assigns json data to parser object
// @param(in) jh Handle of JSON object
// @param(in) jsonsource Pointer to source of JSON data
// @return true on success, false on failure (and sets errno)
//

int jsonset(JSON *jh, char *jsonsource) ;


//
// Extract requested entry from JSON.  Tagpath is provided
// in the form "/label1/label2/0/label3", where numbers are provided
// for array entries. Note numbers must not be padded, i.e. '00' is
// invalid.
//
// @brief Returns parameter at requested position
// @param(in) jh Handle of JSON object
// @param(in) tagpath Path in JSON object of required parameter
// @return pointer to parameter or NULL if not found
//

char *jsonget(JSON *jh, char *tagpath) ;


//
// @brief Dump JSON Data structure
// @param[in] jh handle for JSON data
// @return nothing
//

void jsondump(JSON *jh) ;


//
// @brief Deletes JSON parser object and frees memory
// @param(in) jh Handle of JSON object
// @return true on success, false on failure (and sets errno)
//

int jsondelete(JSON *jh) ;



#endif

