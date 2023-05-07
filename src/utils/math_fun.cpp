#include "math_fun.h"

std::vector<float> math_fun::polyfit(const std::vector<float> X, const std::vector<float> Y, int deg)
{
    // Declarations...
    // ----------------------------------
    enum {maxOrder = 5};
    
    double B[maxOrder+1] = {0.0f};
    double P[((maxOrder+1) * 2)+1] = {0.0f};
    double A[(maxOrder + 1)*2*(maxOrder + 1)] = {0.0f};

    double x, y, powx;
    std::vector<float> coefficients(deg+1, 0);

    unsigned int ii, jj, kk;

    // Verify initial conditions....
    // ----------------------------------

    // This method requires that the countOfElements > 
    // (deg+1) 
    if (X.size() <= deg)
        return coefficients;

    // This method has imposed an arbitrary bound of
    // deg <= maxOrder.  Increase maxOrder if necessary.
    if (deg > maxOrder)
        return coefficients;

    // Identify the column vector
    for (ii = 0; ii < X.size(); ii++)
    {
        x    = X[ii];
        y    = Y[ii];
        powx = 1;

        for (jj = 0; jj < (deg + 1); jj++)
        {
            B[jj] = B[jj] + (y * powx);
            powx  = powx * x;
        }
    }

    // Initialize the PowX array
    P[0] = X.size();

    // Compute the sum of the Powers of X
    for (ii = 0; ii < X.size(); ii++)
    {
        x    = X[ii];
        powx = X[ii];

        for (jj = 1; jj < ((2 * (deg + 1)) + 1); jj++)
        {
            P[jj] = P[jj] + powx;
            powx  = powx * x;
        }
    }

    // Initialize the reduction matrix
    //
    for (ii = 0; ii < (deg + 1); ii++)
    {
        for (jj = 0; jj < (deg + 1); jj++)
        {
            A[(ii * (2 * (deg + 1))) + jj] = P[ii+jj];
        }

        A[(ii*(2 * (deg + 1))) + (ii + (deg + 1))] = 1;
    }

    // Move the Identity matrix portion of the redux matrix
    // to the left side (find the inverse of the left side
    // of the redux matrix
    for (ii = 0; ii < (deg + 1); ii++)
    {
        x = A[(ii * (2 * (deg + 1))) + ii];
        if (x != 0)
        {
            for (kk = 0; kk < (2 * (deg + 1)); kk++)
            {
                A[(ii * (2 * (deg + 1))) + kk] = 
                    A[(ii * (2 * (deg + 1))) + kk] / x;
            }

            for (jj = 0; jj < (deg + 1); jj++)
            {
                if ((jj - ii) != 0)
                {
                    y = A[(jj * (2 * (deg + 1))) + ii];
                    for (kk = 0; kk < (2 * (deg + 1)); kk++)
                    {
                        A[(jj * (2 * (deg + 1))) + kk] = 
                            A[(jj * (2 * (deg + 1))) + kk] -
                            y * A[(ii * (2 * (deg + 1))) + kk];
                    }
                }
            }
        }
        else
        {
            // Cannot work with singular matrices
            return coefficients;
        }
    }

    // Calculate and Identify the coefficients
    for (ii = 0; ii < (deg + 1); ii++)
    {
        for (jj = 0; jj < (deg + 1); jj++)
        {
            x = 0;
            for (kk = 0; kk < (deg + 1); kk++)
            {
                x = x + (A[(ii * (2 * (deg + 1))) + (kk + (deg + 1))] * B[kk]);
            }
            coefficients[deg-ii] = x;
        }
    }

    return coefficients;
}