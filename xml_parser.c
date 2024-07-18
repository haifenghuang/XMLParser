#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xml_lexer.h"
#include "xml_parser.h"

#define NEXT(lexer) lexer_next_token((lexer))
#define GET_CURR_TOKEN_VALUE(lexer) strndup((lexer)->cur_token.literal, (lexer)->cur_token.len)
#define EXPECT(lexer, token_type) \
  if (!lexer_expect_peek(lexer, token_type)) { \
    fprintf(stderr, "Expect next token to be '%s', got '%s')\n", token_type_to_string(token_type), token_type_to_string(lexer->peek_token.type)); \
    return false; \
}

/* read entire file, and return contents. */
static char *read_file(const char *filename) {
  FILE *fp = NULL;
  size_t size_to_read = 0;
  size_t size_read = 0;
  long pos = 0;
  char *file_contents = NULL;

  fp = fopen(filename, "r");
  if (!fp) return NULL;

  fseek(fp, 0L, SEEK_END);
  pos = ftell(fp);
  if (pos < 0) {
    fclose(fp);
    return NULL;
  }

  size_to_read = pos;
  rewind(fp);

  file_contents = (char *)malloc(sizeof(char) * (size_to_read + 1));
  if (!file_contents) {
    fclose(fp);
    return NULL;
  }

  size_read = fread(file_contents, 1, size_to_read, fp);
  if (size_read == 0 || ferror(fp)) {
    fclose(fp);
    free(file_contents);
    return NULL;
  }

  fclose(fp);
  file_contents[size_read] = '\0';
  return file_contents;
}

static void XMLAttrFree(XMLAttr *attr) {
  if (attr == NULL) return;
  if (attr->key) {
    free(attr->key);
    attr->key = NULL;
  }
  if (attr->value) {
    free(attr->value);
    attr->value = NULL;
  }
}

/* Attribute List */
void XMLAttrListInit(XMLAttrList *list) {
  list->capacity = 1;
  list->count = 0;
  list->attrs = (XMLAttr *)malloc(sizeof(XMLAttr) * list->capacity);
}

void XMLAttrListAdd(XMLAttrList *list, XMLAttr *attr) {
  while (list->count >= list->capacity) {
    list->capacity *= 2;
    list->attrs = (XMLAttr *)realloc(list->attrs, sizeof(XMLAttr) * list->capacity);
  }
  list->attrs[list->count++] = *attr;
}

XMLAttr *XMLAttrListGet(XMLAttrList *list, int index) {
  if (index < 0) index = list->count + index; // allow negative indexes
  if (index < 0 || index >= list->count) return NULL;
  return &list->attrs[index];
}

size_t XMLAttrListCount(XMLAttrList *list) {
  if (list == NULL) return 0;
  return list->count;
}

void XMLAttrListFree(XMLAttrList *list) {
  if (list == NULL) return;
  for (size_t i = 0; i < list->count; ++i) XMLAttrFree(&list->attrs[i]);
}

/* Node-List */
void XMLNodeListInit(XMLNodeList *list) {
  list->capacity = 1;
  list->count = 0;
  list->nodes = (XMLNode **)malloc(sizeof(XMLNode *) * list->capacity);
}

void XMLNodeListAdd(XMLNodeList *list, XMLNode *node) {
  while (list->count >= list->capacity) {
    list->capacity *= 2;
    list->nodes = (XMLNode **)realloc(list->nodes, sizeof(XMLNode *) * list->capacity);
  }
  list->nodes[list->count++] = node;
}

void XMLNodeListAddList(XMLNodeList *list, XMLNodeList *srcList) {
  if (srcList == NULL) return;
  for (size_t i = 0; i < srcList->count; ++i) {
    XMLNodeListAdd(list, srcList->nodes[i]);
  }
}

XMLNode *XMLNodeListGet(XMLNodeList *list, int index) {
  if (index < 0) index = list->count + index; // allow negative indexes
  if (index < 0 || index >= list->count) return NULL;
  return list->nodes[index];
}

size_t XMLNodeListCount(XMLNodeList *list) {
  if (list == NULL) return 0;
  return list->count;
}

static void XMLNodeFree(XMLNode *node); //forword declaration
void XMLNodeListFree(XMLNodeList *list) {
  if (list == NULL) return;
  for (size_t i = 0; i < list->count; ++i) {
    XMLNode *child = list->nodes[i];
    if (child != NULL) {
      XMLNodeFree(child);
      free(child);
      child = NULL;
    }
  }
}

