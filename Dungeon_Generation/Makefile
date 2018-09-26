Dungeon_Generation: main.o Dungeon_Generation.o heap.o
	gcc main.o Dungeon_Generation.o heap.o -o Dungeon_Generation

main.o: main.c Dungeon_Generation.h
	gcc -c main.c -Wall -Werror -ggdb -o main.o

Dungeon_Generation.o: Dungeon_Generation.c Dungeon_Generation.h heap.h
	gcc -c Dungeon_Generation.c -Wall -Werror -ggdb -o Dungeon_Generation.o

heap.o: heap.c heap.h macros.h
	gcc -c heap.c -Wall -Werror -ggdb -o heap.o

clean:
	rm -f Dungeon_Generation main.o Dungeon_Generation.o heap.o *~
