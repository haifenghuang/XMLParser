# C语言简单XML解析器
XMLParser是用c语言写的一个简单的XML解析器。它的主要目的是写一个容易理解且易于使用的XML处理器。虽然这个解析器不是很完美, 但是它能够处理大部分标准的XML文档。同时它也不支持xpath语法，但是它提供了一个用户友好且易用(也许更优雅)的方法来选择/过滤XML节点(虽然不如XPath强大)。

English version: [English](README.md)
## 特点
- 简单
- 易于使用和理解
- XML文档格式化
- 节点过滤和选择

## 限制
- ~~不支持CDATA~~
- ~~不支持DOCTYPE()~~(DOCTYPE只支持解析，不支持验证)
- 不支持Unicode(只支持UTF-8)

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

### 选择特定节点(node)
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

## License
MIT License
