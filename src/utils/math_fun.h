#ifndef MATHFUN_H
#define MATHFUN_H

#include <vector>
#include <math.h>
#include <iostream>

namespace math_fun{

    template<typename T>
    std::vector<double> linspace(T start_in, T end_in, int num_in);

    void gaussEliminationLS(std::vector<std::vector<float>> a, std::vector<float> &x);

    void printMatrix(std::vector<std::vector<float>> matrix);

    std::vector<float> polyfit(std::vector<float> y, std::vector<float> x, int deg);
}

#endif  // MATHFUN_H