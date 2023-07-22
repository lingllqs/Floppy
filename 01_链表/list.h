#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#define err_sys(condition, message, process) do { \
    if (condition) {                              \
        printf(#message" false!");                \
        process;                                  \
    }                                             \
} while(0);

typedef int (*CMP)(void *dest, void *src);
typedef void (*PRINT)(void *data);
typedef void (*MODIFY)(void *dest, void *src);

// 结点
typedef struct Node 
{
    void *data;        // 数据域
    struct Node *prev; // 前一个结点
    struct Node *next; // 后一个结点
} Node;

// 链表
typedef struct List 
{
    unsigned int size;  // 链表结点个数
    struct Node *front; // 指向链表的头结点
    struct Node *end;   // 指向链表的尾结点
} List;

// 创建链表
List* list_create();
// 新建结点
Node* node_create(void *data, unsigned int data_size);
// 尾插法
void push_back(List *list, Node *node);
// 删除尾部结点
Node* pop_back(List *list);
// 根据key值查找第一个找到的内容
unsigned int find_by_key(List *list, void *key, CMP cmp, PRINT print);
// 根据key值修改所有匹配的内容
unsigned int modify_by_key(List *list, void *key, void *data, CMP cmp, MODIFY modify);
// 遍历所有结点
void for_each(List *list, PRINT print);
// 销毁链表
void list_free(List *list);
// 销毁结点
void node_free(Node *node);

#endif // !LIST_H
