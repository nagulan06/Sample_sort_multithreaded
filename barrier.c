// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include "barrier.h"

barrier*
make_barrier(int nn)
{
    int rv;
    barrier* bb = malloc(sizeof(barrier));
    assert(bb != 0);

    //initialize pthread locks and condition variables
    rv = pthread_mutex_init(&(bb->barrier), NULL);
    if (rv == -1) {
        perror("mutex_init(barrier)");
        abort();
    }

    rv = pthread_mutex_init(&(bb->lock), NULL);
    if (rv == -1) {
        perror("mutex_init(lock)");
        abort();
    }
    rv = pthread_cond_init(&(bb->condvar), NULL);
    if (rv == -1) {
        perror("cond_init(cond_var)");
        abort();
    }
    //initialize count to total number of process and seen to 0
    bb->seen = nn;
//    bb->count = nn;

    return bb;
}

void
barrier_wait(barrier* bb)
{
    int rv;
    //use lock and let the threads individually update the seen value.
   rv = pthread_mutex_lock(&(bb->lock));
   if(rv!=0)
       perror("mutex_lock of lock");
    bb->seen--;
    int seen = bb->seen;
    pthread_mutex_unlock(&(bb->lock));
    if(rv!=0)
       perror("mutex_unlock of lock");
 

    pthread_mutex_lock(&(bb->barrier));
    if(rv!=0)
       perror("mutex_unlock of barrier");

    if(seen == 0)
    {
  //      printf("Seen value is 0 now: %d\n", seen);
        pthread_cond_broadcast(&(bb->condvar));
    pthread_mutex_unlock(&(bb->barrier));
    }
    else
        pthread_cond_wait(&(bb->condvar), &(bb->barrier));

//    printf("This statement should be printed only after seen is 0: %d", seen);
    pthread_mutex_unlock(&(bb->barrier));

}

void
free_barrier(barrier* bb)
{
   pthread_mutex_destroy(&(bb->lock)); 
   pthread_mutex_destroy(&(bb->barrier)); 
    free(bb);
}

