#ifndef HEAP_H
# define HEAP_H

# ifdef __cplusplus
extern "C" {
# endif

# include <stdint.h>

class heap_node;

class heap {
public:
  heap_node *min;
  uint32_t size;
  int32_t (*compare)(const void *key, const void *with);
  void (*datum_delete)(void *);
};

void heap_init(heap *h,
               int32_t (*compare)(const void *key, const void *with),
               void (*datum_delete)(void *));
void heap_delete(heap *h);
heap_node *heap_insert(heap *h, void *v);
void *heap_peek_min(heap *h);
void *heap_remove_min(heap *h);
int heap_combine(heap *h, heap *h1, heap *h2);
int heap_decrease_key(heap *h, heap_node *n, void *v);
int heap_decrease_key_no_replace(heap *h, heap_node *n);

# ifdef __cplusplus
}
# endif

#endif
