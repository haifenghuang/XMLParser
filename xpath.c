#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "xpath.h"

typedef enum Action {
    SELECT_PARENT,                     // /..                select parent node
    SELECT_THIS,                       // /.                 select current node
    SELECT_NODE_ALL_CHILD,             // //name             select all the children nodes which match 'name
    SELECT_NODE_FIRST_CHILD,           // /name              select the first child node which matches 'name'
    SELECT_NODE_BY_ARRAY_AND_NAME,     // /name[n]           select the nth node of 'name' node.
    SELECT_NODE_BY_ATTR_AND_NAME,      // /name[@attr]       select 'name' node which attribute matches 'attr'
    SELECT_NODE_BY_ATTRVALUE_AND_NAME, // /name[@attr=value] select 'name' which 'attr' = 'value'
    SELECT_TEXT,                       // /text()            select current node's text
    SELECT_TEXTS_FROM_CHILD,           // //text()           select current node's and child node's text
    SELECT_ATTR,                       // /@attr             select attr of current node
}Action;

typedef struct _pair {
  Action first;
  char second[256];
}pair_t;

typedef struct _optionList {
  size_t count;
  size_t capacity;
  pair_t **pairs;
}optionList;

static void optionListInit(optionList *list) {
  list->capacity = 1;
  list->count = 0;
  list->pairs = (pair_t **)malloc(sizeof(pair_t *) * list->capacity);
}

static void optionListAdd(optionList *list, pair_t *pair) {
  while (list->count >= list->capacity) {
    list->capacity *= 2;
    list->pairs = (pair_t **)realloc(list->pairs, sizeof(pair_t *) * list->capacity);
  }
  list->pairs[list->count++] = pair;
}

static pair_t *optionListGet(optionList *list, int index) {
  if (index < 0) index = list->count + index; // allow negative indexes
  if (index < 0 || index >= list->count) return NULL;
  return list->pairs[index];
}

static size_t optionListCount(optionList *list) {
  if (list == NULL) return 0;
  return list->count;
}

static void optionListFree(optionList *list) {
  if (list == NULL) return;
  for (size_t i = 0; i < list->count; ++i) {
    pair_t *pair = list->pairs[i];
    if (pair != NULL) {
      free(pair);
      pair = NULL;
    }
  }

  if (list->pairs) {
    free(list->pairs);
    list->pairs = NULL;
  }
}

/* for debugging */
static const char *action_to_str(Action action) {
 switch (action) {
   case SELECT_PARENT:
     return "select_parent_node";
     break;
   case SELECT_THIS:
     return "select_this_node";
     break;
   case SELECT_NODE_ALL_CHILD:
     return "select_node_all_child";
     break;
   case SELECT_NODE_FIRST_CHILD:
     return "select_node_first_child";
     break;
   case SELECT_NODE_BY_ARRAY_AND_NAME:
     return "select_node_by_array_and_name";
     break;
   case SELECT_NODE_BY_ATTR_AND_NAME:
     return "select_node_by_attr_and_name";
     break;
   case SELECT_NODE_BY_ATTRVALUE_AND_NAME:
     return "select_node_by_attrvalue_and_name";
     break;
   case SELECT_TEXT:
     return "select_text";
     break;
   case SELECT_TEXTS_FROM_CHILD:
     return "select_texts_from_child";
     break;
   case SELECT_ATTR:
     return "select_attr";
     break;
   default:
     return "Unsupported option";
  }
}

/* //text() */
static void xpath_select_texts_from_child(XMLNode *node, char *out) {
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (child->text == NULL) continue;
    strcat(out, child->text);
    strcat(out, " ");
  }
}

