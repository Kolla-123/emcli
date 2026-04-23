#ifndef JSMN_H
#define JSMN_H

#include <stddef.h>

/* ========================= JSMN PARSER BEGIN ========================= */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

typedef enum {
  JSMN_UNDEFINED = 0,
  JSMN_OBJECT = 1 << 0,
  JSMN_ARRAY = 1 << 1,
  JSMN_STRING = 1 << 2,
  JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

enum jsmnerr {
  JSMN_ERROR_NOMEM = -1,
  JSMN_ERROR_INVAL = -2,
  JSMN_ERROR_PART = -3
};

typedef struct jsmntok {
  jsmntype_t type;
  int start;
  int end;
  int size;
#ifdef JSMN_PARENT_LINKS
  int parent;
#endif
} jsmntok_t;

typedef struct jsmn_parser {
  unsigned int pos;
  unsigned int toknext;
  int toksuper;
} jsmn_parser;

JSMN_API void jsmn_init(jsmn_parser *parser);
JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                        jsmntok_t *tokens, const unsigned int num_tokens);

#ifdef __cplusplus
}
#endif
/* ========================== JSMN PARSER END ========================== */

#endif /* JSMN_H */
