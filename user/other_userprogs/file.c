#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0D);
    printLine(" => PrettyFile", 2, 0x0F);
	printLine("    by Christian F. Coors (Cuervo)", 4, 0x0F);
    printLine("--------------------------------------------------------------------------------", 6, 0x0D);
	
	
    iSetCursor(0, 7);
    textColor(0x0F);
	
	printf("\n");
	//##########################################################_);
    printf(" => Opening file...                                 ");
	
	file_t* file = fopen("1:/lrm_ipm.txt","r");
	
	if(file==0) {
		printf("Cannot open 1:/lrm_ipm.txt!\n");
		printf("Press any key to continue...\n\n");
		getchar();
		return(0);
	}
	
	textColor(0x07);
	printf("[");
	textColor(0x0A);
	printf("OK");
	textColor(0x07);
	printf("]\n");
	textColor(0x0F);
	
	printf("\n\n");
	
	
	char c;
	
	
	while ((c = fgetc(file)) != EOF)
	{
		printf("%c",c);
	}
	
	
	fclose(file);
	printf("\n\n");
	
	//##########################################################_);
    printf(" => Writing file 'm.txt'...                         ");
	
	file_t* file2 = fopen("1:/m.txt","w");
	
	if(file==0) {
		printf("Cannot open 1:/m.txt for writing!\n");
		printf("Press any key to continue...\n\n");
		getchar();
		return(0);
	}
	
	textColor(0x07);
	printf("[");
	textColor(0x0A);
	printf("OK");
	textColor(0x07);
	printf("]\n");
	textColor(0x0F);
	
	printf("\n\n");
	
	fputc('T',file2);
	fputc('e',file2);
	fputc('s',file2);
	fputc('t',file2);
	
	
	fclose(file2);
	
	printf("\n\n");
	textColor(0x0F);
	printf(" => => Press any key to continue... <= <=\n\n");
	getchar();
    
    return(0);
}
