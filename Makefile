rlg_exe: main.o dgen.o heap.o pathfinding.o
	gcc main.o dgen.o heap.o pathfinding.o -o rlg_exe

main.o: main.c dgen/dgen.h dgen/pathfinding.h
	gcc -c main.c -Wall -Werror -ggdb -o main.o

dgen.o: dgen/dgen.c dgen/dgen.h pqueue/heap.h
	gcc -c dgen/dgen.c -Wall -Werror -ggdb -o dgen.o

heap.o: pqueue/heap.c pqueue/heap.h macros.h
	gcc -c pqueue/heap.c -Wall -Werror -ggdb -o heap.o

pathfinding.o: dgen/pathfinding.c dgen/pathfinding.h pqueue/heap.h dgen/dgen.h
	gcc -c dgen/pathfinding.c -Wall -Werror -ggdb -o pathfinding.o

clean:
	rm -f rlg_exe main.o dgen.o heap.o pathfinding.o *~
