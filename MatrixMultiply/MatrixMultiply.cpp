// MatrixMultiplication.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

// This is our 18742 Project. 

#include "Matrix.h"
#include <iostream>
#include <chrono>
#include <thread>

#define NUM_CORES 2

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

    for (int i = lowerbound; i < upperbound; i++){

        for (int k = 0; k < Bptr->n; k++){

            for (int j = 0; j < Aptr->n; j++){
                Cptr->matrix[Bptr->n * i + k] += Aptr->matrix[Aptr->n * i + j] * Bptr->matrix[Bptr->n * j + k];
                //std::cout << "running..."<<i;
            }
        }
    }
    return;
}

void threadedmult(Matrix::matrix* Cptr, Matrix::matrix* Aptr, Matrix::matrix* Bptr) {
    int thread_num = NUM_CORES; 
    std::thread threadarr[thread_num];
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
    std::cout << "\n[*] Threads running...";
    for (int i = 0; i < thread_num; i++)
    {
        threadarr[i].join();
    }
    std::cout << "\n[+] Threads completed!";
}


void *main_func(void *arg) {
    Matrix::matrix *matrixCWOTPtr = &matrixCWOT; //Pointer verweist auf den Speicher einer anderen Variable
    Matrix::matrix *matrixAPtr = &matrixA;
    Matrix::matrix *matrixBPtr = &matrixB;
    Matrix::matrix* matrixCWTPtr = &matrixCWT;

    matrixA.print();
    matrixB.print();
    matrixCWT.print();
    

    std::cout << "\n[+] Multithreaded calculation started. \n[*] Calculating...";
    auto startWT = high_resolution_clock::now();
    threadedmult(matrixCWTPtr, matrixAPtr, matrixBPtr);
    auto stopWT = high_resolution_clock::now();
    std::cout << "\n[+] Multithreaded calculation finished \n[+] Duration: " << duration<double>(stopWT - startWT).count() << " seconds";

    std::cout << "\n" << std::endl; 
    matrixA.print();
    matrixB.print();
    matrixCWT.print();

    matrixA.deleteMatrix();
    matrixB.deleteMatrix();
    matrixCWT.deleteMatrix();

    return 0;
}

int main()
{
    std::cout << "Matrix multiplication\n";
    /*time_t startWOT, stopWOT, startWT, stopWT;*/

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

    // Compile this with: 
    // g++ -std=c++11 -pthread Matrix.h Matrix.cpp MatrixMultiply.cpp 

}