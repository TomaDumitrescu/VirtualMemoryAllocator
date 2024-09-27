#include "vma.h"

void alloc_arena(const uint64_t size, arena_t *arena)
{
	arena->arena_size = size;
	arena->block_list = dll_create(sizeof(block_t));
}

/* Freeing nodes and lists from arena, method: order of freeing: rw_buffer,
miniblock, m_node for every node of miniblock_list, then miniblock_list,
block, b_node for every block, then block_list */
void dealloc_arena(arena_t *arena)
{
	list_t *b_list = arena->block_list;
	node_t *bsearch = b_list->head;

	// traversing lists and deleting from last to first in the hierarchy
	while (bsearch) {
		node_t *bnext = bsearch->next;
		// obligatory conversion
		block_t *block = (block_t *)bsearch->info;
		node_t *msearch = block->miniblock_list->head;
		while (msearch) {
			node_t *mnext = msearch->next;
			miniblock_t *miniblock = (miniblock_t *)msearch->info;
			free(miniblock->rw_buffer);
			// impossible to be NULL
			free(miniblock);
			free(msearch);
			msearch = mnext;
		}
		free(block->miniblock_list);
		free(block);
		free(bsearch);
		bsearch = bnext;
	}
	free(b_list);
}

/* Blocks will be allocated in increasing order of their addresses and total
size. The block of the given address will be placed in list so that we
obtain a list of blocks sorted in increasing order by the start address
value. This function will also initialize the miniblock list where it's
possible. */
// adjacent blocks -> a single block
// it's possible to free miniblocks correctly, because no rw_buffer allocated
// when adding
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return;
	}
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return;
	}

	// for the first block, no sort is need
	node_t *search = arena->block_list->head, *prev, *next;
	uint64_t aux_address = 0;
	if (search) {
		block_t *block = (block_t *)search->info;
		aux_address = block->start_address;
	}
	// add first
	if (!search || address + size < aux_address) {
		// tmp alloc, then free
		block_t *block = malloc(sizeof(*block));
		if (!block) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		block->start_address = address, block->size = size;
		block->miniblock_list = dll_create(sizeof(miniblock_t));
		miniblock_t *miniblock = malloc(sizeof(*miniblock));
		if (!miniblock) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		miniblock->start_address = address, miniblock->size = size;
		miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
		add_nth_node(block->miniblock_list, 0, (const void *)miniblock);
		free(miniblock);
		add_nth_node(arena->block_list, 0, (const void *)block);
		// block was used only for copying data
		free(block);
		return;
	}
	// moving to a location where address is between 2 block addresses
	search = arena->block_list->head;
	block_t *block = (block_t *)search->info;
	prev = NULL;
	while (address > block->start_address && search) {
		prev = search;
		search = search->next;
		// update the block->address
		if (search)
			block = (block_t *)search->info;
	}
	// search	new_node	search->next

	// to work in add first concatenate case
	if (prev)
		search = prev;

	block = (block_t *)search->info;
	// add first, but in the blocks concatenate case, before search
	if (!prev) {
		if (address + size > block->start_address) {
			printf("This zone was already allocated.\n");
			return;
		}
		// add first at the search miniblock_list
		if (address + size == block->start_address) {
			block->size += size;
			block->start_address = address;
			miniblock_t *miniblock = malloc(sizeof(*miniblock));
			if (!miniblock) {
				fprintf(stderr, "Malloc failed!\n");
				exit(1);
			}
			miniblock->start_address = address, miniblock->size = size;
			miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
			add_nth_node(block->miniblock_list, 0, (const void *)miniblock);
			free(miniblock);
			return;
		}
	}

	// add last, after search
	if (!search->next) {
		if (block->start_address + block->size > address) {
			printf("This zone was already allocated.\n");
			return;
		}
		if (block->start_address + block->size == address) {
			block->size += size;
			miniblock_t *miniblock = malloc(sizeof(*miniblock));
			if (!miniblock) {
				fprintf(stderr, "Malloc failed!\n");
				exit(1);
			}
			miniblock->start_address = address, miniblock->size = size;
			miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
			uint64_t n = block->miniblock_list->num_nodes;
			add_nth_node(block->miniblock_list, n + 1, (const void *)miniblock);
			free(miniblock);
			return;
		}
		block_t *new_block = malloc(sizeof(*new_block));
		if (!new_block) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		new_block->start_address = address, new_block->size = size;
		new_block->miniblock_list = dll_create(sizeof(miniblock_t));
		miniblock_t *miniblock = malloc(sizeof(*miniblock));
		if (!miniblock) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		miniblock->start_address = address, miniblock->size = size;
		miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
		add_nth_node(new_block->miniblock_list, 0, (const void *)miniblock);
		free(miniblock);
		uint64_t n = arena->block_list->num_nodes;
		add_nth_node(arena->block_list, n + 1, (const void *)new_block);
		free(new_block);
		return;
	}

	// add mid
	next = search->next;
	node_t *next2 = next->next;
	block_t *block_n = (block_t *)next->info;
	if (block->start_address + block->size > address ||
		address + size > block_n->start_address) {
		printf("This zone was already allocated.\n");
		return;
	}
	/* Method: add the miniblock to block->miniblock_list
		miniblock->next = block_n->miniblock_list->head
		delete block_n node;
	*/

	// l-r concatenate
	if (block->start_address + block->size == address &&
		address + size == block_n->start_address) {
		block->size += size + block_n->size;
		// add miniblock
		miniblock_t *miniblock = malloc(sizeof(*miniblock));
		if (!miniblock) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		miniblock->start_address = address, miniblock->size = size;
		miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
		uint64_t n = block->miniblock_list->num_nodes;
		add_nth_node(block->miniblock_list, n + 1, (const void *)miniblock);
		free(miniblock);
		// updating the number of miniblocks
		block->miniblock_list->num_nodes += block_n->miniblock_list->num_nodes;
		// miniblock_list union
		node_t *msearch = block->miniblock_list->head;
		while (msearch->next)
			msearch = msearch->next;
		msearch->next = block_n->miniblock_list->head;
		block_n->miniblock_list->head->prev = msearch;
		// delete block_n
		free(block_n->miniblock_list);
		free(block_n);
		search->next = next2;
		if (next2)
			next2->prev = search;
		free(next);
		// updating the number of blocks
		arena->block_list->num_nodes--;
		return;
	}

	// left concatenate
	if (block->start_address + block->size == address) {
		block->size += size;
		miniblock_t *miniblock = malloc(sizeof(*miniblock));
		if (!miniblock) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		miniblock->start_address = address, miniblock->size = size;
		miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
		uint64_t n = block->miniblock_list->num_nodes;
		add_nth_node(block->miniblock_list, n + 1, (const void *)miniblock);
		free(miniblock);
		return;
	}

	// right concatenate
	if (address + size == block_n->start_address) {
		block_n->start_address = address;
		block_n->size += size;
		miniblock_t *miniblock = malloc(sizeof(*miniblock));
		if (!miniblock) {
			fprintf(stderr, "Malloc failed!\n");
			exit(1);
		}
		miniblock->start_address = address, miniblock->size = size;
		miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
		add_nth_node(block_n->miniblock_list, 0, (const void *)miniblock);
		free(miniblock);
		return;
	}

	// add mid-block, manually
	node_t *new_node = malloc(sizeof(*new_node));
	if (!new_node) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	block_t *new_block = malloc(sizeof(*new_block));
	if (!new_block) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	new_block->start_address = address, new_block->size = size;
	new_block->miniblock_list = dll_create(sizeof(miniblock_t));
	miniblock_t *miniblock = malloc(sizeof(*miniblock));
	if (!miniblock) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	miniblock->start_address = address, miniblock->size = size;
	miniblock->perm = DEF_PERM, miniblock->rw_buffer = malloc(size);
	add_nth_node(new_block->miniblock_list, 0, (const void *)miniblock);
	free(miniblock);
	// adding block info after creating miniblock list
	new_node->info = (void *)new_block;
	new_node->next = next;
	next->prev = new_node;
	search->next = new_node;
	new_node->prev = search;
	arena->block_list->num_nodes++;
}

