#ifndef LIBK_TREE
#define LIBK_TREE

#define RB_TREE_RED 0
#define RB_TREE_BLACK 1

#define RB_TREE(name, data)			\
  struct name ## _node				\
  {						\
    struct name ## _node *left;			\
    struct name ## _node *right;		\
    struct name ## _node *prev;			\
    struct name ## _node *next;			\
    struct name ## _node *parent;		\
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
  struct libk_rb_tree_node *prev;
  struct libk_rb_tree_node *next;
  struct libk_rb_tree_node *parent;
  unsigned char color;
};

struct libk_rb_tree
{
  struct libk_rb_tree_node *head;
};

static struct libk_rb_tree_node *
_libk_rb_tree_lookup (struct libk_rb_tree_node *node,
		      struct libk_rb_tree_node **iter,
		      int (*compare)(struct libk_rb_tree_node *a,
				     void *user_data),
		      void *user_data)
{
  int res;
  
  if (node == NULL)
    return NULL;

  if (iter != NULL)
    *iter = node;
  
  res = compare (node, user_data);
  return (res == 0) ? node
                    : _libk_rb_tree_lookup((res < 0) ? node->left
					             : node->right,
					   iter, compare, user_data);
}

static void
_libk_rb_tree_rotate_right (struct libk_rb_tree_node *node)
{
  struct libk_rb_tree_node *q, *p, *b, *c, *pp;

  pp = node->parent;
  q = node;
  p = node->left;
  b = p->right;
  c = q->right;
  
  if(pp)
    {
      if (pp->left = q)
	pp->left = p;
      else
	pp->right = p;
    }
  p->parent = pp;
  q->parent = p;
  p->right = q;
  if (b)
    b->parent = q;
  q->left = b;
  if (c)
    c->parent = q;
  q->right = c;
}

static void
_libk_rb_tree_rotate_left (struct libk_rb_tree_node *node)
{
  struct libk_rb_tree_node *q, *p, *b, *c, *pp;

  pp = q->parent;
  q = node->right;
  p = node;
  b = q->left;
  c = q->right;

  if(pp)
    {
      if (pp->left = p)
	pp->left = q;
      else
	pp->right = q;
    }
  if (b)
    b->parent = p;
  p->right = b;
  p->parent = q;
  q->left = p;
  if (c)
    c->parent = q;
  q->right = c;
}

static void
_libk_rb_tree_insert_balance (struct libk_rb_tree_node *node)
{
  struct libk_rb_tree_node *uncle;
  struct libk_rb_tree_node *parent;
  struct libk_rb_tree_node *grandparent;
  
  parent = node->parent;
  if (parent)
    {
      grandparent = node->parent->parent;
      if (grandparent)
	{
	  if (parent == grandparent->left)
	    uncle = grandparent->right;
	  else
	    uncle = grandparent->left;
	}
    }
  
  if (parent == NULL)
    {
      node->color = RB_TREE_BLACK;
    }
  else if (parent->color == RB_TREE_BLACK)
    {
      node->color = RB_TREE_RED;
    }
  else if (uncle && uncle->color == RB_TREE_RED)
    {
      parent->color = RB_TREE_BLACK;
      uncle->color = RB_TREE_BLACK;
      grandparent->color = RB_TREE_RED;
      _libk_rb_tree_insert_balance (grandparent);
    }
  else
    {
      if (node == parent->right && parent == grandparent->left)
	{
	  _libk_rb_tree_rotate_left(parent);
	  parent = node;
	  uncle = node->right;
	  node = node->left;
	}
      else if (node == parent->left && parent == grandparent->right)
	{
	  _libk_rb_tree_rotate_right (parent);
	  parent = node;
	  uncle = node->left;
	  node = node->right;
	}

      parent->color = RB_TREE_BLACK;
      grandparent->color = RB_TREE_RED;
      if ((node == parent->left) && (parent == grandparent->left))
	{
	  _libk_rb_tree_rotate_right (grandparent);
	}
      else
	{
	  _libk_rb_tree_rotate_left (grandparent);
	}
    }
}

static void
_libk_rb_tree_insert (struct libk_rb_tree_node *node,
		      struct libk_rb_tree_node **place,
		      struct libk_rb_tree_node *parent,
		      int (*compare)(struct libk_rb_tree_node *a,
				     struct libk_rb_tree_node *b))
{
  struct libk_rb_tree_node **nuncle;

  if (*place != NULL)
    {
      int branch;
      struct libk_rb_tree_node **nplace;
      branch = compare (parent, node);
      if (branch < 0)
	{
	  nplace = &parent->left;
	}
      else
	{
	  nplace = &parent->right;
	}
      _libk_rb_tree_insert(node, nplace, *place, compare);
    }
  else
    {
      int branch;
      *place = node;
      node->parent = parent;
      if (branch < 0)
	{
	  node->next = parent;
	  if (parent)
	    {
	      node->prev = parent->prev;
	      if (parent->prev)
		{
		  parent->prev->next = node;
		}
	      parent->prev = node;
	    }
	}
      else
	{
	  node->prev = parent;
	  if (parent)
	    {
	      node->next = parent->next;
	      if (parent->next)
		{
		  parent->next->prev = node;
		}
	      parent->next = node;
	    }
	}
      _libk_rb_tree_insert_balance (node);
    }
};

static struct libk_rb_tree_node *
_libk_rb_tree_remove (struct libk_rb_tree_node **node,
		      int (*compare)(struct libk_rb_tree_node *a,
				     void *user_data))
{
  /* TODO: Implement */
}

#define RB_TREE_LOOKUP(tree, iter, compare, user_data)			\
  _libk_rb_tree_lookup ((struct libk_rb_tree_node *)(tree)->head,	\
			(struct libk_rb_tree_node **)(iter),		\
			(int (*)(struct libk_rb_tree_node *,		\
				 void *))(compare),			\
			(void *)(user_data))
#define RB_TREE_INSERT(tree, node, compare)				\
  _libk_rb_tree_insert ((struct libk_rb_tree_node *)(node),		\
			(struct libk_rb_tree_node **)&(tree)->head,	\
			NULL,						\
			(int (*)(struct libk_rb_tree_node *,		\
				 struct libk_rb_tree_node *))		\
			(compare))
#define RB_TREE_REMOVE(tree, compare, user_data)			\
  _libk_rb_tree_remove ((struct libk_rb_tree_node **)&(tree)->head,	\
			(int (*)(struct libk_rb_tree_node *,		\
				 void *))				\
			(compare), (user_data))
 
#endif
