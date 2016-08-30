#include <stdint.h>

#ifndef LIST_H
#define LIST_H

struct __List;
struct __ListNode;

typedef struct __List     List;
typedef struct __ListNode ListNode;
typedef int (*ocutilsListSortFunction)(ListNode*,ListNode*);
typedef void (*ocutilsListFreeFunction)(void* ptr);

struct __List {
  int64_t                 items;
  ListNode*               head;
  ListNode*               tail;
  ocutilsListFreeFunction cleanup;
};

struct __ListNode {
  ListNode* next;
  ListNode* prev;
  void*     value;
};

#define ocutils_list_size(LIST) LIST->items
#define ocutils_list_head(LIST) LIST->head
#define ocutils_list_tail(LIST) LIST->tail

List*   ocutils_list_create (void);
void    ocutils_list_freefp (List* list, ocutilsListFreeFunction fp);
void    ocutils_list_clear  (List* list);
void    ocutils_list_destroy(List* list);
void    ocutils_list_remptr (List* list, void* val);
void    ocutils_list_addpos (List* list, void* val, int64_t position);
void*   ocutils_list_rempos (List* list, int64_t position);
void*   ocutils_list_get    (List* list, int64_t position);
int64_t ocutils_list_getpos (List* list, void* val);
void    ocutils_list_sort   (List* list, ocutilsListSortFunction);

#define ocutils_list_add(LIST, VALUE) \
  ocutils_list_addpos(LIST, VALUE, ocutils_list_size(LIST))

#define ocutils_list_push(LIST, VALUE) \
  ocutils_list_addpos(LIST, VALUE, 0)

#define ocutils_list_pop(LIST) \
  ocutils_list_rempos(LIST,0)

#define ocutils_list_enqueue(LIST, VALUE) \
  ocutils_list_addpos(LIST, VALUE, ocutils_list_size(LIST))

#define ocutils_list_dequeue(LIST) \
  ocutils_list_rempos(LIST,0)

#define ocutils_list_foreach_f(LIST, ITERATOR) ListNode* ITERATOR ## _node = NULL;\
  ListNode* ITERATOR = NULL;\
  for(ITERATOR = ITERATOR ## _node = LIST->head;\
      ITERATOR ## _node != NULL;\
      ITERATOR = ITERATOR ## _node = ITERATOR ## _node->next)

#define ocutils_list_foreach_b(LIST, ITERATOR) ListNode* ITERATOR ## _node = NULL;\
  ListNode* ITERATOR = NULL;\
  for(ITERATOR = ITERATOR ## _node = LIST->tail;\
      ITERATOR_node != NULL;\
      ITERATOR = ITERATOR ## _node = ITERATOR_node->prev)

#endif
