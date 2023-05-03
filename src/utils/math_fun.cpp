#include "math_fun.h"

template<typename T>
std::vector<double> math_fun::linspace(T start_in, T end_in, int num_in)
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

void math_fun::gaussEliminationLS(std::vector<std::vector<float>> a, std::vector<float> &x){
    int i, j, k;
    int m = a.size();
    int n = a[0].size();

    for(i=0; i<m-1; i++){
        // Partial Pivoting
        for(k=i+1;k<m;k++){
            // If diagonal element(absolute vallue) is smaller than any of the terms below it
            if(std::fabs(a[i][i])<std::fabs(a[k][i])){
                // Swap the rows
                for(j=0;j<n;j++){                
                    double temp;
                    temp = a[i][j];
                    a[i][j] = a[k][j];
                    a[k][j] = temp;
                }
            }
        }
        // Begin Gauss Elimination
        for(k=i+1;k<m;k++){
            double  term = a[k][i]/ a[i][i];
            for(j=0; j<n; j++){
                a[k][j]=a[k][j]-term*a[i][j];
            }
        }
        
    }
    // Begin Back-substitution
    for(i=m-1; i>=0; i--){
        x[i] = a[i][n-1];
        for(j=i+1; j<n-1; j++){
            x[i] = x[i]-a[i][j]*x[j];
        }
        x[i] = x[i]/a[i][i];
    }           
};

void math_fun::printMatrix(std::vector<std::vector<float>> matrix){
    int i,j;
    for(i=0;i<matrix.size();i++){
        for(j=0; j<matrix[0].size(); j++){
            std::cout << matrix[i][j];
        }
        std::cout << std::endl;
    } 
};

std::vector<float> math_fun::polyfit(std::vector<float> y, std::vector<float> x, int deg){
    //y-axis data-points - x-axis data-points - degree of polynomial
    int N = x.size();  //no. of data-points
    int i, j;
    
    // an array of size 2*n+1 for storing N, Sig xi, Sig xi^2, ...., etc. which are the independent components of the normal matrix
    double X[2*deg+1];  
    for(i=0; i<=2*deg; i++){
        X[i]=0;
        for(j=0; j<N; j++){
            X[i] = X[i] + std::pow(x[j], i);
        }
    }

    //the normal augmented matrix
    std::vector<std::vector<float>> B(deg+1, std::vector<float>(deg+2));
    // rhs
    float Y[deg+1];      
    for(i=0; i<=deg; i++){
        Y[i] = 0;
        for(j=0; j<N; j++){
            Y[i] = Y[i] + std::pow(x[j], i)*y[j];
        }
    }
    for(i=0; i<=deg; i++){
        for(j=0; j<=deg; j++){
            B[i][j] = X[i+j]; 
        }
    }
    for(i=0; i<=deg; i++){
        B[i][deg+1] = Y[i];
    }
    
    std::vector<float> A;
    A.reserve(deg+1);
    printMatrix(B);
    gaussEliminationLS(B, A);
    return A;
};
