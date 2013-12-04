#include <errno.h>
#include <stdio.h>

#define HEAP_SIZE 4096

char heap[HEAP_SIZE];

caddr_t
_sbrk(int incr)
{
   static int heap_index = 0;
   int start_index;

   start_index = heap_index;

   if ((heap_index + incr) > HEAP_SIZE) {
      errno = ENOMEM;
      return (caddr_t)-1;
   }
   heap_index += incr;

   return &heap[start_index];
}
