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

#define THREAD_COUNT 2
#define ELEMENT_COUNT 10000

// GLOBALS ACCESSABLE

unsigned long sum;
unsigned long psum[THREAD_COUNT];
unsigned long A[ELEMENT_COUNT];

// int accum;

typedef struct pin_data {
    // unsigned long addr_sum;
    unsigned long addr_psum; 
    unsigned long addr_A; 
    // int sum_length; 
    int psum_length; 
    int A_length; 
    int num_cores;  
} pin_data_t;


pin_data_t ret_pin_data() {
    pin_data_t data;
    // data.addr_sum = (unsigned long) &sum;
    data.addr_psum = (unsigned long) &psum;
    data.addr_A = (unsigned long) &A;
    // data.sum_length = 1;
    data.psum_length = THREAD_COUNT;
    data.A_length = ELEMENT_COUNT;
    data.num_cores = THREAD_COUNT;
    return data;
}



// // PTHREAD GLOBALS
// pthread_mutex_t mutex;
// pthread_barrier_t barrier;

// void *Bash(void *threadid)
// {
//     // pthread_mutex_lock(&mutex); 
//     int try = 10000; // change this value to change test length
  
//     while (try--) { 	
//         pthread_mutex_lock(&mutex); 
//         accum = accum + 1;
//         pthread_mutex_unlock(&mutex); 
//     } 
//     // pthread_mutex_unlock(&mutex);
//     pthread_exit(NULL);
// }

void *SumParallel(void *threadid)
{
    long id = (long)threadid; 
    int i; 
    psum[id] = 0; 

    // for (i = 0; i < (ELEMENT_COUNT/THREAD_COUNT); i++) {
    //     psum[id] += A[id * (ELEMENT_COUNT/THREAD_COUNT) + i]; 
    // }

    for (i = 0; i < (ELEMENT_COUNT/THREAD_COUNT); i++) {
        psum[id] += A[id * (ELEMENT_COUNT/THREAD_COUNT) + i]; 
    }

    // pthread_mutex_lock(&mutex); 
    // sum = sum + psum[id]; 
    // pthread_mutex_unlock(&mutex); 

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


void *main_func(void *arg) {
    // Start timer
	// struct timeval start_time, end_time;
	// gettimeofday(&start_time, NULL);

    // spawn threads to perform parallel summations
    // for(thread = 0; thread < THREAD_COUNT; thread++ ) {
    //     pthread_create(&threads[thread], NULL, Bash, (void *)thread);
    // }

    pthread_t threads[THREAD_COUNT];
	long thread;

    int i; 
    // for(i = 0; i < ELEMENT_COUNT; i++) {
    //     printf("i: %d, A[i]: %li\n", i, A[i]); 
    // }
    for(i = 0; i < THREAD_COUNT; i++) {
        printf("i: %d, psum[i]: %li\n", i, psum[i]); 
    }

    printf("\n[+] Multithreaded calculation started. \n[*] Calculating...");


    for(thread = 0; thread < THREAD_COUNT; thread++ ) {
        pthread_create(&threads[thread], NULL, SumParallel, (void *)thread);
    }

    // wait for other threads
    void *status;
    for (thread=0; thread < THREAD_COUNT; thread++) {
        pthread_join(threads[thread], &status);
    }
    // printf("runtime = %0.1f (microsec) \n", 
	// 	    (end_time.tv_sec - start_time.tv_sec)*1e6 + (end_time.tv_usec - start_time.tv_usec) );

    
	// gettimeofday(&end_time, NULL);

    printf("\n[+] Multithreaded calculation finished \n[+] Duration: \n\n");

    // for(i = 0; i < ELEMENT_COUNT; i++) {
    //     printf("i: %d, A[i]: %li\n", i, A[i]); 
    // }
    printf("\n"); 
    for(i = 0; i < THREAD_COUNT; i++) {
        printf("i: %d, psum[i]: %li\n", i, psum[i]); 
        sum += psum[i]; 
    }
    // Print the summation
    // printf("accum      = %d\n", accum);
    printf("sum        = %li\n", sum); 


    return 0;
}


int main()
{
    printf("Sum Parallel\n"); 

	// pthread_mutex_init(&mutex, NULL);
	// pthread_barrier_init(&barrier, NULL, THREAD_COUNT);

    // Initialize matrix A 
    unsigned long temp = 0; 
    int i;
    for (i=0; i<ELEMENT_COUNT; i=i+1) {
        A[i] = (i+1);
        temp += 1; 
    }	
    printf("\ncorrect sum: %li\n", temp); 
	
	// Execute the entire main section in a separate thread for PIN
    pthread_t npid1;
    pthread_create(&npid1, NULL, main_func, NULL);
    pthread_join(npid1, NULL);

	// pthread_barrier_destroy(&barrier);
	// pthread_mutex_destroy(&mutex);
    
    pthread_exit(NULL);	
}

