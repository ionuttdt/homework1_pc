#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

char check(msg M) {
    int i;
    char c = M.payload[0];
    for(i = 3; i < M.len; i++)
        c ^= M.payload[i];
    return c;
}

int main(int argc,char** argv){
    msg r,t;
    init(HOST,PORT);
    char c;
    int COUNT = 0, last_seqv = -1;
    int i = 0, seqv_recv = 0, not_text = -1;
    char *filen, *text;
    int fd;
    const char recv_[] = "recv_";


    while(i <= COUNT ) {
        if(recv_message(&r) < 0) {
            return -1;
        }
        c = check(r);
        if(r.payload[0] == 100 && c == r.payload[1]) {
            printf("O singura data\n");
            memcpy(&COUNT, r.payload + 2, sizeof(int));
            filen =  malloc(r.len);
            memcpy(filen, r.payload + 6, r.len );
            char *file = malloc(sizeof(recv_) + strlen(filen) +1);
            strcpy(file, recv_);
            strncat(file, filen, strlen(filen));
            text = calloc(COUNT * 1394, sizeof(char));
            if(!text)
                return -1;
            not_text = 1;

            //deschid fisierlui si ii dau drepturi
            fd = open( file , O_CREAT | O_TRUNC | O_WRONLY, 0664);
            if(fd < 0) {
                free(filen);
                return -1;
            }
            t.len = 1;
            t.payload[0] = 4;           //4 = am primit mesajul bun
            send_message(&t);           // confirmarea primirii mesajului
            i++;
        }
        else if(r.payload[0] == 100 && c != r.payload[1]) {
            t.len = 1;
            t.payload[0] = 5;
            send_message(&t);
        }
        else  if(r.payload[0] != 100 && c == r.payload[1] && not_text == 1) {
            if(i == COUNT ) {
                memcpy(&last_seqv, &(r.len), sizeof(int));
                printf("%d\n", last_seqv);
            }
            memcpy(&seqv_recv, r.payload + 2, sizeof(int));
            printf("sesss%d\n",seqv_recv );
            printf("iiiiiiii%d\n",i );
            memcpy(text+1394*seqv_recv, r.payload+6, r.len);

            t.len = 1;
            t.payload[0] = 4;
            send_message(&t);
            i++;
        }
        else {
            printf("nu trebuie AICI\n");
            t.len = 1;
            t.payload[0] = 5;
            send_message(&t);
        }


    }

    //scrierea propriu-zisa in fisier
    for(i = 0; i < COUNT-1 ; i++) {
        write(fd, text+ i*1394, 1394);
    }
    write(fd, text + (COUNT-1)*1394, last_seqv);
    printf("Gata!\n");

    t.len = 1;
    t.payload[0] = 'q';
    send_message(&t);
    if (recv_message(&r)<0){
        perror("Receive message");
        return -1;
    }

    //inchide fisierul
    close(fd);
    //eliberez memoria
    free(text);

    return 0;
}
