build:
		gcc -o vma *.c -Wall -Wextra -std=c99
run_vma:
		./vma
clean:
		rm -f *.o vma