/* /name[@attr=value] */
static XMLNode *xpath_select_node_by_attrValue_and_name(const char* name, XMLNode *node) {
  char *at_idx = NULL;
  char *equal_idx = NULL;
  char *l_idx = NULL;
  char *r_idx = NULL;
   
  char attr_name[64] = { 0 };
  char value_name[128] = { 0 };
  char node_name[64] = { 0 };

  at_idx = strstr(name, "@");
  equal_idx = strstr(name, "=");
  l_idx = strstr(name, "[");
  r_idx = strstr(name, "]");
  
  strncpy(attr_name, at_idx + 1, equal_idx - at_idx - 1);
  strncpy(value_name, equal_idx + 1, r_idx - equal_idx - 1);
  strncpy(node_name, name, l_idx-name);

  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strcmp(child->name, node_name) == 0) {
      for (size_t j = 0; j < child->attrList.count; ++j) {
        XMLAttr attr = child->attrList.attrs[j];
        if (strcmp(attr.key, attr_name) == 0 && strcmp(attr.value, value_name) == 0) return child;
      }
    }
  }
  return NULL;
}
/* /name[@attr] */
static XMLNode *xpath_select_node_by_attr_and_name(const char *name, XMLNode *node) {
  char *at_idx = NULL;
  char *l_idx = NULL;
  char *r_idx = NULL;
   
  char attr_name[64] = { 0 };
  char node_name[64] = { 0 };

  at_idx = strstr(name, "@");
  l_idx = strstr(name, "[");
  r_idx = strstr(name, "]");
  
  strncpy(attr_name, at_idx + 1, r_idx - at_idx - 1);
  strncpy(node_name, name, l_idx-name);

  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strcmp(child->name, node_name) == 0) {
      for (size_t j = 0; j < child->attrList.count; ++j) {
        XMLAttr attr = child->attrList.attrs[j];
        if (strcmp(attr.key, attr_name) == 0) return child;
      }
    }
  }

  return NULL;
}

/* /name[n] */
static XMLNode *xpath_select_node_by_array_and_name(const char *name, XMLNode *node) {
  char *l_idx = NULL;
  char *r_idx = NULL;
   
  char str_idx[64] = { 0 };
  char attr_name[64] = { 0 };

  l_idx = strstr(name, "[");
  r_idx = strstr(name, "]");
  
  strncpy(str_idx, l_idx + 1, r_idx-l_idx);
  int j = atoi(str_idx);
  strncpy(attr_name, name, l_idx-name);

  int i = 0;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strcmp(child->name, attr_name) == 0) {
      i++;
      if (i == j) return child;
    }
  }
  return NULL;
}
/* /name */
static XMLNode *xpath_select_node_first_child(const char *name, XMLNode *node) {
  if (strcmp(node->name, name) == 0) return node;
  if (node->children.count == 0) return NULL;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strcmp(child->name, name) == 0) return child;
  }
  return NULL;
}

/* /@attr */
static void xpath_select_attr_from_this(const char *name, XMLNode *node, char *out) {
  char *at_idx = NULL;
  char attr_name[64] = { 0 };

  at_idx = strstr(name, "@");
  strcpy(attr_name, at_idx + 1);
  for (size_t i = 0; i < node->attrList.count; ++i) {
    XMLAttr attr = node->attrList.attrs[i];
    if (strcmp(attr.key, name) == 0) {
      strncpy(out, attr.value, strlen(attr.value));
      return;
    }
  }
}
/* //name */
static void xpath_select_node_all_child(const char *name, XMLNode *node, XMLNodeList *list)
{
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strcmp(child->name, name) == 0) {
      XMLNodeListAdd(list, child);
    }
  }
}

static pair_t* parse_sub_path(const char *op) {
  pair_t *ret = malloc(sizeof(pair_t));
  if (strstr(op, "//") != NULL) {
    if (strstr(op, "text()") != NULL) {
      ret->first = SELECT_TEXTS_FROM_CHILD;
      strcpy(ret->second, op+2);
    } else {
      ret->first = SELECT_NODE_ALL_CHILD;
      strcpy(ret->second, op+2);
    }
  } else {
    //取子代
    if (strstr(op, "..") != NULL) {
      ret->first = SELECT_PARENT;
      strcpy(ret->second, "");
    } else if (strstr(op, ".") != NULL) {
      ret->first = SELECT_THIS;
      strcpy(ret->second, "");
    } else if (strstr(op, "[") != NULL) {
      if (strstr(op, "@") != NULL) {
        if (strstr(op, "=") != NULL) {
          ret->first = SELECT_NODE_BY_ATTRVALUE_AND_NAME;
          strcpy(ret->second, op+1);
        } else {
          ret->first = SELECT_NODE_BY_ATTR_AND_NAME;
          strcpy(ret->second, op+1);
        }
      } else {
        ret->first = SELECT_NODE_BY_ARRAY_AND_NAME;
        strcpy(ret->second, op+1);
      }
    } else if (strstr(op, "@") != NULL) {
      ret->first = SELECT_ATTR;
      strcpy(ret->second, op+1);
    } else if (strstr(op, "text()") != NULL) {
      ret->first = SELECT_TEXT;
      strcpy(ret->second, op+1);
    } else {
      ret->first = SELECT_NODE_FIRST_CHILD;
      strcpy(ret->second, op+1);
    }
  }
  //printf("%s %s\n", action_to_str(ret->first), ret->second);
  return ret;
}

