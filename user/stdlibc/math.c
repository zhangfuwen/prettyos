#include "math.h"


static double yMulLog(double x, double y)
{
    double result;
    __asm__ volatile("fyl2x" : "=t" (result) : "0" (x), "u" (y));
    return result;
}

static double pow2x(double x)
{
    double rndResult;
    double powResult = 1;
    double result;
    double fl = 0;
    int i;

    __asm__ volatile("frndint" : "=t" (rndResult) :"0"(x));

    if (rndResult > x)
    {
        fl = x - (rndResult-1);
        rndResult -=1.0;
    }
    else if (rndResult < x)
    {
        fl = x - rndResult;
    }


    for (i=1; i <= rndResult; i++)
    {
        powResult *= 2;
    }
    __asm__ volatile("f2xm1" : "=t" (result) : "0" (fl));

    if (x >=0)
    {
        return(result + 1.0) * powResult;
    }
    return 1 / ((result + 1.0) * powResult);
}


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

double ceil(double x)
{
    double result;
    __asm__ volatile("frndint" :"=t" (result) : "0"(x));
    if (result < x)
    {
        return result + 1;
    }
    return result;
}
double floor(double x)
{
    double result;
    __asm__ volatile("frndint" :"=t" (result) : "0"(x));
    if (result > x)
    {
        return result - 1;
    }

    return result;
}

double fmod(double numerator, double denominator)
{
    return(numerator - (double)((int)(numerator/denominator)) * denominator);
}

double cos(double x)
{
    double result;
    __asm__ volatile("fcos" : "=t" (result) : "0" (x));
    return result;
}
double sin(double x)
{
    double result;
    __asm__ volatile("fsin" : "=t" (result) : "0" (x));
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

double exp(double x)
{
    double result;
    __asm__ volatile("fldl2e": "=t" (result));
    return pow(pow2x(result),x);
}

double frexp(double x, int* exponent)
{
    int sign = 1;
    if (x < 0)
    {
        x *=-1;
        sign = -1;

    }
    double e = log(x);
    int po2x;
    *exponent =(int) ceil(e);
    if (*exponent == e)
    {
        return 1.0;
    }
    po2x = (int) pow2x((double)*exponent);
    return (x / po2x) *sign;
}
double ldexp(double x, int exponent)
{
    return(x*pow(2, exponent));
}

double log(double x)
{
    if (x <= 0)
    {
        return NAN;
    }
    return yMulLog(x,1.0);
}
double log10(double x)
{
    if (x <= 0)
    {
        return NAN;
    }
    return (yMulLog(x,1.0) / yMulLog(10.0,1.0));
}

double modf(double x, double* intpart)
{
    *intpart = (double)((int)x);
    return(x-*intpart);
}

double pow(double base, double exponent)
{
    int isOdd = 1;
    if (base < 0)
    {
        isOdd = ((int)floor(exponent) % 2) ? -1: 1;
        base *=-1;
    }
    return isOdd * pow2x(yMulLog(base,exponent));
}
