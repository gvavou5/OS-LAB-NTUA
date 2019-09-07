all: run1 newexecute
	
main.o: main.c
	gcc -Wall -c main.c

run1: main.o zing.o
	gcc main.o zing.o -o run1

zing2.o: zing2.c
	gcc -Wall -c zing2.c

newexecute: main.o zing2.o
	gcc main.o zing2.o -o newexecute

execute2: newexecute
	./newexecute

execute: run1
	./run1