/* XML Node */
static XMLNode *XMLNodeNew(XMLNode *parent) {
  XMLNode *node = (XMLNode *)malloc(sizeof(XMLNode));
  node->parent = parent;
  node->name = NULL;
  node->text = NULL;

  XMLAttrListInit(&node->attrList);
  XMLNodeListInit(&node->children);

  if (parent) XMLNodeListAdd(&parent->children, node);

  return node;
}

static void XMLNodeFree(XMLNode *node) {
  if (node == NULL) return;
  if (node->name) {
    free(node->name);
    node->name = NULL;
  }

  if (node->text) {
    free(node->text);
    node->text = NULL;
  }

  //Free attributes
  XMLAttrListFree(&node->attrList);

  //Free children
  XMLNodeListFree(&node->children);
}

XMLNode *XMLNodeChildrenGet(XMLNode *node, int index) {
  if (node == NULL) return NULL;
  if (index < 0) index = node->children.count + index; // allow negative indexes
  if (index < 0 || index >= node->children.count) return NULL; //index-out-of-bounds
  return node->children.nodes[index];
}

size_t XMLNodeChildrenCount(XMLNode *node) {
  if (node == NULL) return 0;
  return node->children.count;
}

static bool _XMLParseTree(lexer_t *lexer, XMLNode *node) {
  EXPECT(lexer, TOKEN_NAME);
  node->name = GET_CURR_TOKEN_VALUE(lexer);
  node->type = NT_NODE;
  NEXT(lexer);

  /* parse attributes */
  while (!lexer_cur_token_is(lexer, TOKEN_CLOSE_TAG) && !lexer_cur_token_is(lexer, TOKEN_CLOSESLASH_TAG)) {
    if (!lexer_cur_token_is(lexer, TOKEN_NAME)) return false;
    XMLAttr curr_attr =  { 0 };
    curr_attr.node = node;
    curr_attr.key = GET_CURR_TOKEN_VALUE(lexer);
    EXPECT(lexer, TOKEN_ASSIGN);
    EXPECT(lexer, TOKEN_STRING);
    curr_attr.value = GET_CURR_TOKEN_VALUE(lexer);
    XMLAttrListAdd(&node->attrList, &curr_attr);
    NEXT(lexer);
  } //end while

  if (lexer_cur_token_is(lexer, TOKEN_CLOSESLASH_TAG)) { //self contained node, no children
    NEXT(lexer);
    return true;
  }

  /* parse children */
  NEXT(lexer);
  while (!lexer_cur_token_is(lexer, TOKEN_EOF)) {
    if (lexer_cur_token_is(lexer, TOKEN_OPEN_TAG)) {
      XMLNode *child = XMLNodeNew(node);
      if (!_XMLParseTree(lexer, child)) return false;
    } else if (lexer_cur_token_is(lexer, TOKEN_OPENSLASH_TAG)) {
      EXPECT(lexer, TOKEN_NAME);
      if (strncmp(node->name, lexer->cur_token.literal, lexer->cur_token.len) != 0) {
        fprintf(stderr, "Mismatched name(%s != %.*s\n", node->name, lexer->cur_token.len, lexer->cur_token.literal);
        return false;
      }
      NEXT(lexer);
      NEXT(lexer);
      break;
    } else if (lexer_cur_token_is(lexer, TOKEN_TEXT)) {
      node->text = GET_CURR_TOKEN_VALUE(lexer);
      node->type = NT_TEXT;
      NEXT(lexer);
    } else if (lexer_cur_token_is(lexer, TOKEN_CDATA)) { /* CDATA is treated as text */
      node->text = GET_CURR_TOKEN_VALUE(lexer);
      node->type = NT_CDATA;
      NEXT(lexer);
    } else if (lexer_cur_token_is(lexer, TOKEN_COMMENT)) {
      XMLNode *child = XMLNodeNew(node);
      child->name = GET_CURR_TOKEN_VALUE(lexer);
      node->type = NT_COMMENT;
      NEXT(lexer);
    } else {
      return false;
    }
  } //end while

  return true;
}

