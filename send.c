#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

//functia care verifica daca mesajul s-a trimis fara corupere
char check(msg M) {
    int i;
    char c = M.payload[0];
    for(i = 3; i < M.len; i++)
        c ^= M.payload[i];
    return c;
}

int main(int argc,char** argv){

    init(HOST,PORT);
    msg t, r, f;
    int COUNT, sizefile;
    int timeout = 2*atoi(argv[3]);
    int i, seqv = 0;
    int window_aux;
    int* ack = calloc(COUNT, sizeof(int));

    //deschid fisierul
    char* file_name = argv[1];
    int file = open(file_name, O_RDONLY);
        if (file < 0) {
            printf("File error!\n");
            return -1;
    }
    //calculez dimensiunea fisierului si numarul de pachete
    sizefile = lseek(file, (size_t) 0, SEEK_END);
    lseek(file, (size_t) 0, SEEK_SET);
    COUNT = sizefile/1394 + 1;
    int window = 1000*atoi(argv[2])*atoi(argv[3])/(8*MSGSIZE);
    if(window > COUNT)
        window = COUNT/2;
    window_aux = window;
    char *text = calloc( (COUNT + 1)*1400, sizeof(char));

    memset(f.payload, 0, 1400);
    memcpy(f.payload+2, &COUNT, sizeof(int));
    memcpy(f.payload+6, file_name, strlen(file_name));
    f.len = sizeof(file_name);
    f.payload[0] = 100;
    f.payload[1] = check(f);


    for(i = 0; i <= COUNT +window_aux; ) {
        //printf("send %d\n", i);
        while(window > 0) {
            //printf("send %d\n", i);
            if(i == 0) {
                send_message(&f);
                window--;
                i++;
            }
            else {
                memset(t.payload, 0, 1400);
                t.len = read(file, t.payload + 6, 1394);
                t.payload[0] = 1;
                memcpy(t.payload + 2, &seqv, sizeof(int));
                seqv++;
                printf("send%d\n", seqv);
                t.payload[1] = check(t);
                //copie in caz de pierdere sau corupere
                memcpy(text + (i-1)*1400, t.payload, 1400);
                send_message(&t);
                i++;
                window--;
            }
        }
        if(recv_message_timeout(&r, timeout) < 0) {
            if(i == window_aux) {
                send_message(&f);
            }
            memcpy(t.payload, text + (i - window)*1400, 1400);
            t.len = 1400;
            send_message(&t);
        }
        else if(r.payload[0] != 4) {
            if(i == window_aux) {
                send_message(&f);
            }
            memcpy(t.payload, text + (i - window)*1400, 1400);
            t.len = 1400;
            send_message(&t);
        }
        else {
            if(i > COUNT) {
                i++;
                printf("i>COUNT\n");
            }
            else
                window++;
        }


    }

    while(1) {
        if (recv_message_timeout(&r,timeout) < 0) {
            return -1;
        }
        else if(r.payload[0] == 'q') {
            memset(t.payload, 0, 1400);
            t.len = 1;
            t.payload[0] = 7;
            break;
        }
    }

    free(ack);
    free(text);
    close(file);

    return 0;
}
