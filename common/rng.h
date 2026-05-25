#ifndef RNG_H
#define RNG_H

#include <random>

class Rng {
private:
    std::random_device rd;
    std::mt19937 generator;

public:
    Rng();
    double get_random_double(double first, double last);
    int get_random_int(int first, int last);
};

#endif
