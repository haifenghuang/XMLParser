#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct XMLAttr {
  char *key;
  char *value;
}XMLAttr;

typedef struct XMLAttrList {
  size_t count;
  size_t capacity;
  XMLAttr *attrs;
}XMLAttrList;

typedef struct XMLNodeList {
  size_t count;
  size_t capacity;
  struct XMLNode **nodes;
}XMLNodeList;

typedef struct XMLNode {
  char *name;
  char *text;
  struct XMLNode *parent;
  XMLAttrList attrList;
  XMLNodeList children;
  bool isComment; /* is this node a comment node? */
}XMLNode;

typedef struct XMLDocument {
  char *contents;
  XMLNodeList others; /* other nodes before root(only support DOCTYPE and comment nodes) */
  XMLNode *root;
  char *version;
  char *encoding;
}XMLDocument;

typedef bool (*Predicate)(XMLNode *node, int idx, void *user_data);
typedef XMLNodeList *(*Selector)(XMLNode *node, int idx, void *user_data);

/* XML AttributeList */
void XMLAttrListInit(XMLAttrList *list);
void XMLAttrListAdd(XMLAttrList *list, XMLAttr *attr);
XMLAttr* XMLAttrListGet(XMLAttrList *list, int index);
size_t XMLAttrListCount(XMLAttrList *list);
void XMLAttrListFree(XMLAttrList *list);

/* XML NodeList */
void XMLNodeListInit(XMLNodeList *list);
void XMLNodeListAdd(XMLNodeList *list, XMLNode *node);
void XMLNodeListAddList(XMLNodeList *list, XMLNodeList *srcList);
XMLNode *XMLNodeListGet(XMLNodeList *list, int index);
size_t XMLNodeListCount(XMLNodeList *list);
void XMLNodeListFree(XMLNodeList *list);

/* XML Node */
/* Get children node at index */
XMLNode *XMLNodeChildrenGet(XMLNode *node, int index);
/* Get children count */
size_t XMLNodeChildrenCount(XMLNode *node);

/* Select node which satisfy node_path.
 * Note: 'node_path' doesn't support attribute, only Node.
 * */
XMLNode *XMLSelectNode(XMLNode *node, const char *node_path);

#define XML_ROOT(doc) (doc)->root->children.nodes[0]
XMLNode *XMLRootNode(XMLDocument *doc);

/* Find first node of `node` with name `node_name` */
XMLNode *XMLFindFirstNode(const XMLNode *node, const char *node_name);

/* Find all child node of `node` with `node_name`.
 * Note: you must free the returned XMLNodeList if the result is not NULL;
 * */
XMLNodeList *XMLFindNode(const XMLNode *node, const char *node_name);

/* Fina all child node of `node` which satify `Predicate`.
 * Note: you must free the returned XMLNodeList if the result is not NULL;
 * */
XMLNodeList *XMLFindNodeWhere(const XMLNode *node, Predicate predicateFn, void *user_data);

/* Find all child node of `node` which satify `Selector`.
 * Note: you must free the returned XMLNodeList if the result is not NULL;
 * */
XMLNodeList *XMLFindNodeSelector(const XMLNode *node, Selector selectFn, void *user_data);

/* decode xml node text */
char *XMLDecodeText(const XMLNode *node);

/* XML Document */
bool XMLDocumentParseFile(XMLDocument *doc, const char *path);
bool XMLDocumentParseStr(XMLDocument *doc, const char *xmlStr);
bool XMLPrettyPrint(XMLDocument *doc, FILE *fp, int ident_len);
void XMLDocumentFree(XMLDocument *doc);

#endif
