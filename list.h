#ifndef APP_LIST_H
#define APP_LIST_H

#include <stddef.h>
#include <stdint.h>

typedef struct list_node {
  uint32_t item_value;
  struct list_node *Next;
  struct list_node *Prev;
  void *owner;
  void *container;
} ListItem_t;

typedef struct list {
  size_t number_of_items;
  ListItem_t *index;
  ListItem_t end;
} List_t;

void vListInit(List_t *List);
void vListItemInit(ListItem_t *ListItem);
void vListInsertEnd(List_t *List, ListItem_t *ListItem);
void vListInsert(List_t *List, ListItem_t *ListItem);
void vListRemove(ListItem_t *ListItem);

#endif /* APP_LIST_H */