XMLNode *XMLSelectNode(XMLNode *node, const char *node_path) {
  if (node == NULL) return NULL;
  if (node_path == NULL || node_path[0] == '\0') return node;

  XMLNode *result = node;

  int len = strlen(node_path + 2);
  char *tag = (char *)malloc(len * sizeof(char));
  if (tag == NULL) {
    fprintf(stderr, "Cannot allocate enough memory.\n");
    return NULL;
  }

  if (*node_path != '/') {
    sprintf(tag, "/%s", node_path);
  } else {
    strcpy(tag, node_path);
  }

  char *p = strtok(tag, "/");
  while (p != NULL) {
    char *p1 = strchr(p, '[');
    char *p2 = NULL;
    if (p1 != NULL) p2 = strchr(p, ']');

    if ((p2 < p1) || (p1 == NULL && p2 != NULL) || (p1 != NULL && p2 == NULL)) {
      fprintf(stderr, "Unmatched '[' and ']'.\n");
      free(tag);
      return NULL;
    }

    bool has_index = (p1 != NULL) && (p2 != NULL);
    if (has_index) {
      //char index[8] = { 0 };
      char *index = malloc((p2 - p1) * sizeof(char));
      if (index == NULL) {
        fprintf(stderr, "malloc failed\n");
        free(tag);
        return NULL;
      }
      strncpy(index, p1 + 1, p2 - p1 - 1);
      index[p2 - p1 - 1] = '\0'; //make sure it is null terminated
      int idx = atoi(index);
      if (idx < 0) idx = result->children.count + idx; // allow negative indexes
      if (idx < 0 || idx >= result->children.count) {
        fprintf(stderr, "index out of bounds\n");
        free(tag);
        free(index);
        return NULL;
      }
      free(index);

      //char tagname[128] = { 0 };
      char *tagname = malloc((p1 - p + 1) * sizeof(char));
      if (tagname == NULL) {
        fprintf(stderr, "malloc failed\n");
        free(tag);
        return NULL;
      }
      strncpy(tagname, p, p1 - p);
      tagname[p1 - p] = '\0'; //make sure it is null terminated

      XMLNode *child = result->children.nodes[idx];
      if (strncmp(child->name, tagname, strlen(tagname)) == 0) {
        result = child;
        free(tagname);
      } else {
        fprintf(stderr, "Node name '%s' not found\n", tagname);
        free(tag);
        free(tagname);
        return NULL;
      }
    } else {
     result = XMLFindFirstNode(result, p);
    }
    if (result == NULL) {
      fprintf(stderr, "Node not found\n");
      free(tag);
      return NULL;
    }
    p = strtok(NULL, "/");
  } //end while

  free(tag);
  return result;
}

XMLNode *XMLRootNode(XMLDocument *doc) {
  if (doc == NULL) return NULL;
  return doc->root->children.nodes[0];
}

XMLNode *XMLFindFirstNode(const XMLNode *node, const char *node_name) {
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, node_name, strlen(node_name)) == 0) {
      return child;
    }
  }

  return NULL;
}

XMLNodeList *XMLFindNode(const XMLNode *node, const char *node_name) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;

  XMLNodeListInit(list);
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, node_name, strlen(node_name)) == 0) {
      XMLNodeListAdd(list, child);
    }
  }

  return list;
}

XMLNodeList *XMLFindNodeWhere(const XMLNode *node, Predicate predicateFn, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;

  XMLNodeListInit(list);
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (predicateFn(child, i, user_data)) {
      XMLNodeListAdd(list, child);
    }
  }

  return list;
}

XMLNodeList *XMLFindNodeSelector(const XMLNode *node, Selector selectFn, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;

  XMLNodeListInit(list);
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    XMLNodeList *new = selectFn(child, i, user_data);
    XMLNodeListAddList(list, new);
    if (new != NULL) free(new);
  }

  return list;
}

static struct Special_Mapping {
  char ch;
  char *str;
  int str_len;
} Mapping[] = {
  {'<', "&lt;", 4},
  {'>', "&gt;", 4},
  {'"', "&quot;", 6},
  {'\'', "&apos;", 6},
  {'&', "&amp;", 5},
  {'\0', NULL, 0},
};

char *XMLDecodeText(const XMLNode *node) {
  char *d, *s;
  int i = 0;

  if (node == NULL || node->text == NULL) return "";

  char *result = node->text;
  for(d = result, s = node->text; *s; s++, d++) {
    if (strncmp(s, "<![CDATA[", 9) == 0) {
      s += 9;
    } else if (strncmp(s, "]]>", 3) == 0) {
      s += 3;
    }
    if (*s != '&') {
      if (d != s) *d = *s;
      continue;
    }

    for (i = 0; Mapping[i].ch; i++) {
      if (strncmp(s, Mapping[i].str, Mapping[i].str_len)) continue;
      *d = Mapping[i].ch;
      s += Mapping[i].str_len - 1;
      break;
    }

   if (Mapping[i].ch == '\0' && d != s) *d = *s;
  }

  *d = '\0';
  return result;
}

XMLNode *XMLNodeNextSibling(XMLNode *node)
{
  int i = 0;
  XMLNode* parent = NULL;
  
  if (node == NULL || node->parent == NULL) return NULL;
  
  parent = node->parent;
  for (i = 0; i < parent->children.count && parent->children.nodes[i] != node; i++) ;
  i++;

  return i < parent->children.count ? parent->children.nodes[i] : NULL;
}

