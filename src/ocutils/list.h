/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

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

#define ocutils_list_foreach_f(LIST, ITERATOR, TYPE)\
  ListNode* ITERATOR ## _node = NULL;\
  TYPE ITERATOR = NULL;\
  for(ITERATOR ## _node = LIST->head,\
        ITERATOR = ITERATOR ## _node == NULL ? 0 : ITERATOR ## _node->value;\
      ITERATOR ## _node != NULL;\
      ITERATOR ## _node = ITERATOR ## _node->next,\
        ITERATOR = ITERATOR ## _node == NULL ? 0 : ITERATOR ## _node->value)

#define ocutils_list_foreach_fraw(LIST, ITERATOR)\
  ListNode* ITERATOR ## _node = NULL;\
  ListNode* ITERATOR = NULL;\
  for(ITERATOR = ITERATOR ## _node = LIST->head;\
      ITERATOR ## _node != NULL;\
      ITERATOR = ITERATOR ## _node = ITERATOR ## _node->next)

#define ocutils_list_foreach_b(LIST, ITERATOR, TYPE)\
  ListNode* ITERATOR ## _node = NULL;\
  TYPE ITERATOR = NULL;\
  for(ITERATOR ## _node = LIST->tail;\
        ITERATOR = ITERATOR ## _node == NULL ? 0 : ITERATOR ## _node->value;\
      ITERATOR_node != NULL;\
      ITERATOR ## _node = ITERATOR ## _node->prev,\
        ITERATOR = ITERATOR ## _node == NULL ? 0 : ITERATOR ## _node->value)

#define ocutils_list_foreach_braw(LIST, ITERATOR)\
  ListNode* ITERATOR ## _node = NULL;\
  ListNode* ITERATOR = NULL;\
  for(ITERATOR = ITERATOR ## _node = LIST->tail;\
      ITERATOR_node != NULL;\
      ITERATOR = ITERATOR ## _node = ITERATOR_node->prev)

#endif
