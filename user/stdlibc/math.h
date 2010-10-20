#ifndef MATH_H
#define MATH_H

#define NAN (__builtin_nanf (""))
#define pi 3.1415926535897932384626433832795028841971693993

#ifdef _cplusplus
extern "C" {
#endif

int abs(int n);
double fabs(double x);

double ceil(double x);
double floor(double x);

double fmod(double numerator, double denominator);

double cos(double x);
double sin(double x);
double tan(double x);

double cosh(double x);
double sinh(double x);
double tanh(double x);

double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double x, double y);

double sqrt(double x);

double exp(double x);
double frexp(double x, int* exp);
double ldexp(double x, int exp);

double log(double x);
double log10(double x);

double modf(double x, double* intpart);

double pow(double base, double exponent);

#ifdef _cplusplus
}
#endif

#endif
