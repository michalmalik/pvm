#ifndef __NODE_H__
#define __NODE_H__

#include <stdint.h>

/*
	mn - master node
	n - temporary node
	b - data block
*/
#define ADD_NODE(mn, n, b)	{	\
	n->next = mn;			\
	n->block = b;			\
	mn = n;				\
				}
/*
	t - type
*/
#define NEW_NODE(t)			(t *)safecalloc(sizeof(t))
/*
	n - temporary node
	t - type
*/
#define CAST_NODE(n, t)			((t *)n->block)
/*
	n - temporary node
	l - list of nodes
*/
#define ENUM_LIST(n, l)			for(n = l; n; n = n->next)

/*
	b - block
*/
#define ZERO(b)				memset(b, 0, sizeof(b))
#define COUNT(b)			sizeof((b))/sizeof((b)[0])

struct Node {
	struct Node *next;
	void *block;
};

typedef struct Node Node_t;

void die(const char *format, ...);
void *safecalloc(size_t size);
void free_node(struct Node **head);

#endif