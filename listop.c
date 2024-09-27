#include "vma.h"

//functions that creates a generic doubly linked list
list_t *dll_create(uint64_t info_size)
{
	list_t *dll;
	dll = malloc(sizeof(*dll));
	if (!dll) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	dll->head = NULL;
	dll->info_size = info_size;
	dll->num_nodes = 0;
	return dll;
}

// adds a node on the nth position
void add_nth_node(list_t *dll, uint64_t n, const void *new_info)
{
	node_t *prev, *act, *new_node;

	if (!dll)
		return;

	if (n > dll->num_nodes)
		n = dll->num_nodes;

	act = dll->head;
	prev = NULL;
	while (n > 0) {
		prev = act;
		act = act->next;
		--n;
	}

	new_node = malloc(sizeof(*new_node));
	if (!new_node) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	new_node->info = malloc(dll->info_size);
	if (!new_node->info) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	memcpy(new_node->info, new_info, dll->info_size);

	new_node->next = act;
	new_node->prev = prev;

	if (act)
		act->prev = new_node;
	if (!prev) {
		dll->head = new_node;
		dll->head->prev = NULL;
	} else {
		prev->next = new_node;
		new_node->prev = prev;
	}
	dll->num_nodes++;
}

// returns the address of the node that should be freed
node_t *remove_nth_node(list_t *dll, uint64_t n)
{
	node_t *prev, *act;

	if (!dll || !dll->head)
		return NULL;

	if (n > dll->num_nodes - 1)
		n = dll->num_nodes - 1;

	act = dll->head;
	prev = NULL;
	while (n > 0) {
		prev = act;
		act = act->next;
		--n;
	}

	if (!prev) {
		dll->head = act->next;
		if (act->next)
			act->next->prev = NULL;
	} else {
		prev->next = act->next;
		if (act->next)
			act->next->prev = prev;
	}

	if (act && act->next)
		act->next->prev = prev;

	dll->num_nodes--;

	return act;
}
