#include "math.h"

int abs(int n)
{
	return(n<0?-n:n);
}
double fabs(double x)
{
    double result;
    __asm__ volatile("fabs" : "=t" (result) : "0" (x));
    return result;
}

double ceil(double x); /// TODO
double floor(double x); /// TODO

double fmod(double numerator, double denominator); /// TODO

double cos(double x)
{
    double result;
    __asm__ volatile("fcos;" : "=t" (result) : "0" (x));
    return result;
}
double sin(double x)
{
    double result;
    __asm__ volatile("fsin;" : "=t" (result) : "0" (x));
    return result;
}
double tan(double x)
{
    double result;
    __asm__ volatile("fptan; fstp %%st(0)": "=t" (result) : "0" (x));
    return result;
}

double cosh(double x); /// TODO
double sinh(double x); /// TODO
double tanh(double x); /// TODO

double acos(double x)
{
    if (x < -1 || x > 1)
        return NAN;

    return(pi / 2 - asin(x));
}
double asin(double x)
{
    if (x < -1 || x > 1)
        return NAN;

    return(2 * atan(x / (1 + sqrt(1 - (x * x)))));
}
double atan(double x)
{
    double result;
    __asm__ volatile("fld1; fpatan" : "=t" (result) : "0" (x));
    return result;
}
double atan2(double x, double y)
{
    double result;
    __asm__ volatile("fpatan" : "=t" (result) : "0" (y), "u" (x));
    return result;
}

double sqrt(double x)
{
    if (x <  0.0)
        return NAN;

    double result;
    __asm__ volatile("fsqrt" : "=t" (result) : "0" (x));
    return result;
}

double exp(double x); /// TODO
double frexp(double x, int* exp); /// TODO
double ldexp(double x, int exp); /// TODO

double log(double x); /// TODO
double log10(double x); /// TODO

double modf(double x, double* intpart); /// TODO

double pow(double base, double exponent); /// TODO
