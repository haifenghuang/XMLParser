#include "xml_lexer.h"
#include "xml_parser.h"

#ifdef LEX_DEBUG
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
#endif

static bool NodeNameIsField(XMLNode *node, int idx, void *user_data) {
  return strncmp(node->name, "field", 5) == 0;
}

static XMLNodeList* AgeGreaterEqualThan48(XMLNode *node, int idx, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;
  XMLNodeListInit(list);

  char *age = (char *)user_data;
  //for (size_t i = 0; i < node->children.count; ++i) {
  for (size_t i = 0; i < XMLNodeChildrenCount(node); ++i) { //same as above 'for'
    //XMLNode *child = node->children.nodes[i];
    XMLNode *child = XMLNodeChildrenGet(node, i); //same as above
    if (strncmp(child->name, age, strlen(age)) == 0) {
      if (atoi(child->text) >= 48) {
        XMLNodeListAdd(list, child);
      }
    }
  }
  return list;
}

/* Select 'food' node which price is greater than 5 */
static XMLNodeList *PriceGreaterThanFiveFood(XMLNode *node, int idx, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;
  XMLNodeListInit(list);

  char *price = (char *)user_data;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, price, strlen(price)) == 0) {
      if (atof(child->text+1) >= 5.0) { //+1: skip the '$'
        XMLNodeListAdd(list, child->parent);
      }
    }
  }
  return list;
}

/* Testing for 'simple.xml' file */
void Test_simplexml(XMLDocument *doc) {
  fprintf(stdout, "\n============Search Node with Selector ============\n");
  XMLNodeList *list = XMLFindNodeSelector(XML_ROOT(doc), PriceGreaterThanFiveFood, (void *)"price");
  if (list != NULL) {
    fprintf(stdout, "food count=[%ld]\n", list->count);
    for (size_t i = 0; i < list->count; ++i) {
      XMLNode *food_node = list->nodes[i];
      fprintf(stdout, "MATCHES [%ld]:\n", i);
      for (size_t j = 0; j < food_node->children.count; ++j) {
        XMLNode *child = food_node->children.nodes[j];
        if (child->isComment) continue;
        fprintf(stdout, "\t %s: %s\n", child->name, XMLDecodeText(child));
      }
    }

    free(list);
  }
}

/* Testing for 'test2.xml' file */
void Test_test2xml(XMLDocument *doc) {
  XMLNode *root = XML_ROOT(doc);
  XMLNode *n2 = XMLSelectNode(root, "h:tr[2]/h:td[1]");
  if (n2 != NULL) printf("n2.text=%s\n", n2->text);

  XMLNode *n3 = XMLSelectNode(root, "h:tr[-1]/h:td[1]");
  if (n3 != NULL) printf("n3.text=%s\n", n3->text); //should be the same result with above code.
}

/* Testing 'test.xml' file */
void Test_testxml(XMLDocument *doc) {
  XMLNode *root = XML_ROOT(doc);

  fprintf(stdout, "\n============Search Node============\n");
  XMLNode *m = XMLSelectNode(root, "fields");
  if (m) {
    XMLNodeList *list = XMLFindNode(m, "field");
    if (list) {
      fprintf(stdout, "list count=[%ld]\n", list->count);
      for (size_t i = 0; i < list->count; ++i) {
        XMLNode *node = list->nodes[i];
        for (size_t j = 0; j < node->attrList.count; ++j) {
          XMLAttr attr = node->attrList.attrs[j];
          fprintf(stdout, "\t%s => %s\n", attr.key, attr.value);
        }
     }
     free(list);
    }
  }

  fprintf(stdout, "\n============Search Node with Predicate============\n");
  XMLNodeList *list = XMLFindNodeWhere(m, NodeNameIsField, NULL);
  if (list) {
      fprintf(stdout, "list count=[%ld]\n", list->count);
      for (size_t i = 0; i < list->count; ++i) {
        XMLNode *node = list->nodes[i];
        for (size_t j = 0; j < node->attrList.count; ++j) {
          XMLAttr attr = node->attrList.attrs[j];
          fprintf(stdout, "\t%s => %s\n", attr.key, attr.value);
        }
      }
      free(list);
  }

  fprintf(stdout, "\n============Search Node with Selector============\n");
  XMLNode *m2 = XMLSelectNode(root, "fields");
  if (m) {
    XMLNodeList *list = XMLFindNodeSelector(m2, AgeGreaterEqualThan48, (void*)"age");
    if (list) {
      fprintf(stdout, "list count=[%ld]\n", list->count);
      for (size_t i = 0; i < list->count; ++i) {
        XMLNode *node = list->nodes[i];
        fprintf(stdout, "name: %s, age: %s\n", node->name, node->text);
      }
      free(list);
    }
  }

  fprintf(stdout, "\n============XMLSelectNode============\n");
  XMLNode *n = XMLSelectNode(root, "fields/field/name");
  if (n != NULL) printf("n.text=%s\n", n->text);

  XMLNode *n2 = XMLSelectNode(root, "fields/field[1]/age");
  if (n != NULL) printf("n2.text=%s\n", n2->text);

}

