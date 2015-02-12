#ifdef _MSC_VER
#include <string.h>
#define strcasecmp _stricmp
#define  strncasecmp _strnicmp
#else
#include <string.h>
#include <strings.h>
#endif
