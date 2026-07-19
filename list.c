#include "list.h"
#include <stdint.h>

void vListInit(List_t *List) {
  if (List == NULL) {
    return;
  }

  List->number_of_items = 0U;
  List->index = &(List->end);

  List->end.item_value = UINT32_MAX;
  List->end.Next = &(List->end);
  List->end.Prev = &(List->end);
  List->end.owner = NULL;
  List->end.container = NULL;
}

void vListItemInit(ListItem_t *ListItem) {
  if (ListItem == NULL) {
    return;
  }

  ListItem->item_value = 0U;
  ListItem->Next = NULL;
  ListItem->Prev = NULL;
  ListItem->owner = NULL;
  ListItem->container = NULL;
}

void vListInsertEnd(List_t *List, ListItem_t *ListItem) {
  ListItem_t *index;

  if (List == NULL || ListItem == NULL || ListItem->container != NULL) {
    return;
  }

  index = List->index;

  ListItem->Next = index;
  ListItem->Prev = index->Prev;

  index->Prev->Next = ListItem;
  index->Prev = ListItem;

  ListItem->container = List;
  List->number_of_items++;
}

void vListInsert(List_t *List, ListItem_t *ListItem) {
  ListItem_t *Iterator;

  if (List == NULL || ListItem == NULL || ListItem->container != NULL) {
    return;
  }

  Iterator = List->end.Next;

  while ((Iterator != &(List->end)) &&
         (Iterator->item_value <= ListItem->item_value)) {
    Iterator = Iterator->Next;
  }

  ListItem->Next = Iterator;
  ListItem->Prev = Iterator->Prev;

  Iterator->Prev->Next = ListItem;
  Iterator->Prev = ListItem;

  ListItem->container = List;
  List->number_of_items++;
}

void vListRemove(ListItem_t *ListItem) {
  List_t *List;

  if (ListItem == NULL || ListItem->container == NULL) {
    return;
  }

  List = (List_t *)ListItem->container;

  ListItem->Next->Prev = ListItem->Prev;
  ListItem->Prev->Next = ListItem->Next;

  if (List->index == ListItem) {
    List->index = ListItem->Prev;
  }

  ListItem->Next = NULL;
  ListItem->Prev = NULL;
  ListItem->container = NULL;

  if (List->number_of_items > 0U) {
    List->number_of_items--;
  }
}
