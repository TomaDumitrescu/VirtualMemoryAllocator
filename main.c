/*
	User interface program in C console
*/
#include "vma.h"

// interprets the string and transform it to the corresponding numerical mask
uint8_t permission_convert(char *s)
{
	char *tmp;
	tmp = strtok(s, " |\n");
	bool r = false, w = false, x = false;
	while (tmp) {
		if (strcmp(tmp, "PROT_NONE") == 0)
			r = false, w = false, x = false;
		if (strcmp(tmp, "PROT_READ") == 0)
			r = true;
		if (strcmp(tmp, "PROT_WRITE") == 0)
			w = true;
		if (strcmp(tmp, "PROT_EXEC") == 0)
			x = true;
		tmp = strtok(NULL, " |\n");
	}
	int8_t conv = 0;
	//boolean variables kept track of the permissions, no collisions when adding
	if (x)
		conv += 1;
	if (w)
		conv += 2;
	if (r)
		conv += 4;
	return conv;
}

int main(void)
{
	char *command = malloc(MAX_COMMAND);
	bool arena_alloc = false;
	if (!command) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	char *prot = malloc(MAX_COMMAND);
	if (!prot) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	char *text = malloc(MAX_TEXT);
	if (!text) {
		fprintf(stderr, "Malloc failed!\n");
		exit(1);
	}
	uint64_t p1, p2;
	arena_t arena;

	// infinite loop for command session
	while (true) {
		fscanf(stdin, "%s", command);

		// connection between the command and the functions from vma.h
		if (strcmp(command, "ALLOC_ARENA") == 0) {
			arena_alloc = true;
			fscanf(stdin, "%lu", &p1);
			alloc_arena(p1, &arena);
		} else if (strcmp(command, "DEALLOC_ARENA") == 0) {
			if (!arena_alloc)
				break;
			dealloc_arena(&arena);
			break;
		} else if (strcmp(command, "ALLOC_BLOCK") == 0) {
			if (!arena_alloc)
				break;
			fscanf(stdin, "%lu%lu", &p1, &p2);
			alloc_block(&arena, p1, p2);
		} else if (strcmp(command, "FREE_BLOCK") == 0) {
			if (!arena_alloc)
				break;
			fscanf(stdin, "%lu", &p1);
			free_block(&arena, p1);
		} else if (strcmp(command, "READ") == 0) {
			if (!arena_alloc)
				break;
			fscanf(stdin, "%lu%lu", &p1, &p2);
			read(&arena, p1, p2);
		} else if (strcmp(command, "WRITE") == 0) {
			if (!arena_alloc)
				break;
			fscanf(stdin, "%lu%lu", &p1, &p2);
			fgetc(stdin);
			uint64_t i = 0;
			while (i < p2) {
				text[i] = fgetc(stdin);
				i++;
			}
			write(&arena, p1, p2, text);
		} else if (strcmp(command, "PMAP") == 0) {
			if (!arena_alloc)
				break;
			pmap(&arena);
		} else if (strcmp(command, "MPROTECT") == 0) {
			if (!arena_alloc)
				break;
			fscanf(stdin, "%lu", &p1);
			fgets(prot, MAX_COMMAND, stdin);
			uint8_t perm = permission_convert(prot);
			//passing the address of perm
			mprotect(&arena, p1, &perm);
		} else {
			fprintf(stdout, "Invalid command. Please try again.\n");
		}
	}
	free(command);
	free(prot);
	free(text);
	return 0;
}
