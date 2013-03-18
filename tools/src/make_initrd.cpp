#include <cstdio>
#include <iostream>

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#endif
struct initrd_header
{
    unsigned int magic;
    char name[64];
    unsigned int offset;
    unsigned int length;
    initrd_header() {
        memset(this, 0, sizeof(*this));
    }
}
#ifdef WIN32
;
#pragma pack(pop)
#else
__attribute__((packed));
#endif

int main(unsigned int argc, char* argv[])
{
    int nheaders = (argc-1)/2;
    initrd_header* headers = new initrd_header[nheaders]; // declare nheaders headers in Filesystem

    unsigned int off = nheaders * sizeof(initrd_header) + sizeof(int); // set data offset to nheaders * 76

    for (int i = 0; i < nheaders; ++i)
    {
        std::cout << "Writing file " << argv[i*2+1] << "->" << argv[i*2+2] << " at 0x%x\n";
        strncpy(headers[i].name, argv[i*2+2], 64);
        headers[i].offset = off;
        FILE* stream = fopen(argv[i*2+1], "rb");
        if (stream == 0)
        {
            std::cerr << "Error: file not found: " << argv[i*2+1] << '\n';
            delete[] headers;
            return 1;
        }
        fseek(stream, 0, SEEK_END);
        headers[i].length = ftell(stream);
        off += headers[i].length;
        fclose(stream);
        headers[i].magic = 0xBF;
    }

    FILE* wstream = fopen("./initrd.dat", "wb");
    fwrite(&nheaders, sizeof(int), 1, wstream);
    fwrite(headers, sizeof(initrd_header), nheaders, wstream); // write N headers

    for (int i = 0; i < nheaders; ++i)
    {
        FILE* stream = fopen(argv[i*2+1], "rb");
        unsigned char* buf = new unsigned char[headers[i].length];
        fread(buf, 1, headers[i].length, stream);
        fwrite(buf, 1, headers[i].length, wstream);
        fclose(stream);
        delete[] buf;
    }

    fclose(wstream);
    delete[] headers;

    return 0;
}
