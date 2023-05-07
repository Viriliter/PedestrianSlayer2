#ifndef MATHFUN_H
#define MATHFUN_H

#include <vector>
#include <math.h>
#include <iostream>

namespace math_fun{

    template<class T>
    std::vector<double> linspace(T start_in, T end_in, int num_in)
    {
        std::vector<double> linspaced;

        double start = static_cast<double>(start_in);
        double end = static_cast<double>(end_in);
        double num = static_cast<double>(num_in);

        if (num == 0) { return linspaced; }
        if (num == 1) 
        {
            linspaced.push_back(start);
            return linspaced;
        }

        double delta = (end - start) / (num - 1);

        for(int i=0; i < num-1; ++i)
        {
            linspaced.push_back(start + delta * i);
        }
        linspaced.push_back(end); // I want to ensure that start and end
                                    // are exactly the same as the input
        return linspaced;
    };

    void gaussEliminationLS(std::vector<std::vector<float>> a, std::vector<float> &x);

    void printMatrix(std::vector<std::vector<float>> matrix);

    std::vector<float> polyfit(std::vector<float> y, std::vector<float> x, int deg);
}

#endif  // MATHFUN_H