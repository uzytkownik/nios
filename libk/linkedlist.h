#ifndef LIBK_LINKEDLIST
#define LIBK_LINKEDLIST

#define LIST(name, data)			\
  struct name ## _node				\
  {						\
    struct name ## _node *next;			\
    struct name ## _node *prev;			\
    data;					\
  };						\
						\
  struct name					\
  {						\
    struct name ## _node *head;			\
    struct name ## _node *tail;			\
  };

#define LIST_NEXT(iter) (iter)->next
#define LIST_PREV(iter) (iter)->prev
#define LIST_LPUSH(list, node)			    \
  if (list->head == NULL)			    \
    {						    \
      list->head = list->tail = node;		    \
      node->next = node->prev = NULL;		    \
    }						    \
  else						    \
    {						    \
      node->prev = NULL;			    \
      node->next = list->head;			    \
      list->head->prev = node;			    \
      list->head = node;			    \
    }
#define LIST_LPOP(list, node)		    \
  if (list->head == NULL)		    \
    {					    \
      *(node) = NULL;			    \
    }					    \
  else					    \
    {					    \
      *(node) = list->head;		    \
      if (list->head->next)		    \
	list->head->next->prev = NULL;	    \
      else				    \
	list->tail = NULL;		    \
      list->head = list->head->next;	    \
    }
#define LIST_INSERT_AFTER(list, iter, node) \
  if (iter->next == NULL)			\
    {						\
      iter->next = node;			\
      node->prev = iter;			\
      node->next = NULL;			\
      list->tail = node;			\
    }						\
  else						\
    {						\
      iter->next->prev = node;			\
      node->next = iter->next->prev;		\
      iter->next = node;			\
      node->prev = node;			\
    }
#define LIST_INSERT_BEFORE(list, iter, node) \
  if (iter->prev == NULL)			\
    {						\
      iter->prev = node;			\
      node->next = iter;			\
      node->prev = NULL;			\
      list->head = node;			\
    }						\
  else						\
    {						\
      iter->prev->next = node;			\
      node->prev = iter->prev->next;		\
      iter->prev = node;			\
      node->next = node;			\
    }
#define LIST_REMOVE(list, iter)			\
  do						\
    {						\
      if (iter->next == NULL)			\
	{					\
	  list->tail = iter->prev;		\
	}					\
      else					\
	{					\
	  iter->next->prev = iter->prev;	\
	}					\
      if (iter->prev == NULL)			\
	{					\
	  list->head = iter->next;		\
	}					\
      else					\
	{					\
	  iter->prev->next = iter->next;	\
	}					\
    }						\
  while (0)
#define LIST_FIND(list, iter, find, user_data)		\
  do							\
    {							\
      *(iter) = list->head;				\
      while (*(*(iter)) && !find (*(iter), user_data))	\
	*(iter) = LIST_NEXT(*(iter));			\
    }							\
  while (0)

#endif


