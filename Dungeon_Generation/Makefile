Dungeon_Generation: main.o Dungeon_Generation.o
	gcc main.o Dungeon_Generation.o -o Dungeon_Generation

main.o: main.c Dungeon_Generation.h
	gcc -c main.c -Wall -Werror -ggdb -o main.o

Dungeon_Generation.o: Dungeon_Generation.c Dungeon_Generation.h
	gcc -c Dungeon_Generation.c -Wall -Werror -ggdb -o Dungeon_Generation.o

clean:
	rm -f Dungeon_Generation main.o Dungeon_Generation.o *~
