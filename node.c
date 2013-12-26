#include "node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void die(const char *format, ...) {
	char buf[512] = {0};
	va_list va;

	va_start(va, format);

	vsprintf(buf, format, va);

	puts(buf);
	exit(1);
}

void *safecalloc(size_t size) {
	void *block = NULL;

	if((block = calloc(1, size)) == NULL) {
		die("error: calloc fail on line %d in %s", __LINE__, __FILE__);
	}

	return block;
}	

void free_node(struct Node **head) {
	if(!(*head)) return;

	struct Node *node = *head, *tmp = NULL;
	while(node) {
		tmp = node;
		node = node->next;
		if(tmp->block) free(tmp->block);
		free(tmp);
	}

	free(node);
	*head = NULL;
}