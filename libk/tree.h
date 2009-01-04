#ifndef LIBK_TREE
#define LIBK_TREE

#define RB_TREE_RED 1
#define RB_TREE_BLACK 0

#define RB_TREE(name, data)			\
  struct name ## _node				\
  {						\
    struct name ## _node *left;			\
    struct name ## _node *right;		\
    struct name ## _node *prev;			\
    struct name ## _node *next;			\
    unsigned char color;			\
    data;					\
  };						\
  struct name					\
  {						\
    struct name ## _node *head;			\
  };

struct libk_rb_tree_node				
{						
  struct libk_rb_tree_node *left;
  struct libk_rb_tree_node *right;
  unsigned char color;
};

struct libk_rb_tree
{
  struct libk_rb_tree_node *head;
};

typedef int (*libk_rb_tree_compare)(struct libk_rb_tree_node *node,
				    void *data);
typedef int (*libk_rb_tree_compare_node)(struct libk_rb_tree_node *a,
					 struct libk_rb_tree_node *b);

#define RB_TREE_LOOKUP(tree, iter, compare, user_data) \
  _libk_rb_tree_lookup ((tree)->head, (iter), (compare), (user_data))
#define RB_TREE_INSERT(tree, node, compare) \
  _libk_rb_tree_insert ((tree)->head, (node), NULL, NULL, (compare))
#define RB_TREE_REMOVE(tree, compare, user_data) \
  libk_rb_tree_true_remove ((tree), (compare), (user_data))

static inline struct libk_rb_tree_node *
_libk_rb_tree_rotate_left (struct libk_rb_tree_node *node)
{
  struct libk_rb_tree_node *nnode;

  nnode = node->right;
  node->right = nnode->left;
  nnode->left = node;
  nnode->color = node->color;
  node->color = RB_TREE_RED;

  return nnode;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_rotate_right (struct libk_rb_tree_node *node)
{
  struct libk_rb_tree_node *nnode;

  nnode = node->left;
  node->left = nnode->right;
  nnode->right = node;
  nnode->color = node->color;
  node->color = RB_TREE_RED;

  return nnode;
}

static inline
_libk_rb_tree_flip_color (struct libk_rb_tree_node *node)
{
  node->color = !node->color;
  node->left->color = !node->left->color;
  node->right->color = !node->right->color;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_lookup (struct libk_rb_tree_node *head,
		      struct libk_rb_tree_node **iter,
		      libk_rb_tree_compare compare,
		      void *data)
{
  struct libk_rb_tree_node *_iter;

  if (iter == NULL)
    iter = &_iter;

  iter = head;
  while (*iter)
    {
      int cmp;

      cmp = compare(*iter, data);
      if (cmp == 0)
	return *iter;
      else if (cmp < 0)
	*iter = (*iter)->left;
      else
	*iter = (*iter)->right;
    }
  
  return NULL;
}

#define _LIBK_RB_IS_RED(n) ((n) && (n)->color)

static inline void
_libk_rb_tree_fix (struct libk_rb_tree_node *head)
{
 if (_LIBK_RB_IS_RED (head->right) && !_LIBK_RB_IS_RED (head->left))
   head = _libk_rb_tree_rotate_left (head);
 if (!_LIBK_RB_IS_RED (head->right) && _LIBK_RB_IS_RED (head->left))
   head = _libk_rb_tree_rotate_right (head);
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_insert (struct libk_rb_tree_node *head,
		      struct libk_rb_tree_node *node,
		      struct libk_rb_tree_node *prev,
		      struct libk_rb_tree_node *next,
		      libk_rb_tree_compare_node compare)
{
  int cmp;
  
  if (head == NULL)
    {
      node->prev = prev;
      node->next = next;
      if (prev)
	prev->next = node;
      if (next)
	next->prev = node;
      node->color = RB_TREE_RED;
      return node;
    }

  if (_LIBK_RB_IS_RED (head->left) && _LIBK_RB_IS_RED (head->right))
    _libk_rb_tree_flip_color (head);

  cmp = compare (node, head);
  if (cmp < 0)
    head->left = _libk_rb_tree_insert (head->left, node, prev, head, compare);
  else
    head->right = _libk_rb_tree_insert (head->right, node, head, next, compare);

  _libk_rb_tree_fix (head);
  
  return head;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_move_red_left (struct libk_rb_tree_node *node)
{
  _libk_rb_tree_flip_color (node);
  if (_LIBK_RB_IS_RED (node->right->left))
    {
      node->right = _libk_rb_tree_rotate_right (node->right);
      node = _libk_rb_tree_rotate_left (node);
      _libk_rb_tree_flip_color (node);
    }
  return node;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_move_red_right (struct libk_rb_tree_node *node)
{
  _libk_rb_tree_flip_color (node);
  if (_LIBK_RB_IS_RED (node->left->left))
    {
      node = _libk_rb_tree_rotate_right (node);
      _libk_rb_tree_flip_color (node);
    }
  return node;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_remove_min_tmp (struct libk_rb_tree_node *node,
			      struct libk_rb_tree_node **removed)
{
  if (node->left == NULL)
    {
      *removed = node;
      return NULL;
    }

  if (!_LIBK_RB_IS_RED (node->left) && !_LIBK_RB_IS_RED(node->left->left))
    node = _libk_rb_tree_move_red_left (node);

  node->left = _libk_rb_tree_remove_min_tmp(node->left, removed);

  return _libk_rb_tree_fix (node);
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_true_remove (struct libk_rb_tree *tree,
			   libk_rb_tree_compare compare,
			   void *user_data)
{
  struct libk_rb_tree_node *removed;

  _libk_rb_tree_remove (tree->head, &removed, compare, user_data);

  return removed;
}

static inline struct libk_rb_tree_node *
_libk_rb_tree_remove (struct libk_rb_tree_node *node,
		      struct libk_rb_tree_node **removed,
		      libk_rb_tree_compare compare,
		      void *user_data)
{
  if (compare (node, user_data) < 0)
    {
      if (!_LIBK_RB_IS_RED (node->left) && !_LIBK_RB_IS_RED(node->left->left))
	node = _libk_rb_tree_move_red_left (node);
      node->left = _libk_rb_tree_remove (node->left, removed,
					 compare, user_data);
    }
  else
    {
      if (_LIBK_RB_IS_RED (node->left))
	node = _libk_rb_tree_rotate_right (node);
      if (compare (node, user_data) == 0 && (node->right))
	{
	  *removed = node;
	  return NULL;
	}
      if (!_LIBK_RB_IS_RED (node->right) &&
	  !_LIBK_RB_IS_RED(node->right->left))
	node = _libk_rb_tree_move_red_right (node);
      if (compare (node, user_data) == 0)
	{
	  struct libk_rb_tree_node *nright;

	  *removed = node;
	  
	  nright = _libk_rb_tree_remove_min_tmp (node->right, &node);
	  node->left = (*removed)->left;
	  node->right = nright;
	  node->color = (*removed)->color;

	  node->prev = (*removed)->prev;
	  if ((*removed)->prev)
	    (*removed)->prev->next = node;
	}
      else
	node->right = _libk_rb_tree_remove(node->right, removed,
					   compare, user_data);
    }

  return _libk_rb_tree_fix (node);
}

#endif