static void parse_path(optionList *options, const char *exp) {
  int l = 0, r = 0;
  int len = 0;

  while (len <= strlen(exp)) {
    if ((exp[len] == '/')) {
      if (exp[len + 1] == '/') r = l + 2;
      else                     r = l + 1;
      while (r <= strlen(exp)) {
        if (exp[r] == '/') break;
        r++;
      }
      char sub_option[256] = { 0 };
      strncpy(sub_option, exp+l, r-l);
      optionListAdd(options, parse_sub_path(sub_option));
    }
    len = r;
    l = r;
  }
}

static bool execute(optionList *options, XMLNode *node, XPathResult *ret) {
  XMLNode *n = node;
  for (size_t i = 0; i < options->count; ++i) {
    pair_t *pair = options->pairs[i];
    Action action = pair->first;
    char *name = pair->second;
    //printf("first = [%s], second=[%s]\n", action_to_str(option), name);

    switch (action) {
      case SELECT_PARENT:
        n = n->parent;
        ret->node = n;
	ret->isMulti = false;
        break;
      case SELECT_THIS:
	ret->isMulti = false;
        break;
      case SELECT_NODE_ALL_CHILD:
        xpath_select_node_all_child(name, n, &ret->nodes);
	ret->isMulti = true;
	break;
      case SELECT_NODE_FIRST_CHILD:
        n = xpath_select_node_first_child(name, n);
	if (n == NULL) return false;
	ret->isMulti = false;
        ret->node = n;
        break;
      case SELECT_NODE_BY_ARRAY_AND_NAME:
        n = xpath_select_node_by_array_and_name(name, n);
	if (n == NULL) return false;
	ret->isMulti = false;
        ret->node = n;
        break;
      case SELECT_NODE_BY_ATTR_AND_NAME:
        n = xpath_select_node_by_attr_and_name(name, n);
	if (n == NULL) return false;
	ret->isMulti = false;
        ret->node = n;
        break;
      case SELECT_NODE_BY_ATTRVALUE_AND_NAME:
        n = xpath_select_node_by_attrValue_and_name(name, n);
	if (n == NULL) return false;
	ret->isMulti = false;
        ret->node = n;
        break;
      case SELECT_TEXT:
        strcpy(ret->text, n->text);
	ret->isMulti = false;
        return true;
      case SELECT_TEXTS_FROM_CHILD:
        xpath_select_texts_from_child(n, ret->text);
	ret->isMulti = false;
        return true;
      case SELECT_ATTR:
        xpath_select_attr_from_this(name, n, ret->text);
	ret->isMulti = false;
        return true;
      default:
        return false;
     }
  }
  return true;
}

XPathResult xpath(const char *path, XMLNode *node) {
  int len = 0;
  //char *_path = NULL;
  optionList options = { 0 };
  XPathResult ret = { 0 };

  XMLNodeListInit(&ret.nodes);
  optionListInit(&options);

  parse_path(&options, path);

  bool result = execute(&options, node, &ret);
  if (!result) memset(&ret, 0x00, sizeof(ret));

  optionListFree(&options);

  return ret;
}

void xpath_free(XPathResult *path_result) {
  if (path_result == NULL) return;
  if (path_result->nodes.nodes) {
    free(path_result->nodes.nodes);
    path_result->nodes.nodes = NULL;
  }
}