/* XML Document */
static bool _XMLDocumentParseInternal(XMLDocument *doc, const char *xmlStr, const char *path, lexer_t *lexer) {
  XMLNodeListInit(&doc->others);

  doc->root = XMLNodeNew(NULL);
  XMLNode *curr_node = doc->root;

  if (path == NULL) {
    lexer_init(lexer, xmlStr, NULL);
  } else {
    lexer_init(lexer, xmlStr, path);
  }

  /* get next two tokens, so we have two positions */
  NEXT(lexer);
  NEXT(lexer);

  /* check for node before root */
  while (lexer_cur_token_is(lexer, TOKEN_DOCTYPE) || lexer_cur_token_is(lexer, TOKEN_COMMENT) || 
         lexer_cur_token_is(lexer, TOKEN_CDATA) || lexer_cur_token_is(lexer, TOKEN_PI)) {
    XMLNode *node = XMLNodeNew(NULL);
    token_type_t curTok = lexer_cur_token(lexer);
    switch (curTok) {
      case TOKEN_DOCTYPE: node->type = NT_DOCTYPE; break;
      case TOKEN_COMMENT: node->type = NT_COMMENT; break;
      case TOKEN_CDATA: node->type = NT_CDATA; break;
      case TOKEN_PI: node->type = NT_PI; break;
      default: break;
    } /* end switch */
    node->name = GET_CURR_TOKEN_VALUE(lexer);
    XMLNodeListAdd(&doc->others, node);
    NEXT(lexer);
  }

  // parse root node
  curr_node = XMLNodeNew(curr_node);
  if (!_XMLParseTree(lexer, curr_node)) return false;

  return lexer_cur_token_is(lexer, TOKEN_EOF);
}

bool XMLDocumentParseFile(XMLDocument *doc, const char *path) {
  lexer_t lexer = { 0 };
  char *xmlStr = doc->contents = read_file(path);
  if (xmlStr == NULL) return false;
  return _XMLDocumentParseInternal(doc, xmlStr, path, &lexer);
}

bool XMLDocumentParseStr(XMLDocument *doc, const char *xmlStr) {
  lexer_t lexer = { 0 };
  /* we need to own the string, so that in `XMLNodListFree`, we could free it */
  char *buf = doc->contents = strdup(xmlStr);
  if (buf == NULL) return false;
  return _XMLDocumentParseInternal(doc, buf, NULL, &lexer);
}

static void _XMLPrettyPrintInternal(XMLNode *node, FILE *fp, int indent_len, int times) {
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];

    //indent level
    if (times > 0) fprintf(fp, "%*s", indent_len * times, " ");

    if (child->type == NT_COMMENT) {
      fprintf(fp, "%s\n", child->name); //node name
      continue;
    } else {
      fprintf(fp, "<%s", child->name); //node name
    }

    for (size_t j = 0; j < child->attrList.count; ++j) { //node attributes
      XMLAttr attr = child->attrList.attrs[j];
      if (!attr.value || !strcmp(attr.value, "")) continue;
      fprintf(fp, " %s = \"%s\"", attr.key, attr.value);
    }

    if (child->children.count == 0 && !child->text && (child->type != NT_COMMENT)) {
      fprintf(fp, " />\n");
    } else {
      fprintf(fp, ">");
      if (child->children.count == 0) {
        fprintf(fp, "%s</%s>\n", child->text, child->name);
      } else {
        if (child->text) fprintf(fp, "%s", child->text);
        fprintf(fp, "\n");
        _XMLPrettyPrintInternal(child, fp, indent_len, times + 1);
        if (times > 0) fprintf(fp, "%*s", indent_len * times, " ");
        fprintf(fp, "</%s>\n", child->name);
      }
    }
  } //end for 
}

bool XMLPrettyPrint(XMLDocument *doc, FILE *fp, int indent_len) {
  if (fp == NULL) fp = stdout;

  /* print nodes before root */
  for (size_t i = 0; i < doc->others.count; ++i) {
    XMLNode *other = doc->others.nodes[i];
      fprintf(fp, "%s\n", other->name); //node name
  }

  _XMLPrettyPrintInternal(doc->root, fp, indent_len, 0);
}

void XMLDocumentFree(XMLDocument *doc) {
  if (doc == NULL) return;
  if (doc->contents) {
    free(doc->contents);
    doc->contents = NULL;
  }

  //Free others node(s) before root
  XMLNodeListFree(&doc->others);

  if (doc->root) {
    XMLNodeFree(doc->root);
    doc->root = NULL;
  }
}

