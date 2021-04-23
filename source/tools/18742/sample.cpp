#include <vector>

class Sample {
    public: 
        int t;
        Sample() {t = 66;}
        int ret(std::vector<int> s);
};

int Sample::ret(std::vector<int> s) {
    return this->t;
}