// frees a miniblock
void free_m_node(node_t *m_node)
{
	miniblock_t *miniblock = (miniblock_t *)m_node->info;
	free(miniblock->rw_buffer);
	free(miniblock);
	free(m_node);
}

// frees a block
void free_b_node(node_t *b_node)
{
	block_t *block = (block_t *)b_node->info;
	free(block->miniblock_list);
	free(block);
	free(b_node);
}

// Deleting a miniblock from the memory; split if the miniblock is not at bounds
void free_block(arena_t *arena, const uint64_t address)
{
	node_t *bsearch = arena->block_list->head;
	bool valid = false;
	uint64_t idx = 0;
	while (bsearch) {
		block_t *block = (block_t *)bsearch->info;
		if (block->start_address <= address &&
			address <= block->start_address + block->size) {
			valid = true;
			break;
		}
		bsearch = bsearch->next, idx++;
	}
	if (valid) {
		block_t *block = (block_t *)bsearch->info;
		node_t *msearch;

		// delete a miniblock from start; if it's the only one, then rm block
		if (block->start_address == address) {
			// function to remove from list, not from memory
			msearch = remove_nth_node(block->miniblock_list, 0);
			miniblock_t *miniblock = (miniblock_t *)msearch->info;
			block->start_address += miniblock->size;
			block->size -= miniblock->size;
			free_m_node(msearch);
			if (block->miniblock_list->num_nodes == 0) {
				bsearch = remove_nth_node(arena->block_list, idx);
				free_b_node(bsearch);
			}
			return;
		}
		msearch = block->miniblock_list->head;
		miniblock_t *miniblock = (miniblock_t *)msearch->info;
		uint64_t idx2 = 0, left_size = 0;
		while (miniblock->start_address != address) {
			if (!msearch->next) {
				valid = false;
				break;
			}
			left_size += miniblock->size;
			msearch = msearch->next, idx2++;
			miniblock = (miniblock_t *)msearch->info;
		}
		if (valid) {
			// delete end miniblock
			if (!msearch->next) {
				msearch = remove_nth_node(block->miniblock_list, idx2);
				block->size -= miniblock->size;
				free_m_node(msearch);
				return;
			}
			// delete mid and split into 2 blocks
			node_t *mprev = msearch->prev, *mnext = msearch->next;
			msearch = remove_nth_node(block->miniblock_list, idx2);
			uint64_t right_size = block->size - left_size - miniblock->size;
			free_m_node(msearch);
			uint64_t total_nodes = block->miniblock_list->num_nodes;
			block->size = left_size, block->miniblock_list->num_nodes = idx2;
			mprev->next = NULL, mnext->prev = NULL;
			block_t *new_block = malloc(sizeof(*new_block));
			if (!new_block) {
				fprintf(stderr, "Malloc failed!\n");
				exit(1);
			}
			miniblock_t *mb_next = (miniblock_t *)mnext->info;
			new_block->start_address = mb_next->start_address;
			new_block->size = right_size;
			new_block->miniblock_list = dll_create(sizeof(miniblock_t));
			new_block->miniblock_list->num_nodes = total_nodes - idx2;
			new_block->miniblock_list->head = mnext;
			add_nth_node(arena->block_list, idx + 1, (const void *)new_block);
			free(new_block);
		} else {
			printf("Invalid address for free.\n");
		}
	} else {
		printf("Invalid address for free.\n");
	}
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	node_t *bsearch = arena->block_list->head;
	bool ok = false;
	block_t *block;
	while (bsearch) {
		block = (block_t *)bsearch->info;
		if (block->start_address <= address && block->start_address +
		block->size > address) {
			ok = true;
			break;
		}
		bsearch = bsearch->next;
	}
	if (ok) {
		ok = false;
		node_t *msearch = block->miniblock_list->head;
		miniblock_t *miniblock;
		while (msearch) {
			miniblock = (miniblock_t *)msearch->info;
			if (miniblock->start_address <= address && address <
				miniblock->start_address + miniblock->size) {
				ok = true;
				break;
			}
			msearch = msearch->next;
		}
		if (ok) {
			bool good_size = true;
			node_t *mnode = msearch;
			miniblock_t *mb_tmp;
			uint64_t check_size = 0;
			while (mnode) {
				mb_tmp = (miniblock_t *)mnode->info;
				check_size += mb_tmp->size;
				uint8_t perm = mb_tmp->perm;
				// no read permission
				if (perm < 4) {
					printf("Invalid permissions for read.\n");
					return;
				}
				mnode = mnode->next;
			}
			if (check_size < size) {
				printf("Warning: size was bigger than the block size. ");
				printf("Reading %lu characters.\n", check_size);
				good_size = false;
			}
			if (good_size)
				check_size = size;
			// Writing method:
			uint64_t idx = 0; //index for data string
			mnode = msearch;
			while (idx < check_size && mnode) {
				// bigger addres for miniblock possibility
				miniblock = (miniblock_t *)mnode->info;
				uint64_t j = 0; // buffer index
				if (address > miniblock->start_address)
					j = address - miniblock->start_address;
				while (j < miniblock->size) {
					// case for single words
					if (idx == check_size) {
						printf("\n");
						return;
					}
					printf("%c", ((char *)miniblock->rw_buffer)[j]);
					j++, idx++;
				}
				mnode = mnode->next;
			}
			printf("\n");
		} else {
			printf("Invalid address for read.\n");
		}
	} else {
		printf("Invalid address for read.\n");
	}
}

