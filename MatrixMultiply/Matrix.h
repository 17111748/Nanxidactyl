// This is our 18742 Project. 

// #pragma once
#include <vector>

namespace Matrix {
	class matrix {
	public:
		const static int n = 8;
		const static int m = 8;
		float *matrix;
		void createRandomMatrix();
		void print();
		void createEmptyMatrix();
		void createAllOnes(); 
		void deleteMatrix();
	};
}