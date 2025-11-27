#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define FIFO_NAME "myfifo"

int main() {
    mkfifo(FIFO_NAME, 0666);

    FILE *fifo = fopen(FIFO_NAME, "w");
    if (!fifo) {
        perror("fopen");
        exit(1);
    }

    time_t t = time(NULL);

    char msg[256];
    snprintf(msg, sizeof(msg),
             "Writer time: %sWriter PID: %d\n",
             ctime(&t), getpid());

    printf("Writer: sending message...\n");

    fprintf(fifo, "%s", msg);
    fclose(fifo);

    return 0;
}
