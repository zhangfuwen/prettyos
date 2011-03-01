#include "userlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

int main()
{
    setScrollField(0, 43); // We do not want to see scrolling output...
    puts("Berechnung quadratischer Gleichungen vom Typ x*x + p*x + q = 0");

    float p, q, x1, x2;
    printf("\nBitte p eingeben: ");

    char str[80];
    gets(str);
    p = atof(str);
    printf("\np= %f\n",p);
    printf("Bitte q eingeben: ");
    gets(str);
    q = atof(str);
    printf("\nq= %f\n",q);

    x1 = -p/2 + sqrt( p*p/4.0 - q );
    x2 = -p/2 - sqrt( p*p/4.0 - q );

    printf("x1 = %f  x2 = %f\n\n", x1, x2);

    printf("Press key.");
    getchar();

    return 0;
}
