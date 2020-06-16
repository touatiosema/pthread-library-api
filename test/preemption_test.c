#include <stdio.h>
#include <assert.h>
#include "thread.h"

// ce test est pour illustrer le fonctionement de preemption.
// la compilation doit etre faite manuelement, je ne l'ai pas rajouter au Makefile.

static void * thfunc(void *id)
{
  while(1){
    fprintf(stderr, "in thefunc %s\n", (char*)id);
  }
}

int main()
{
  thread_t th1, th2;
  void *res;
  int err;

  err = thread_create(&th1, thfunc, "fils1");
  assert(!err);
  err = thread_create(&th2, thfunc, "fils2");
  assert(!err);
  set_priority(&th1, MAX_PRIORITY);
  err = thread_join(th1, &res);
  assert(!err);
  for(;;){
      fprintf(stderr, "in main\n");
  }
  printf("main terminé\n");
  return 0;
}