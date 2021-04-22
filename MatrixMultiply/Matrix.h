// This is our 18742 Project. 

// #pragma once
#include <vector>

namespace Matrix {
	class matrix {
	public:
		const static int n = 2;
		const static int m = 2;
		float *matrix;
		void createRandomMatrix();
		void print();
		void createEmptyMatrix();
		void deleteMatrix();
	};
}