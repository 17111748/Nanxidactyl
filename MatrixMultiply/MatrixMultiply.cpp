// MatrixMultiplication.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

// This is our 18742 Project. 

#include "Matrix.h"
#include <iostream>
#include <chrono>
#include <thread>

#define NUM_CORES 4

pthread_mutex_t mutex;
pthread_barrier_t barrier;

typedef struct pin_data {
    unsigned long addr_A;
    unsigned long addr_B;
    unsigned long addr_CWT;
    unsigned long addr_CWOT;
    int A_length;
    int B_length;
    int CWT_length;
    int CWOT_length;
    int num_cores;
} pin_data_t;

Matrix::matrix matrixA;
Matrix::matrix matrixB;
Matrix::matrix matrixCWT;
Matrix::matrix matrixCWOT;

pin_data_t ret_pin_data() {
    pin_data_t data;
    data.addr_A = (unsigned long) matrixA.matrix;
    data.addr_B = (unsigned long) matrixB.matrix;
    data.addr_CWT = (unsigned long) matrixCWT.matrix;
    data.addr_CWOT = (unsigned long) matrixCWOT.matrix;
    data.A_length = matrixA.n * matrixA.m;
    data.B_length = matrixB.n * matrixB.m;
    data.CWT_length = matrixCWT.n * matrixCWT.m;
    data.CWOT_length = matrixCWOT.n * matrixCWOT.m;
    data.num_cores = NUM_CORES;
    return data;
}

using namespace std::chrono;

void matrixmult(Matrix::matrix* Cptr, Matrix::matrix* Aptr, Matrix::matrix* Bptr,int upperbound,int lowerbound = 0) {
   
    for (int count = 0; count < 10; count++) {
        pthread_barrier_wait(&barrier); 
        for (int i = lowerbound; i < upperbound; i++){
            for (int k = 0; k < Bptr->n; k++){
                for (int j = 0; j < Aptr->n; j++){
                    Cptr->matrix[Cptr->n * i + k] += Aptr->matrix[Aptr->n * i + j] * Bptr->matrix[Bptr->n * j + k];
                    //std::cout << "running..."<<i;
                }
            }
        }
        // pthread_mutex_lock(&mutex);
        pthread_barrier_wait(&barrier); 
        for (int i = lowerbound; i < upperbound; i++) {
            for (int k = 0; k < Aptr->n; k++) {
                // float max = (Aptr->matrix[Bptr->n * i + k], Bptr->matrix[Bptr->n * i + k], Cptr->matrix[Bptr->n * i + k]); 
                
                Bptr->matrix[Bptr->n * i + k] = Cptr->matrix[Bptr->n * i + k]/13 + Aptr->matrix[Bptr->n * i + k]; 
                
                // if(k % 2 == 0) {
                //     Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k] + 1; 
                // }
                // else {
                //     Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k] - 1; 
                // }

                // if(k % 2 == 0) {
                //     if (i % 2 == 0) {
                //         Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k]/7 + 2;
                //     }
                //     else {
                //         Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k]/4 + 3;
                //     }
                // }
                // else {
                //     if (i % 2 == 0) {
                //         Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k]/3 - 1;
                //     }
                //     else {
                //         Bptr->matrix[Bptr->n * i + k] += Cptr->matrix[Bptr->n * i + k]/5 - 2;
                //     }
                // }
            }
        }
        // pthread_mutex_unlock(&mutex);
        
    }

    return;
}

void threadedmult(Matrix::matrix* Cptr, Matrix::matrix* Aptr, Matrix::matrix* Bptr) {
    int thread_num = NUM_CORES; 
    std::thread threadarr[thread_num];

    for(int i = 0; i < Aptr->n*Aptr->n; i++) {
        Aptr->matrix[i] = (i+1); 
        Bptr->matrix[i] = (i+2); 
    }
    // Aptr->matrix[0] = 0; 
    int step = Aptr->m / thread_num;
    int upperbound;
    for (int i = 0; i < thread_num; i++)
    {
        if (i == (thread_num - 1))
        {
            upperbound = Aptr->m;
        }
        else
        {
            upperbound = (i + 1) * step;
        }
        threadarr[i] = std::thread(matrixmult, Cptr, Aptr, Bptr, upperbound, (i*step));
    }
    // std::cout << "\n[*] Threads running...";
    for (int i = 0; i < thread_num; i++)
    {
        threadarr[i].join();
    }
    // std::cout << "\n[+] Threads completed!";
}


void *main_func(void *arg) {
    Matrix::matrix *matrixCWOTPtr = &matrixCWOT; //Pointer verweist auf den Speicher einer anderen Variable
    Matrix::matrix *matrixAPtr = &matrixA;
    Matrix::matrix *matrixBPtr = &matrixB;
    Matrix::matrix* matrixCWTPtr = &matrixCWT;

    // matrixA.print();
    // matrixB.print();
    // matrixCWT.print();
    

    // std::cout << "\n[+] Multithreaded calculation started. \n[*] Calculating...";
    auto startWT = high_resolution_clock::now();
    threadedmult(matrixCWTPtr, matrixAPtr, matrixBPtr);
    auto stopWT = high_resolution_clock::now();
    // std::cout << "\n[+] Multithreaded calculation finished \n[+] Duration: " << duration<double>(stopWT - startWT).count() << " seconds";

    // std::cout << "\n" << std::endl; 
    // matrixA.print();
    // matrixB.print();
    matrixCWT.print();

    matrixA.deleteMatrix();
    matrixB.deleteMatrix();
    matrixCWT.deleteMatrix();

    return 0;
}

int main()
{
    // std::cout << "Matrix multiplication\n";
    /*time_t startWOT, stopWOT, startWT, stopWT;*/

    pthread_mutex_init(&mutex, NULL);
	pthread_barrier_init(&barrier, NULL, NUM_CORES);

    // matrixA.createRandomMatrix();
    // matrixB.createRandomMatrix();
    matrixA.createAllOnes(); 
    matrixB.createAllOnes(); 
    matrixCWT.createEmptyMatrix();
    matrixCWOT.createEmptyMatrix();
    


    // Only start pin trace after setting up problem space

    // printf("Address of A: %lx\n", (unsigned long) matrixA.matrix);
    // printf("Address of B: %lx\n", (unsigned long) matrixB.matrix);
    // printf("Address of C: %lx\n", (unsigned long) matrixCWT.matrix);

    // std::cout << "\n[+] Single Core calculation started. \n[*] Calculating...";
    // auto startWOT = high_resolution_clock::now();
	// matrixmult(matrixCWOTPtr, matrixAPtr, matrixBPtr, matrixAPtr->m);
    // auto stopWOT = high_resolution_clock::now();
    // //double durationWOT = double(stopWOT - startWOT);
    // std::cout << "\n[+] Single Core calculation finished \n[+] Duration: " << duration<double> (stopWOT - startWOT).count() << " seconds";

    // Execute the entire main section in a separate thread for PIN
    pthread_t npid1;
    pthread_create(&npid1, NULL, main_func, NULL);
    pthread_join(npid1, NULL);


    // pthread_exit(NULL); 
    // Compile this with: 
    // g++ -std=c++11 -pthread Matrix.h Matrix.cpp MatrixMultiply.cpp 

}