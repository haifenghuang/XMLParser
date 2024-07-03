#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml_lexer.h"

static src_pos_t src_pos_make(const char *file, int line, int column) {
  src_pos_t pos;
  pos.file = file;
  pos.line = line;
  pos.column = column;
  return pos;
}

static const char *token_type_names[] = {
  "NONE",
  "EOF",
  "<",
  ">",
  "</",
  "/>",
  "<?",
  "?>",
  "NAME",
  "ASSIGN",
  "STRING",
  "TEXT"
};

static void token_init(token_t *tok, token_type_t type, const char *literal, int len) {
  tok->type = type;
  tok->literal = literal;
  tok->len = len;
}

/* mainly for debug */
const char *token_type_to_string(token_type_t type) {
  return token_type_names[type];
}

#ifdef DEBUG
/* print token contents(debug use) */
void token_dump(token_t tok) {
  printf("TOKEN :[TYPE: %s], [Literal:%.*s], [Len:%d], [Pos.line:%d], [Pos.column:%d\n",
         token_type_to_string(tok.type), tok.len, tok.literal,
         tok.len, tok.pos.line, tok.pos.column);
}
#endif

static bool is_letter(char ch) {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

static void read_char(lexer_t *lex) {
  if (lex->next_position >= lex->input_len) {
    lex->ch = '\0';
  } else {
    lex->ch = lex->input[lex->next_position];
  }

  lex->position = lex->next_position;
  lex->next_position++;

  if (lex->ch == '\n') {
    lex->line++;
    lex->column = 0;
  } else {
    lex->column++;
  }
}

static char peek_char(lexer_t *lex) {
  if (lex->next_position >= lex->input_len) return '\0';
  return lex->input[lex->next_position];
}

static char peek_nchar(lexer_t *lex, int n) {
  if (lex->next_position + n >= lex->input_len) return '\0';
  return lex->input[lex->next_position + n];
}

static const char *read_identifier(lexer_t *lex, int *out_len) {
  int position = lex->position;
  int len = 0;
  while (is_digit(lex->ch) || is_letter(lex->ch) || lex->ch == ':' || lex->ch == '-') read_char(lex);

  len = lex->position - position;
  *out_len = len;
  return lex->input + position;
}

static const char *read_text(lexer_t *lex, int *out_len) {
  int position = lex->position;
  int len = 0;
  while (lex->ch != '<') read_char(lex);

  len = lex->position - position;
  *out_len = len;
  return lex->input + position;
}

static const char *read_string(lexer_t *lex, int *out_len) {
  int position = lex->position;
  int len = 0;
  read_char(lex);
  while (lex->ch != '"' && lex->ch != '\'') read_char(lex);
  read_char(lex);

  len = lex->position - position;
  *out_len = len;
  return lex->input + position;
}

static const char *read_comment(lexer_t *lex, int *out_len) {
  int position = lex->position;
  int len = 0;

  while (lex->ch != '\0') {
    if (!strncmp(lex->input + lex->position, "-->", 3)) {
      read_char(lex);
      read_char(lex);
      read_char(lex);
      break;
    }
  }
  read_char(lex);

  len = lex->position - position;
  *out_len = len;
  return lex->input + position;
}

static void skip_whitespace(lexer_t *lex) {
  char ch = lex->ch;
  while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
    read_char(lex);
    ch = lex->ch;
  }
}

bool lexer_init(lexer_t *lex, const char *input, const char *filename) {
  lex->input = input;
  lex->input_len = (int)strlen(input);
  lex->position = 0;
  lex->next_position = 0;
  lex->ch = '\0';
  lex->line = 1;
  lex->column = 0;
  lex->file = filename;
  lex->inTag = false;

  read_char(lex);
  token_init(&lex->cur_token, TOKEN_NONE, NULL, 0);
  token_init(&lex->peek_token, TOKEN_NONE, NULL, 0);

  return true;
}

bool lexer_cur_token_is(lexer_t *lex, token_type_t type) {
  return lex->cur_token.type == type;
}

bool lexer_peek_token_is(lexer_t *lex, token_type_t type) {
  return lex->peek_token.type == type;
}

bool lexer_expect_peek(lexer_t *lex, token_type_t type) {
  if (lex->peek_token.type != type) return false;
  lexer_next_token(lex);
  return true;
}

static token_t lexer_next_token_internal(lexer_t *lex) {
  while (true) {
    skip_whitespace(lex);

    token_t out_tok;
    out_tok.type = TOKEN_NONE;
    out_tok.literal = lex->input + lex->position;
    out_tok.len = 1;
    out_tok.pos = src_pos_make(lex->file, lex->line, lex->column);

    char c = lex->ch;
    /* check current char */
    switch (c) {
      case '\0': token_init(&out_tok, TOKEN_EOF, "EOF", 3); break;
      case '>': token_init(&out_tok, TOKEN_CLOSE_TAG, ">", 1); lex->inTag = false; break;
      case '=': token_init(&out_tok, TOKEN_ASSIGN, "=", 1); break;
      case '<': {
        if (peek_char(lex) == '/') {
          token_init(&out_tok, TOKEN_OPENSLASH_TAG, "</", 2);
          read_char(lex);
        } else if (peek_char(lex) == '?') {
          token_init(&out_tok, TOKEN_OPEN_HEADER, "<?", 2);
          read_char(lex);
        } else if ((peek_nchar(lex, 0) == '!') && (peek_nchar(lex, 1) == '-') && (peek_nchar(lex, 1) == '-')) {
          int str_len = 0;
          const char *str = read_comment(lex, &str_len);
          token_init(&out_tok, TOKEN_COMMENT, str, str_len);
          return out_tok;
        } else {
          token_init(&out_tok, TOKEN_OPEN_TAG, "<", 1);
        }
        lex->inTag = true;
      }
      break;

      case '/': {
        if (peek_char(lex) == '>') {
          token_init(&out_tok, TOKEN_CLOSESLASH_TAG, "/>", 2);
          read_char(lex);
          lex->inTag = false;
        }
      }
      break;
      case '?': {
        if (peek_char(lex) == '>') {
          token_init(&out_tok, TOKEN_CLOSE_HEADER, "?>", 2);
          read_char(lex);
          lex->inTag = false;
        }
      }
      break;
      default:
        if (!lex->inTag) {
          int text_len = 0;
          const char *text = read_text(lex, &text_len);
          token_init(&out_tok, TOKEN_TEXT, text, text_len);
          return out_tok;
        }

        if (is_letter(lex->ch)) {
          int ident_len = 0;
          const char *ident = read_identifier(lex, &ident_len);
          token_init(&out_tok, TOKEN_NAME, ident, ident_len);
          return out_tok;
        } else if (lex->ch == '"' || lex->ch == '\'') {
          int str_len = 0;
          const char *str = read_string(lex, &str_len);
          token_init(&out_tok, TOKEN_STRING, str, str_len);
          return out_tok;
        }
        break;
    } /* end switch */

    read_char(lex);
    return out_tok;
  } /* end while */
}

void lexer_next_token(lexer_t *lex) {
  lex->cur_token = lex->peek_token;
  lex->peek_token = lexer_next_token_internal(lex);
}


