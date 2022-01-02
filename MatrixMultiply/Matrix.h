// This is our 18742 Project. 

// #pragma once
#include <vector>

namespace Matrix {
	class matrix {
	public:
		const static int n = 4;
		const static int m = 4;
		int *matrix;
		void createRandomMatrix();
		void print();
		void createEmptyMatrix();
		void createAllOnes(); 
		void deleteMatrix();
	};
}