static XMLNodeList* SelectAllPricesNodes(XMLNode *node, int idx, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;
  XMLNodeListInit(list);

  char *price = (char *)user_data;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, price, strlen(price)) == 0) {
      XMLNodeListAdd(list, child);
    }
  }
  return list;
}

static XMLNodeList* SelectPriceGreaterThan35(XMLNode *node, int idx, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;
  XMLNodeListInit(list);

  char *price = (char *)user_data;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, price, strlen(price)) == 0) {
      if (atof(child->text) > 35.0) XMLNodeListAdd(list, child);
    }
  }
  return list;
}

static XMLNodeList* SelectTitleWithPriceGreaterThan35(XMLNode *node, int idx, void *user_data) {
  XMLNodeList *list = malloc(sizeof(XMLNodeList));
  if (list == NULL) return NULL;
  XMLNodeListInit(list);

  char *price = (char *)user_data;
  XMLNode *title = NULL;
  for (size_t i = 0; i < node->children.count; ++i) {
    XMLNode *child = node->children.nodes[i];
    if (strncmp(child->name, "title", 5) == 0) {
      title = child;
      continue;
    }
    if (strncmp(child->name, price, strlen(price)) == 0) {
      if (atof(child->text) > 35.0) {
        XMLNodeListAdd(list, title);
      }
    }
  }
  return list;
}

static void Test_test4xml(XMLDocument *doc) {
  XMLNode *root = XML_ROOT(doc);

  /* Select the title of the first book */
  fprintf(stdout, "\n============Select the title of the first book============\n");
  XMLNode *n1 = XMLSelectNode(root, "book[0]/title");
  if (n1) {
    printf("title=%s\n", n1->text);
  }

  fprintf(stdout, "\n============Select all the prices============\n");
  XMLNodeList *list = XMLFindNodeSelector(root, SelectAllPricesNodes, (void *)"price");
  if (list != NULL) {
    fprintf(stdout, "prices count=[%ld]\n", list->count);
    for (size_t i = 0; i < list->count; ++i) {
      XMLNode *price = list->nodes[i];
      fprintf(stdout, "%s\n", price->text);
    }
    free(list);
  }

  fprintf(stdout, "\n============Select price nodes with price>35============\n");
  XMLNodeList *list2 = XMLFindNodeSelector(root, SelectPriceGreaterThan35, (void *)"price");
  if (list2 != NULL) {
    fprintf(stdout, "prices greater than 35 count=[%ld]\n", list2->count);
    for (size_t i = 0; i < list2->count; ++i) {
      XMLNode *price = list2->nodes[i];
      fprintf(stdout, "%s\n", price->text);
    }
    free(list2);
  }

  fprintf(stdout, "\n============Select title nodes with price>35============\n");
  XMLNodeList *list3 = XMLFindNodeSelector(root, SelectTitleWithPriceGreaterThan35, (void *)"price");
  if (list3 != NULL) {
    fprintf(stdout, "title nodes with prices greater than 35 count=[%ld]\n", list3->count);
    for (size_t i = 0; i < list3->count; ++i) {
      XMLNode *title = list3->nodes[i];
      fprintf(stdout, "%s\n", title->text);
    }
    free(list3);
  }
}

static void Test_cdataxml(XMLDocument *doc) {
  XMLNode *root = XML_ROOT(doc);
  XMLNode *m = XMLSelectNode(root, "description");
  if (m) {
    fprintf(stdout, "description=%s\n", XMLDecodeText(m));
  }
}

int main(int argc, char **argv) {
  char *filename = "./test.xml";
#ifdef LEX_DEBUG
  conents = read_file(filename);
  lexer_t lexer;
  lexer_init(&lexer, contents, filename);

  /* get next two tokens, so we have two positions */
  lexer_next_token(&lexer);
  lexer_next_token(&lexer);
  while (!lexer_cur_token_is(&lexer, TOKEN_EOF)) {
#ifdef DEBUG
    token_dump(lexer.cur_token);
#endif
    lexer_next_token(&lexer);
  } /* end while */

#else
  if (argc == 2) filename = argv[1];
#endif

  /* open xml file & parser it */
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, filename);
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  fprintf(stdout, "\n============Pretty Print============\n");
  XMLPrettyPrint(&doc, NULL, 4); /* output to console */

  /* Output to file */
  //FILE *out = fopen("./log.txt", "w");
  //XMLPrettyPrint(&doc, out, 4);
  //fclose(out);

  if (strcmp(filename, "./test.xml") == 0) {
    Test_testxml(&doc);
  } else if (strcmp(filename, "./simple.xml") == 0) {
    Test_simplexml(&doc);
  } else if (strcmp(filename, "./test2.xml") == 0) {
    Test_test2xml(&doc);
  } else if (strcmp(filename, "./test4.xml") == 0) {
    Test_test4xml(&doc);
  } else if (strcmp(filename, "./cdata.xml") == 0) {
    Test_cdataxml(&doc);
  }

  XMLDocumentFree(&doc);
  return 0;
}
