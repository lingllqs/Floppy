#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

List* list_create()
{
    List *new_list = (List *)malloc(sizeof(List));
    err_sys(NULL == new_list, malloc List, goto ERR1);

    new_list->size = 0;
    new_list->front = NULL;
    new_list->end = NULL;

    return new_list;

ERR1:
    return NULL;
}

Node* node_create(void *data, unsigned int data_size)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    err_sys(NULL == new_node, malloc Node, goto ERR1);

    new_node->data = (void *)malloc(data_size);
    err_sys(NULL == new_node->data, malloc new_node data, goto ERR2);

    memcpy(new_node->data, data, data_size);
    new_node->prev = NULL;
    new_node->next = NULL;

    return new_node;

ERR1:
    return NULL;

ERR2:
    free(new_node);
    new_node = NULL;
    return NULL;
}

// 插入的结点请确保没有其他指针所指向
// 否则销毁链表时会产生错误
void push_back(List *list, Node *node)
{
    err_sys(NULL == list || NULL == node, push_back, goto ERR1);

    if (list->size == 0)
    {
        list->front = node;
        list->end = node;
    }
    else
    {
        list->end->next = node;
        node->prev = list->end;
        list->end = node;
    }

    list->size++;

ERR1:
    return;
}

// 返回删除的结点
Node* pop_back(List *list)
{
    err_sys(list == NULL, pop_back, goto ERR1);

    if (list->size == 0)
    {
        perror("链表数据为空");
        goto ERR1;
    }

    Node *tail = list->end;

    if (list->size == 1)
    {
        list->front = NULL;
        list->end = NULL;
    }
    else 
    {
        list->end = tail->prev;
        tail->prev = NULL;
        list->end->next = NULL;
    }

    list->size--;

    return tail;

ERR1:
    return NULL;
}

unsigned int find_by_key(List *list, void *key, CMP cmp, PRINT print)
{
    err_sys(NULL == list, find_by_key, goto ERR1);

    int i = 0, count = 0;
    Node *tmp = list->front;
    for (i = list->size; i > 0; i--)
    {
        if (cmp(tmp->data, key) == 0)
        {
            print(tmp->data);
            count++;
        }
    }

    return count;

ERR1:
    return 0;
}

// 返回修改的次数
unsigned int modify_by_key(List *list, void *key, void *data, CMP cmp, MODIFY modify)
{
    err_sys(NULL == list, modify_by_key, goto ERR1);

    int i = 0, count = 0;
    Node *tmp = list->front;
    for (i = list->size; i > 0; i--)
    {
        if (cmp(tmp->data, key) == 0)
        {
            count++;
            modify(tmp->data, data);
        }
    }

    return count;

ERR1:
    return 0;
}

void for_each(List *list, PRINT print)
{
    err_sys(NULL == list, modify_by_key, goto ERR1);

    int i = 0;
    Node *tmp = list->front;
    for (i = list->size; i > 0; i--, tmp = tmp->next)
    {
        print(tmp->data);
    }

    return;
    
ERR1:
    return;
}

void list_free(List *list)
{
    err_sys(NULL == list, modify_by_key, goto ERR1);

    int i = 0;
    Node *tmp = list->front;
    for (i = list->size; i > 0; i--)
    {
        list->front = tmp->next;
        node_free(tmp);
        tmp = list->front;
    }

    free(list);
    list = NULL;

    return;

ERR1:
    return;
}

void node_free(Node *node)
{
    if (node)
    {
        if (node->data)
        {
            free(node->data);
            node->data = NULL;
        }
        free(node);
        node = NULL;
    }

    return;
}
