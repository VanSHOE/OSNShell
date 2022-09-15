# Compile all C files

run.out: *.h *.c
	gcc -ggdb -o run.out *.c
