#ifndef __XML_XPATH__
#define __XML_XPATH__

#include <stdbool.h>
#include "xml_parser.h"

#define XPATH_TYPE_TEXT 0
#define XPATH_TYPE_ATTR 1
#define XPATH_TYPE_NODE 2

#define XPATH_MAX_LEN 1000000

typedef struct XPathResult {
   XMLNode *node;     //single node
   XMLNodeList nodes; //multiple nodes
   bool isMulti;      //result is multiple node or not
   char text[5120];   //result text
}XPathResult;

 XPathResult xpath(const char *path, XMLNode *root);
 void xpath_free(XPathResult *path_result);
#endif
