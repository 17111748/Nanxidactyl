/*  sumParallel.c
    Peter F. Klemperer
    April 18, 2011

    Minimal pthreads sumParrallel example with 100 threads
    performing parallel summation of the thread id's
    with sequential reduction.

    Code is taken from POSIX Threads Programming, 
    written by Blaise Barney of LLNL.

	//pthread_mutex_unlock(&mutex);
	//pthread_mutex_lock(&mutex);
	//pthread_barrier_wait(&barrier);
*/
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>

#define THREAD_COUNT 1
#define ELEMENT_COUNT 10000

// GLOBALS ACCESSABLE
double sum;
double psum[THREAD_COUNT];
double A[ELEMENT_COUNT];

int accum;

// PTHREAD GLOBALS
pthread_mutex_t mutex;
pthread_barrier_t barrier;

void *Bash(void *threadid)
{
    // pthread_mutex_lock(&mutex); 
    int try = 10000; // change this value to change test length
  
    while (try--) { 	
        pthread_mutex_lock(&mutex); 
        accum = accum + 1;
        pthread_mutex_unlock(&mutex); 
    } 
    // pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

void *SumParallel(void *threadid)
{
    long id = (long)threadid; 
    int i; 
    psum[id] = 0; 

    for (i = 0; i < (ELEMENT_COUNT/THREAD_COUNT); i++) {
        psum[id] += A[id * (ELEMENT_COUNT/THREAD_COUNT) + i]; 
    }

    pthread_mutex_lock(&mutex); 
    sum = sum + psum[id]; 
    pthread_mutex_unlock(&mutex); 

    /*int remain = THREAD_COUNT; 
    int half; 
    do {
        pthread_barrier_wait(&barrier); 
        half = (remain + 1)/2; 
        if (id < (remain/2)) {
            psum[id] = psum[id] + psum[id+half]; 
        }
        remain = half; 
    } while (remain > 1);  
    */  
    return NULL; 
}

int main()
{
    pthread_t threads[THREAD_COUNT];
	long thread;

	pthread_mutex_init(&mutex, NULL);
	pthread_barrier_init(&barrier, NULL, THREAD_COUNT);

    // Initialize matrix A 
    double temp = 0; 
    int i;
    for (i=0; i<ELEMENT_COUNT; i=i+1) {
        A[i] = i;
        temp += i; 
    }	
    printf("correct sum: %f\n", temp); 
	// Start timer
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);

    // spawn threads to perform parallel summations
    // for(thread = 0; thread < THREAD_COUNT; thread++ ) {
    //     pthread_create(&threads[thread], NULL, Bash, (void *)thread);
    // }
    for(thread = 0; thread < THREAD_COUNT; thread++ ) {
        pthread_create(&threads[thread], NULL, SumParallel, (void *)thread);
    }

    // wait for other threads
    void *status;
    for (thread=0; thread < THREAD_COUNT; thread++ ) {
        pthread_join(threads[thread], &status);
    }
 
    //sum = psum[0]; 
    
	gettimeofday(&end_time, NULL);

    // Print the summation
    printf("accum      = %d\n", accum);
    printf("sum        = %f,  ", sum); 
	printf("runtime = %0.1f (microsec) \n", 
		    (end_time.tv_sec - start_time.tv_sec)*1e6 + (end_time.tv_usec - start_time.tv_usec) );

	pthread_barrier_destroy(&barrier);
	pthread_mutex_destroy(&mutex);
    
    pthread_exit(NULL);	
}