void write(arena_t *arena, const uint64_t address,
		   const uint64_t size, char *data)
{
	node_t *bsearch = arena->block_list->head;
	bool ok = false;
	block_t *block;
	while (bsearch) {
		block = (block_t *)bsearch->info;
		if (block->start_address <= address && block->start_address +
		block->size > address) {
			ok = true;
			break;
		}
		bsearch = bsearch->next;
	}
	if (ok) {
		ok = false;
		node_t *msearch = block->miniblock_list->head;
		miniblock_t *miniblock;
		while (msearch) {
			miniblock = (miniblock_t *)msearch->info;
			if (miniblock->start_address <= address && address <
				miniblock->start_address + miniblock->size) {
				ok = true;
				break;
			}
			msearch = msearch->next;
		}
		if (ok) {
			bool good_size = true;
			node_t *mnode = msearch;
			miniblock_t *mb_tmp;
			uint64_t check_size = 0;
			while (mnode) {
				mb_tmp = (miniblock_t *)mnode->info;
				check_size += mb_tmp->size;
				uint8_t perm = mb_tmp->perm;
				// no write permission
				if (perm != 2 && perm != 3 && perm != 6 && perm != 7) {
					printf("Invalid permissions for write.\n");
					return;
				}
				mnode = mnode->next;
			}
			if (check_size < size) {
				printf("Warning: size was bigger than the block size. ");
				printf("Writing %lu characters.\n", check_size);
				good_size = false;
			}
			if (good_size)
				check_size = size;
			// Writing method:
			uint64_t idx = 0; //index for data string
			mnode = msearch;
			while (idx < check_size) {
				// bigger addres for miniblock possibility
				miniblock = (miniblock_t *)mnode->info;
				uint64_t j = 0; // buffer index
				if (address > miniblock->start_address)
					j = address - miniblock->start_address;
				while (j < miniblock->size) {
					((char *)miniblock->rw_buffer)[j] = data[idx];
					j++, idx++;
				}
				mnode = mnode->next;
			}
		} else {
			printf("Invalid address for write.\n");
		}
	} else {
		printf("Invalid address for write.\n");
	}
}

