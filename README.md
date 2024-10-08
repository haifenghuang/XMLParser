# A simple XML parser for C
XMLParser is a simple XML parser for the C language. It is intended to be simple to understand and easy to use. Although the parser has some limitations, but it can handle
most of the standard xml files. And it support part of xpath syntax,
it also has an easy to understand(maybe more elegent) way for selecting/filtering the xml node(though not powerful than xpath).

Chinese version: [中文](README_cn.md)

## Features
- Simple
- Easy to use and understand
- Pretty printing XML
- Node Filtering & Selecting
- Simple XPath support


## Limitations
- ~~Do not support CDATA~~
- ~~Do not support DOCTYPE~~(Only parsing, no validation)
- Do not support Unicode(Only support UTF-8)

## About CMake
This is my first time trying to use `CMake` in a project. So I changed the origin `makefile` to `makefile.old`,
and added `CMakeLists.txt` file. To use `CMake` to build the project, please use below commands:

```sh
  cmake -S . -B build
  cmake --build build/
  ctest --test-dir build   # if you want to run test
  cd build && ./xml_parser # simple run the command
```

## Usage Examples

### Pretty printing xml file
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test3.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLPrettyPrint(&doc, NULL, 4); /* output to console */

  XMLDocumentFree(&doc);
  return 0;
}
```

Result:
```
# before
<?xml version="1.0" encoding="UTF-8" ?><bookstore><book><title lang="en">Harry Potter</title><author>J K. Rowling</author><year>2005</year><price>29.99</price></book></bookstore>

# after
<?xml version="1.0" encoding="UTF-8" ?>
<bookstore>
    <book>
        <title lang="en">Harry Potter</title>
        <author>J K. Rowling</author>
        <year>2005</year>
        <price>29.99</price>
    </book>
</bookstore>
```

### Select specific node(XMLSelectNode)
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLNode *root = XML_ROOT(&doc);
  XMLNode *n = XMLSelectNode(root, "/struct/fields/field[2]/age"); //select <fields> node's second <field>'s <age> child node
  if (n != NULL) printf("n.text=%s\n", n->text);

  XMLDocumentFree(&doc);
  return 0;
}
```

### Select specific node with name(XMLSelectNode & XMLFindNode)
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test4.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  /* select the third book node of root */
  XMLNode *book = XMLSelectNode(XML_ROOT(&doc), "/bookstore/book[3]");

  /* find all the "author" node of third book node */
  XMLNodeList *authorList = XMLFindNode(book, "author");
  if (authorList != NULL) {
    for (size_t i = 0; i < authorList->count; ++i) {
      XMLNode *author = authorList->nodes[i];
      fprintf(stdout, "%ld: %s\n", author->index, author->text);
    }
    free(authorList);
  }

  XMLDocumentFree(&doc);
  return 0;
}
```

Result:
```
# test4.xml:
<bookstore>
    <book>...</book>
    <book>...</book>
    <book category = "web">
        <title lang = "en">XQuery Kick Start</title>
        <author>James McGovern</author>
        <author>Per Bothner</author>
        <author>Kurt Cagle</author>
        <author>James Linn</author>
        <author>Vaidyanathan Nagarajan</author>
        <year>2003</year>
        <price>49.99</price>
    </book>
</bookstore>

# Run result:
1： James McGovern
2： Per Bothner
3： Kurt Cagle
4： James Linn
5： Vaidyanathan Nagarajan
```

### Select specific node with callback(XMLFindNodeSelector)
```c
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

int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./simple.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLNodeList *list = XMLFindNodeSelector(XML_ROOT(&doc), PriceGreaterThanFiveFood, (void *)"price");
  if (list != NULL) {
    fprintf(stdout, "food count=[%ld]\n", list->count);
    for (size_t i = 0; i < list->count; ++i) {
      XMLNode *food_node = list->nodes[i];
      fprintf(stdout, "MATCHES [%ld]:\n", i);
      for (size_t j = 0; j < food_node->children.count; ++j) {
        XMLNode *child = food_node->children.nodes[j];
        if (child->type == NT_COMMENT) continue;
        fprintf(stdout, "\t %s: %s\n", child->name, XMLDecodeText(child));
      }
    }

    free(list);
  }
  XMLDocumentFree(&doc);
  return 0;
}
```

Result:
```sh
food count=[4]
MATCHES [0]:
         name: Belgian Waffles
         price: $5.95
         description: Two of our famous Belgian with "plenty of maple syrup"
         calories: 650
MATCHES [1]:
         name: Strawberry Belgian Waffles
         price: $7.95
         description: Light Belgian waffles covered with 'strawberries' and whipped cream
         calories: 900
MATCHES [2]:
         name: Berry-Berry Belgian Waffles
         price: $8.95
         description: Light Belgian waffles covered with an assortment of 'fresh berries' and whipped cream
         calories: 900
MATCHES [3]:
         name: Homestyle Breakfast
         price: $6.95
         description: Two eggs, bacon or sausage, toast, and our ever-popular hash browns
         calories: 950
```

### Handling attributes
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test4.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLNode *book = XMLSelectNode(XML_ROOT(&doc), "/bookstore/book[-1]"); //select the last book node
  for (size_t i = 0; i < book->attrList.count; ++i) { //iterating the attribute
    XMLAttr attr = book->attrList.attrs[i];
    fprintf(stdout, "%s => %s\n", attr.key, attr.value);
  }

  XMLDocumentFree(&doc);
  return 0;
}
```

Result:
```
category => web
cover => paperback
```

### XPath
```c
static void xpath_test(void) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./bookstore.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }
  XPathResult result1 = xpath("/bookstore/book[@category=CHILDREN]//text()", doc.root);
  printf("xpath result = %s\n", result1.text);

  XPathResult result2 = xpath("/bookstore/book/title/../price/text()", doc.root);
  printf("xpath result2 = %s\n", result2.text);

  XPathResult result3 = xpath("/bookstore/book[1]/title/text()", doc.root);
  printf("xpath result3 = %s\n", result3.text);

  XPathResult result4 = xpath("/bookstore/book[@category=CHILDREN]/year", doc.root);
  printf("xpath result4 = %s\n", result4.node->text);
  /* below two lines are same as above */
  //XPathResult result4 = xpath("/bookstore/book[@category=CHILDREN]/year/text()", root);
  //printf("xpath result4 = %s\n", result4.text);

  XPathResult result5 = xpath("/bookstore/book[1]//title", doc.root);
  for (size_t i = 0; i < result5.nodes.count; ++i) {
    XMLNode *child = result5.nodes.nodes[i];
    printf("xpath result5 = %s\n", child->text);
  }
  //if the xpath result is a nodelist, we need to use 'xpath_free（）' to free the memory
  xpath_free(&result5);

  XMLDocumentFree(&doc);
}
```

## License
MIT License
