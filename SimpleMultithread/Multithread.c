 /********
 *
 *      File Name: Multithread.c
 *
 *      Name of team members:
 *
 *      Marcel Riera
 *      Anthony Sanchez-Ayra.
 *
 *
 *      How to run the program:
 *
 *      To run this program please make sure that the makefile
 *      is in the same directory as this source file. 
 *
 *      To compile this file just type make.
 *
 *      Another way to compile the file is by typing the
 *      following commands:
 *
 *      gcc Multithread.c  -o Multithread.out -lpthread
 *
 *      Afterwards to execute the program please use the
 *      command below followed by a number.
 *
 *      ./Multithread.out [ ENTER NUMBER HERE ]
 *
 *      For example:
 *
 *      ./Multithread.out 1
 *
 *      If the input you provide is not followed by a number
 *      the program will give you an error and describe its
 *      expected input.
 *
 *      There are two ways to run this program:
 *
 *      - Without synchronization of the pthreads:
 *
 *      To run the program without synchronization make sure to remove 
 *      the preproccessed value PTHREAD_SYNC by commeting it out.
 *
 *      // #define PTHREAD_SYNC 
 *
 *      - With synchronization of the pthreads:
 * 
 *      To run the program with synchronization make sure that the preproccess 
 *      value PTHREAD_SYNC is defined.
 * 
 *      #define PTHREAD_SYNC 
 * 
 *      When toggling with synchronization please make sure that you compile the program.
 *
 ******/

#include <stdio.h> // Includes standard input library for C.
#include <stdlib.h> // Includes the standard library for C.
#include <pthread.h> // Includes the pthread library to initialize thread variables.
#include <unistd.h> // Needed on my machine to use usleep().

#define PTHREAD_SYNC  // ENABLED SYNCHRONIZATION - Comment out to disable

#define INVALID_ARGUMENTS -1 // Whenever an invalid argument is found, -1 is returned.
#define EXPECTED_ARGUMENTS 2 // Number of expected arguments from command line input.
#define ASCII_ZERO 48  // The ascii value that corresponds to 0.
#define ASCII_NINE 57 // The ascii value that corresponds to 9.

// Function prototypes
int validateArguments(int argc, char *argv[]);
void *simpleThread(void *threadArgs);

// Global pthread variables for the mutex and barrier.
#ifdef PTHREAD_SYNC
pthread_mutex_t mutexSum;
pthread_barrier_t myBarrier;
#endif

// The shared memory variable the pthreads will be modifying.
int sharedVariable = 0;

int main(int argc, char *argv[]) 
{
	const int THREAD_AMOUNT = validateArguments(argc, argv); // Gather cmd line input

	if (THREAD_AMOUNT == INVALID_ARGUMENTS) 
    {
	  	printf("Invalid arguments: command line expects 1 argument > 0\n");
	   	printf("e.g. \"./Multithread.out 4\"\n");
    	return EXIT_FAILURE;
	}
        
    // Initializes the variables necessary for synchronization.
    #ifdef PTHREAD_SYNC
    pthread_mutex_init ( &mutexSum, NULL );
    pthread_barrier_init ( &myBarrier, NULL, THREAD_AMOUNT );
    #endif

	pthread_t *threadIDs = calloc(THREAD_AMOUNT, sizeof(pthread_t)); // Allocate array of thread IDs
	int *threadNumbers = calloc(THREAD_AMOUNT, sizeof(int));

	setbuf(stdout, NULL); // Delete stdout buffer, similar to fflush after printf

    int curThread;

    // Create and run threads
	for (curThread = 0; curThread < THREAD_AMOUNT; curThread++)
    {
	threadNumbers[curThread] = curThread;
    	if (pthread_create(&threadIDs[curThread], NULL, simpleThread, (void*) &threadNumbers[curThread]) != 0) 
    	{
    		printf("Error: Thread could not be created.");
    		return(EXIT_FAILURE);
    	}
    }

    // Wait for threads to be done, and join
	for (curThread = 0; curThread < THREAD_AMOUNT; curThread++)
    {
    	if (pthread_join(threadIDs[curThread], NULL) != 0) 
    	{
    		printf("Error: Thread could not be joined.");
    		return(EXIT_FAILURE);
    	}
    }

	free(threadIDs);

    // Removes the variables used for synchronization.
    #ifdef PTHREAD_SYNC
	pthread_mutex_destroy ( &mutexSum );
	pthread_barrier_destroy ( &myBarrier );
    #endif

	return EXIT_SUCCESS;
}

/*********** validateArguments ( length, input )***
 *   |
 *   |   Function: validateArguments
 *   |
 *   |   Parameters:
 *   |
 *   |   length: The length of the amount of
 *   |           inputs entered by the user.
 *   |
 *   |   input: The input that the user entered
 *   |          through the command line.
 *   |
 *   |   return: validatedInput - An integer
 *   |           that specifies if the input
 *   |           given by the user was valid or
 *   |           not.
 *   |
 **********************************************/
int validateArguments( int length, char *input[] ) 
{
	
    int validatedInput = 0;

    char * actualInput = input [ 1 ];

	if ( length != EXPECTED_ARGUMENTS )
    {
		return INVALID_ARGUMENTS;
    }
	else 
    {

        while ( *actualInput != '\0' )
        {

            if ( ASCII_ZERO > *actualInput || ASCII_NINE < *actualInput )
            {
                return INVALID_ARGUMENTS;
            }

            actualInput++; 

        }

        validatedInput = atoi( input [ 1 ] );

        if ( validatedInput <= 0 )
        {
	        return INVALID_ARGUMENTS;
        }
	
        return validatedInput;

	}

}

/*********** simpleThread ( threadArgs  ) ******
 *   |
 *   |   Function: simpleThread
 *   |
 *   |   Parameters:
 *   |
 *   |   threadId: A pointer to number
 *   |             of the thread.
 *   |           
 *   |
 *   |   return: None.
 *   |
 *   |   Note: There are two variations of this
 *   |         method. The first variation is
 *   |         the unsynchronized version of it.
 *   |         To access the usynchronized version
 *   |         please make sure the preproccessed
 *   |         variable PTHREAD_SYNC is disabled
 *   |         as follows
 *   |
 *   |         // #define PTHREAD_SYNC
 *   |
 *   |         If you would like to run the 
 *   |         synchronized version then please
 *   |         make sure that the PTHREAD_SYNC
 *   |         variable is enabled as
 *   |
 *   |         #define PTHREAD_SYNC 
 *   |
 *   |         Make sure to compile the file
 *   |         everytime the varaibles are
 *   |         changed!
 *   |
 *   |
 **********************************************/
void * simpleThread( void *threadArgs ) 
{
	int which = *( (int*) threadArgs); // Thread pointer argument parsed to int
	
    int num, val;        

	for (num = 0; num < 20; num++) {

		if ( random() > RAND_MAX / 2 )
		{
			usleep( 500 );
		}

		#ifdef PTHREAD_SYNC 
     	pthread_mutex_lock ( &mutexSum );
        #endif

		val = sharedVariable;

		printf( "***Thread %d sees value %d\n", which, val );

		sharedVariable = val + 1;

		#ifdef PTHREAD_SYNC 
     	pthread_mutex_unlock ( &mutexSum );
        #endif

	}

	#ifdef PTHREAD_SYNC 
    pthread_barrier_wait( &myBarrier );
    #endif

	val = sharedVariable;

	printf( "Thread %d sees final value %d\n", which, val );

	pthread_exit(NULL);

}

