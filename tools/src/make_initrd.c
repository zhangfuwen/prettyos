#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define N 64

struct initrd_header
{
    unsigned int magic;
    char name[64];
    unsigned int offset;
    unsigned int length;
};

int main(char argc, char **argv)
{
    int nheaders = (argc-1)/2;
    struct initrd_header headers[N]; // declare N headers in Filesystem

    int i;
    for (i=0; i<N; ++i)               // set the memory of N headers to binary 0
    {
        memset((void*)&headers[i], 0, sizeof(struct initrd_header));
    }

    unsigned int off = N * sizeof(struct initrd_header) + sizeof(int); // set data offset to N * 76

    for (i=0; i<nheaders; ++i)
    {
        printf("writing file %s->%s at 0x%x\n", argv[i*2+1], argv[i*2+2], off);
        strcpy(headers[i].name, argv[i*2+2]);
        headers[i].offset = off;
        FILE *stream = fopen(argv[i*2+1], "rb");
        if (stream == 0)
        {
            printf("Error: file not found: %s\n", argv[i*2+1]);
            return 1;
        }
        fseek(stream, 0, SEEK_END);
        headers[i].length = ftell(stream);
        off += headers[i].length;
        fclose(stream);
        headers[i].magic = 0xBF;
    }

    FILE *wstream = fopen("./initrd.dat", "wb");
    unsigned char *data = (unsigned char *)malloc(off);
    fwrite(&nheaders, sizeof(int), 1, wstream);
    fwrite(headers, sizeof(struct initrd_header), N, wstream); // write N headers

    fclose(wstream);
    wstream = fopen("./initrd.dat", "ab");

    for (i=0; i<nheaders; i++)
    {
        FILE *stream = fopen(argv[i*2+1], "rb");
        unsigned char *buf = (unsigned char *)malloc(headers[i].length);
        fread(buf, 1, headers[i].length, stream);
        fwrite(buf, 1, headers[i].length, wstream);
        fclose(stream);
        free(buf);
    }

    fclose(wstream);
    free(data);

    return 0;
}
