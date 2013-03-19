/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.hpp"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"


class File {
    size_t filebuffersize;
    size_t filesize;
    char* buffer;
    char* filename;

public:
    File()
        : filebuffersize(0), filesize(0), buffer(0), filename(0)
    {
    }
    ~File()
    {
        delete[] buffer;
        delete[] filename;
    }
    bool open(const char* filename_)
    {
        delete[] buffer;
        delete[] filename;
        filebuffersize = 0;
        filesize = 0;
        buffer = 0;

        filename = new char[strlen(filename_)+1];
        strcpy(filename, filename_);

        FILE* f = fopen(filename, "r");
        if(!f)
            return false;
        for(size_t i = 0; ; i++)
        { // TODO Use ftell to get file size and allocate a buffer with optimal size
            if(i >= filebuffersize) {
                char* nbuffer = new char[filebuffersize+512];
                if(buffer) {
                    memcpy(nbuffer, buffer, filebuffersize);
                    delete[] buffer;
                }
                buffer = nbuffer;
                filebuffersize += 512;
            }
            char c = fgetc(f);
            if(c == EOF) {
                buffer[i] = 0;
                break;
            }
            buffer[i] = c;
            filesize++;
        }
        fclose(f);
        return true;
    }

    void save(const char* filename_)
    {
        delete[] filename;
        filename = new char[strlen(filename_)+1];
        strcpy(filename, filename_);

        FILE* f = fopen(filename_, "w+");
        fwrite(buffer, filesize, 1, f);
        fclose(f);
    }

    void insertChar(char c, size_t pos)
    { // TODO
    }

    void removeChar(size_t pos)
    { // TODO
    }

    const char* getFilename() const
    {
        return filename;
    }

    const char* getContent() const
    {
        return buffer;
    }

    size_t getFilesize() const
    {
        return filesize;
    }
};

class Editor {
    File file;
public:
    void open(const char* filename)
    {
        file.open(filename);
        clearScreen(0); // TODO: Only in scroll field
        const char* content = file.getContent();
        if(content) {
            for(size_t i = 0; i < file.getFilesize(); i++) {
                if(content[i] == 0)
                    putchar(' ');
                else if(content[i] == '\r' && i+1 < file.getFilesize() && content[i+1] == '\n')
                    ;
                else
                    putchar(content[i]);

            }
        }
    }

    void save()
    {
        save(file.getFilename());
    }

    void save(const char* filename)
    { // TODO
    }

    void textEnteredEvent(char text)
    { // TODO
    }

    void keyPressedEvent(KEY_t key)
    { // TODO
    }
};


int main(int argc, char* argv[])
{
    setScrollField(6, 43);
    printLine("================================================================================", 0, 0x0B);
    printLine("                                     Editor",                                      2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 6);

    Editor editor;

    if(argc > 1)
        editor.open(argv[1]);

    union {
        KEY_t key;
        char text;
    } buffer;

    event_enable(true);
    EVENT_t ev = event_poll(&buffer, 2048, EVENT_NONE);
    bool ctrl;

    for (;;)
    {
        switch (ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TEXT_ENTERED:
                if(!ctrl)
                    editor.textEnteredEvent(buffer.text);
                break;
            case EVENT_KEY_DOWN:
            {
                switch (buffer.key)
                {
                    case KEY_LCTRL: case KEY_RCTRL:
                        ctrl = true;
                        break;
                    case KEY_ESC:
                    {
                        return (0);
                    }
                    case KEY_S:
                    {
                        if (ctrl)
                            editor.save();
                        else
                            editor.keyPressedEvent(buffer.key);
                        break;
                    }
                    case KEY_O:
                    {
                        if (ctrl)
                        {
                            // Open
                        }
                        else
                            editor.keyPressedEvent(buffer.key);
                        break;
                    }
                    default:
                        editor.keyPressedEvent(buffer.key);
                        break;
                }
                break;
            }
            case EVENT_KEY_UP:
                switch (buffer.key)
                {
                    case KEY_LCTRL: case KEY_RCTRL:
                        ctrl = false;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        ev = event_poll(&buffer, 4096, EVENT_NONE);
    }
    return (0);
}

/*
* Copyright (c) 2013 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
