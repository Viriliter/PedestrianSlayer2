#ifndef PID_H
#define PID_H 

#include <cmath>

class PID{
private:
    double dt;
    double integral;
    double pre_error;
    double max;
    double min;

    double Kp;
    double Ki;
    double Kd;
    
public:
    PID(double max, double min, double dt, double Kp, double Ki, double Kd):
    max(max), min(min), dt(dt), Kp(Kp), Ki(Ki), Kd(Kd){

    };

    double calculate(double setpoint, double pv){
        // Calculate error
        double error = setpoint - pv;

        // Proportional term
        double Pout = Kp * error;

        // Integral term
        double integral = error * dt;
        double Iout = Ki * integral;

        // Derivative term
        double derivative = (error - pre_error) / dt;
        double Dout = Kd * derivative;

        double out = Pout + Iout + Dout;

        // TODO Hard limit

        return out;


    };
};

#endif // PID_H