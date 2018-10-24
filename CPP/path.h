#ifndef PATH_H
# define PATH_H

# define HARDNESS_PER_TURN 85

typedef struct dungeon dungeon;

void dijkstra(dungeon *d);
void dijkstra_tunnel(dungeon *d);

#endif
