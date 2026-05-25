#include "rng.h"

Rng::Rng(): generator(rd()) {}

double Rng::get_random_double(double first, double last) {
    std::uniform_real_distribution<double> distribution(first, last);
    return distribution(generator);
}

int Rng::get_random_int(int first, int last) {
    std::uniform_int_distribution<int> distribution(first, last);
    return distribution(generator);
}
