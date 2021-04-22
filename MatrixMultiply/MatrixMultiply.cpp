// MatrixMultiplication.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

// This is our 18742 Project. 

#include "Matrix.h"
#include <iostream>
#include <chrono>
#include <thread>

Matrix::matrix matrixA;
Matrix::matrix matrixB;
Matrix::matrix matrixCWT;
Matrix::matrix matrixCWOT;

// Returns the start address of the matrix memory location
unsigned long matrix_addr_A() {
    return (unsigned long) matrixA.matrix;
}

unsigned long matrix_addr_B() {
    return (unsigned long) matrixB.matrix;
}

unsigned long matrix_addr_CWT() {
    return (unsigned long) matrixCWT.matrix;
}

unsigned long matrix_addr_CWOT() {
    return (unsigned long) matrixCWOT.matrix;
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
    int thread_num = 2; 
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

int main()
{
    std::cout << "Matrix multiplication\n";
    /*time_t startWOT, stopWOT, startWT, stopWT;*/

    matrixA.createRandomMatrix();
    matrixB.createRandomMatrix();
    matrixCWT.createEmptyMatrix();
    matrixCWOT.createEmptyMatrix();

	Matrix::matrix *matrixCWOTPtr = &matrixCWOT; //Pointer verweist auf den Speicher einer anderen Variable
    Matrix::matrix *matrixAPtr = &matrixA;
    Matrix::matrix *matrixBPtr = &matrixB;
    Matrix::matrix* matrixCWTPtr = &matrixCWT;

    // Only start pin trace after setting up problem space

    printf("Address of A: %lx\n", (unsigned long) matrixA.matrix);
    printf("Address of B: %lx\n", (unsigned long) matrixB.matrix);
    printf("Address of C: %lx\n", (unsigned long) matrixCWT.matrix);

        FILE *f;
    f = fopen("testing.address", "w");
    fprintf(f, "%lx", (unsigned long) &matrix_addr_A); // Write the address into file
    printf("FUN ADDR %lx\n", (unsigned long) matrix_addr_A); // Write the address into file
    printf("FUN ADDR %lx\n", matrix_addr_A()); // Write the address into file
    fclose(f);

    // std::cout << "\n[+] Single Core calculation started. \n[*] Calculating...";
    // auto startWOT = high_resolution_clock::now();
	// matrixmult(matrixCWOTPtr, matrixAPtr, matrixBPtr, matrixAPtr->m);
    // auto stopWOT = high_resolution_clock::now();
    // //double durationWOT = double(stopWOT - startWOT);
    // std::cout << "\n[+] Single Core calculation finished \n[+] Duration: " << duration<double> (stopWOT - startWOT).count() << " seconds";

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

    // Compile this with: 
    // g++ -std=c++11 -pthread Matrix.h Matrix.cpp MatrixMultiply.cpp 

}