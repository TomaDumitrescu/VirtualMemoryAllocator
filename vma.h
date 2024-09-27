/*
	Header with memory units lists and functions headers
*/
#ifndef VMA_H
#define VMA_H

#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define MAX_COMMAND 50
#define MAX_TEXT 500
#define DEF_PERM 6

// node for doubly linked list (miniblock or block type)
typedef struct node_t {
	void *info;
	struct node_t *prev;
	struct node_t *next;
} node_t;

//generic doubly linked list classical implementation
typedef struct list_t {
	struct node_t *head;
	uint64_t info_size;
	uint64_t num_nodes;
} list_t;

typedef struct block_t {
	uint64_t start_address;
	size_t size;
	list_t *miniblock_list;
} block_t;

typedef struct miniblock_t {
	uint64_t start_address;
	size_t size;
	//initialized with 6 when the miniblock is allocated
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

// virtual memory field
typedef struct arena_t {
	uint64_t arena_size;
	list_t *block_list;
} arena_t;

// functions for virtual memory representation in the physical memory
void alloc_arena(const uint64_t size, arena_t *arena);
void dealloc_arena(arena_t *arena);
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

// functions for operations on virtual memory
void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, char *data);
char *perm(uint8_t mask);
void pmap(const arena_t *arena);
void mprotect(arena_t *arena, uint64_t address, uint8_t *permission);

list_t *dll_create(uint64_t info_size);
void add_nth_node(list_t *dll, uint64_t n, const void *new_info);
node_t *remove_nth_node(list_t *dll, uint64_t n);

#endif
