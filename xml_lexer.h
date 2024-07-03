#ifndef __XML_LEXER_H__
#define __XML_LEXER_H__

#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/* position struct */
typedef struct src_pos {
  const char *file;
  int line;
  int column;
}src_pos_t;

typedef enum {
  TOKEN_NONE = 0,
  TOKEN_EOF,              /* EOF */
  TOKEN_OPEN_TAG,         /* < */
  TOKEN_CLOSE_TAG,        /* > */
  TOKEN_OPENSLASH_TAG,    /* </ */
  TOKEN_CLOSESLASH_TAG,   /* /> */
  TOKEN_OPEN_HEADER,      /* <? */
  TOKEN_CLOSE_HEADER,     /* ?> */

  TOKEN_NAME,             /* tag name */
  TOKEN_COMMENT,
  TOKEN_ASSIGN,           /* = */
  TOKEN_STRING,           /* "" */
  TOKEN_TEXT              /* tag text */
}token_type_t;

/* token struct */
typedef struct token {
  token_type_t type;
  const char *literal;
  int len;
  src_pos_t pos;
}token_t;

/* lex struct */
typedef struct lexer {
  const char *input;
  int input_len;
  const char *file;

  char ch;
  int line, column;
  int position, next_position;

  token_t cur_token;
  token_t peek_token;
  bool inTag;
}lexer_t;

#ifdef DEBUG
void token_dump(token_t tok);
#endif

bool lexer_init(lexer_t *lex, const char *input, const char *filename);
bool lexer_cur_token_is(lexer_t *lex, token_type_t type);
bool lexer_peek_token_is(lexer_t *lex, token_type_t type);
void lexer_next_token(lexer_t *lex);
bool lexer_expect_peek(lexer_t *lex, token_type_t type);
const char *token_type_to_string(token_type_t type);

#endif
