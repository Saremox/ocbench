/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * openCompressBench is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openCompressBench.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

#include <stdlib.h>
#include "debug.h"
#include "ocutils/list.h"

List* ocutils_list_create()
{
  return calloc(1,sizeof(List));
}

void    ocutils_list_freefp (List* list, ocutilsListFreeFunction fp)
{
  list->cleanup = fp;
}

void  ocutils_list_destroy(List* list)
{
  ocutils_list_clear(list);
  free(list);
}

void ocutils_list_clear  (List* list)
{
  ocutils_list_foreach_fraw(list, cur)
  {
    if(cur->prev)
    {
      if(list->cleanup)
        list->cleanup(cur->prev->value);
      free(cur->prev);
    }
  }
  if(ocutils_list_tail(list))
  {
    if(list->cleanup)
      list->cleanup(ocutils_list_tail(list)->value);
    free(ocutils_list_tail(list));
  }

  list->tail = NULL;
  list->head = NULL;
  list->items = 0;
}

void  ocutils_list_remptr (List* list, void* val)
{
  int64_t position = ocutils_list_getpos(list,val);
  check(position >= 0,"cannot find the given value in the list");
  ocutils_list_rempos(list,position);
error:
  return;
}

void  ocutils_list_addpos (List* list, void* val, int64_t position)
{
  check(position <= ocutils_list_size(list),
    "invalid position. greater than size of list");
  ListNode* newnode = calloc(1,sizeof(ListNode));
  newnode->value = val;
  if(ocutils_list_size(list) == 0)
  {
    list->tail = list->head = newnode;
  }
  else if(position == ocutils_list_size(list))
  {
    newnode->prev = list->tail;
    list->tail = list->tail->next = newnode;
  }
  else if(position == 0)
  {
    newnode->next =list->head;
    list->head = list->head->prev = newnode;
  }
  else
  {
    ListNode* prev = NULL;
    ListNode* next = list->head;
    for(int64_t curPos = 0; curPos < position; curPos++)
    {
      prev = next;
      next = next->next;
    }
    prev->next = newnode;
    next->prev = newnode;
    newnode->prev = prev;
    newnode->next = next;
  }
  list->items++;
error:
  return;
}

void* ocutils_list_rempos (List* list, int64_t position)
{
  if(position >= ocutils_list_size(list))
  {
    log_warn("invalid position. greater than size of list");
    goto error;
  }
  void* val;
  if(position == list->items)
  {
    val = list->tail->value;
    list->tail = list->tail->prev;
    if(list->tail != NULL && list->tail->next !=NULL)
    {
      free(list->tail->next);
      list->tail->next = NULL;
    }
  }
  else if(position == 0)
  {
    val = list->head->value;
    list->head = list->head->next;
    if (list->head != NULL && list->head->prev !=NULL)
    {
      free(list->head->prev);
      list->head->prev = NULL;
    }
  }
  else
  {
    ListNode* prev = NULL;
    ListNode* next = list->head;
    for(int64_t curPos = 0; curPos < position; curPos++)
    {
      prev = next;
      next = next->next;
    }
    val = next->value;
    prev->next = next->next;
    next->next->prev = prev;
    free(next);
  }
  list->items--;
  return val;
error:
  return (void*) -1;
}

int64_t ocutils_list_getpos (List* list, void* value)
{
  int64_t curPos = 0;
  ListNode* next = list->head;
  for(; curPos < list->items; curPos++)
  {
    if(next->value == value)
      break;
    next = next->next;
  }
  if(next->value == value)
    return curPos;
  else
    return -1;
}

void* ocutils_list_get (List* list, int64_t position)
{
  void* val;
  ListNode* next = list->head;
  for(int64_t curPos = 0; curPos < position; curPos++)
  {
    next = next->next;
  }
  val = next->value;

  return val;
}

void ocutils_list_sort (List* list, ocutilsListSortFunction sortfkt)
{
  // Function STUB
}
