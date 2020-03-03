#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>

//#include "float_vec.c"
#include "float_vec.h"
#include "barrier.h"
#include "utils.h"
//#include "utils.c"
//#include "barrier.c"

struct th
{
    int pnum;
    float* data;
    long size;
    int P;
    long* sizes;
    floats* samples;
    barrier* bb;
}th;

// comparator function for qsort function.
int comparator (const void * a, const void * b)
{
  float fa = *(const float*) a;
  float fb = *(const float*) b;
  return (fa > fb) - (fa < fb);
}
void
qsort_floats(floats* xs)
{
    qsort(xs->data, xs->size, sizeof(float), comparator);
}

floats*
sample()
{
    int P = th.P;
    floats* sample;  
    sample  = make_floats(3*(P-1));
      for(int i=0; i<3*(P-1); i++)
        {
        int r = rand()%th.size;
        sample->data[i] = th.data[r]; 
        }
    return (sample);
}
  
  
void*
sort_worker(void*arg)
{
    int pnum = *((int*)arg);

    //local array.
    floats* xs;
    xs = make_floats(0);

    for(int i=0; i<th.size; i++)
    {
        if(th.data[i]>=th.samples->data[pnum] && th.data[i]<th.samples->data[pnum+1])
            floats_push(xs, th.data[i]);
    }
    th.sizes[pnum] = xs->size;
// data for the partition is copied into the local array.

        printf("%d: start %.04f, count %ld\n", pnum, th.samples->data[pnum], xs->size);
    
    //Sort the local array.
    qsort_floats(xs);

    barrier_wait(th.bb);  //wait for all the process to update the sizes array as each process would need the result of its previous process' sizes value.
    int start = 0;
    int end = 0; 
    for(int i=0; i<pnum; i++)
        start = start + th.sizes[i];
    for(int i=0; i<=pnum; i++)
        end = end + th.sizes[i];

    //Copy the local array to the appropriate place in the global array.
    int k = 0;
    for(int i=start; i<end; i++)
    {
        th.data[i] = xs->data[k];
        k++;
    }

    free_floats(xs);
    return NULL;
}

void
run_sort_workers()
{
    //pthread_create cerates threads and the sort_worker function is called
    int arr[th.P];
    pthread_t threads[th.P];
    for(int i=0; i<th.P; i++)
    {
        arr[i] = i;
        pthread_create(&threads[i], NULL, &sort_worker, &arr[i]);
    }
    //join the threads
    for(int i=0; i<th.P; i++)
    {
        pthread_join(threads[i], NULL);
    }

   }

void
sample_sort()
{
    int P = th.P;
    floats* temp;
    temp = sample();
    //temp now contains all the 3*(P-1) elements.
    
    th.samples = make_floats(P+1);  //th.samples is initiased of length P+1 using make_floats.

    qsort_floats(temp); //sort temp
    
    th.samples->data[0] = 0;
    th.samples->data[P] = INT_MAX;
    //after sorting, the medain values are stored in samples.
    int j=1;
    for(int i=1; i<3*(P-1); i=i+3)
    {
        th.samples->data[j] = temp->data[i];
        j++;
    }
    run_sort_workers();
   
    free_floats(temp);
    free_floats(th.samples);
    }

int
main(int argc, char* argv[])
{    if (argc != 3) {
        printf("Usage:\n");
        printf("\t%s P data.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    th.P = P;
    const char* fname = argv[2];
    struct stat buf;
    seed_rng();

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    fstat(fd, &buf);

    //map the file in the virtual address space and the address of the mapping is returned in the void* file.
    void* file = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(file == MAP_FAILED)
    {
       perror("error in mmap: ");
        return(1);
    }

    //The count value and the float data are read from the file and stored in the variable count and data pointer respectively.
    long count = *(long*)file;
    float* data = (float*)(file+8);

    th.size = count;
    th.data = data;

    long sizes_bytes = P * sizeof(long);
//    long* sizes = malloc(sizes_bytes); // TODO: This should be shared memory.

    long* sizes = malloc(sizes_bytes);
    barrier* bb = make_barrier(P);

    th.sizes = sizes;
    th.bb = bb;
    sample_sort();

    free_barrier(bb);

    // TODO: Clean up resources.

    //(void) file;
    munmap(sizes, sizeof(long));
 //   check_rv(rv);
    munmap(file, buf.st_size);
  //  check_rv(rv);

    return 0;

 }
