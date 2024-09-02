# C语言简单XML解析器
XMLParser是用c语言写的一个简单的XML解析器。它的主要目的是写一个容易理解且易于使用的XML处理器。虽然这个解析器不是很完美, 但是它能够处理大部分标准的XML文档。同时它支持简单的xpath语法。它同时提供了一个用户友好且易用(也许更优雅)的方法来选择/过滤XML节点(虽然不如XPath强大)。

English version: [English](README.md)
## 特点
- 简单
- 易于使用和理解
- XML文档格式化
- 节点过滤和选择
- 简单XPATH支持

## 限制
- ~~不支持CDATA~~
- ~~不支持DOCTYPE()~~(DOCTYPE只支持解析，不支持验证)
- 不支持Unicode(只支持UTF-8)

## 关于CMake
这是我第一次在项目中尝试使用`CMake`。所以我将原来的`makefile`更改成了`makefile.old`。增加了`CMakeLists.txt`文件。
要使用`CMake`来构建项目，请使用如下的命令：

```sh
  cmake -S . -B build
  cmake --build build/
  ctest --test-dir build   # 运行测试
  cd build && ./xml_parser # 直接运行
```

## 使用例

### 格式化XML文档
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLPrettyPrint(&doc, NULL, 4); /* output to console */

  XMLDocumentFree(&doc);
  return 0;
}
```

结果：
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

### 选择特定节点(XMLSelectNode)
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLNode *root = XML_ROOT(&doc);
  XMLNode *n = XMLSelectNode(root, "/struct/fields/field[2]/age"); //选择<fields>节点的第二个<field>节点的<age>子节点
  if (n != NULL) printf("n.text=%s\n", n->text);

  XMLDocumentFree(&doc);
  return 0;
}
```

### 选择特定名字的节点(XMLSelectNode & XMLFindNode)
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test4.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  /* 选择根节点的第三个book节点 */
  XMLNode *book = XMLSelectNode(XML_ROOT(&doc), "/bookstore/book[3]");

  /* 查找第三个book节点中的所有"author"节点 */
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

结果:
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

# 运行结果:
1： James McGovern
2： Per Bothner
3： Kurt Cagle
4： James Linn
5： Vaidyanathan Nagarajan
```

### 使用回调函数选择特定节点(XMLFindNodeSelector)
```c
/* 将price节点的值大于5的food节点选出 */
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

结果:
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

### 处理属性
```c
int main(int argc, char **argv) {
  XMLDocument doc = { 0 };
  bool result = XMLDocumentParseFile(&doc, "./test4.xml");
  if (result != true) {
    fprintf(stderr, "XMLDocumentParseFile failed!\n");
    exit(1);
  }

  XMLNode *book = XMLSelectNode(XML_ROOT(&doc), "/bookstore/book[-1]"); //选择最后一个book节点
  for (size_t i = 0; i < book->attrList.count; ++i) { //遍历属性
    XMLAttr attr = book->attrList.attrs[i];
    fprintf(stdout, "%s => %s\n", attr.key, attr.value);
  }

  XMLDocumentFree(&doc);
  return 0;
}
```

结果:
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
  /* 下面两行等价于上面的两行 */
  //XPathResult result4 = xpath("/bookstore/book[@category=CHILDREN]/year/text()", root);
  //printf("xpath result4 = %s\n", result4.text);

  XPathResult result5 = xpath("/bookstore/book[1]//title", doc.root);
  for (size_t i = 0; i < result5.nodes.count; ++i) {
    XMLNode *child = result5.nodes.nodes[i];
    printf("xpath result5 = %s\n", child->text);
  }
  //如果xpath()的结果是一个nodelist， 需要调用'xpath_free（）'释放内存
  xpath_free(&result5);

  XMLDocumentFree(&doc);
}
```

## License
MIT License