// unsigned int mask to char* perm; result will be freed after used
// rule of convertion is the same as permissions of files
char *perm(uint8_t mask)
{
	char *p = malloc(5);
	if (!p) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	switch (mask) {
	case 0:
		strcpy(p, "---");
		break;
	case 1:
		strcpy(p, "--X");
		break;
	case 2:
		strcpy(p, "-W-");
		break;
	case 3:
		strcpy(p, "-WX");
		break;
	case 4:
		strcpy(p, "R--");
		break;
	case 5:
		strcpy(p, "R-X");
		break;
	case 6:
		strcpy(p, "RW-");
		break;
	case 7:
		strcpy(p, "RWX");
		break;
	}
	return p;
}

// simplified memory visualization
void pmap(const arena_t *arena)
{
	uint64_t free_mem = arena->arena_size, num_miniblocks = 0;
	// uint64_t can be printed in hex format using lx format specifier
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	node_t *bsearch = arena->block_list->head;

	// calculating free_memory and number of miniblocks
	// free_size is calcultated: total_memory - sum(block_i_size)
	while (bsearch) {
		block_t *block = (block_t *)bsearch->info;
		num_miniblocks += block->miniblock_list->num_nodes;
		free_mem -= block->size;
		bsearch = bsearch->next;
	}

	printf("Free memory: 0x%lX bytes\n", free_mem);
	printf("Number of allocated blocks: %lu\n", arena->block_list->num_nodes);
	if (arena->block_list->num_nodes != 0)
		printf("Number of allocated miniblocks: %lu\n\n", num_miniblocks);
	else
		printf("Number of allocated miniblocks: %lu\n", num_miniblocks);
	int i = 1, j;
	// i = index of block node, j = index of miniblock node
	bsearch = arena->block_list->head;

	// traversing lists and showing the info in the required format
	while (bsearch) {
		block_t *block = (block_t *)bsearch->info;
		printf("Block %d begin\n", i);
		printf("Zone: 0x%lX - 0x%lX\n", block->start_address,
			   block->start_address + block->size);
		node_t *msearch = block->miniblock_list->head;
		j = 1;
		while (msearch) {
			miniblock_t *miniblock = (miniblock_t *)msearch->info;
			printf("Miniblock %d:\t\t0x%lX\t\t-", j, miniblock->start_address);
			printf("\t\t0x%lX\t\t", miniblock->start_address + miniblock->size);
			char *p = perm(miniblock->perm);
			printf("| %s\n", p);
			// p was allocated in perm function, but not freed
			free(p);
			msearch = msearch->next;
			j++;
		}
		if (bsearch->next)
			printf("Block %d end\n\n", i);
		else
			printf("Block %d end\n", i);
		bsearch = bsearch->next;
		i++;
	}
}

void mprotect(arena_t *arena, uint64_t address, uint8_t *permission)
{
	node_t *bsearch = arena->block_list->head;
	bool ok = false;
	block_t *block;
	while (bsearch) {
		// valid address since bsearch != NULL
		block = (block_t *)bsearch->info;
		if (block->start_address <= address && address <
			block->start_address + block->size) {
			ok = true;
			break;
		}
		bsearch = bsearch->next;
	}
	if (!ok) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	ok = false;
	node_t *msearch = block->miniblock_list->head;
	miniblock_t *miniblock;
	while (msearch) {
		miniblock = (miniblock_t *)msearch->info;
		if (miniblock->start_address == address) {
			// permission change
			miniblock->perm = *permission;
			ok = true;
			break;
		}
		msearch = msearch->next;
	}
	if (!ok) {
		printf("Invalid address for mprotect.\n");
		return;
	}
}
