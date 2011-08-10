#include "userlib.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"


unsigned int duration = 400;
void PlayTone(unsigned int Frequency, double DurFak) {
    if (Frequency == 0) {
        sleep(duration*DurFak);
    }
    else {
        beep(Frequency, duration*DurFak);
    }
}

void Play(char* string) {
    unsigned int Frequency = 0;
    for (int i = 0; string[i] != 0; ++i) {
        switch (string[i]) {
            case 'c':
                Frequency = 132;
                break;
            case 'd':
                Frequency = 149;
                break;
            case 'e':
                Frequency = 165;
                break;
            case 'f':
                Frequency = 176;
                break;
            case 'g':
                Frequency = 198;
                break;
            case 'a':
                Frequency = 220;
                break;
            case 'h':
                Frequency = 248;
                break;
            case 'C':
                Frequency = 264;
                break;
            case 'D':
                Frequency = 297;
                break;
            case 'E':
                Frequency = 330;
                break;
            case 'F':
                Frequency = 352;
                break;
            case 'G':
                Frequency = 396;
                break;
            case 'A':
                Frequency = 440;
                break;
            case 'H':
                Frequency = 495;
                break;
            case '0':
                Frequency = 0;
                break;
            default:
                continue;
        }
        if (string[i+1] == '+') {
            if (string[i+2] == '+') {
                PlayTone(Frequency, 4);
            }
            else {
                PlayTone(Frequency, 2);
            }
        }
        else if (string[i+1] == '-') {
            if (string[i+2] == '-') {
                PlayTone(Frequency, 0.25);
            }
            else {
                PlayTone(Frequency, 0.5);
            }
        }
        else {
            PlayTone(Frequency, 1.0);
        }
    }
}

int main() {
    textColor(0x0B);
    puts("================================================================================\n");
    puts("                    Mr.X Simple-Simple-Music-Creator  v0.4.2                    \n");
    puts("--------------------------------------------------------------------------------\n\n");
    puts("Please type in the duration of a full note or type in \"Alle meine Entchen\" or \"Hänschen klein\" to play that song and press ENTER.\n");
    char string1[100];
    gets(string1);
    if (strcmp(string1, "Alle meine Entchen") == 0) {
        duration = 500;
        Play("cdefg+g+aaaag++aaaag++ffffe+e+ddddc++\n");
    }
    if (strcmp(string1, "Hänschen klein\n") == 0) {
        duration = 500;
        Play("gee+fdd+cdefggg+gee+fdd+ceggc++dddddef+eeeeefg+gee+fdd+ceggc++\n");
    }
    else {
        duration = atoi(string1);
        puts("Please insert your notes (cdefgahCDEFGAH or 0 (break); speed controlled by --, -, nothing, +, ++) and press ENTER.\n");
        gets(string1);
        Play(string1);
    }
    return(0);
}
