#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define FIFO_NAME "myfifo"

int main() {
    FILE *fifo = fopen(FIFO_NAME, "r");
    if (!fifo) {
        perror("fopen");
        exit(1);
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), fifo);
    fclose(fifo);

    time_t t = time(NULL);

    printf("Reader received:\n%s", buffer);
    printf("Reader time: %s\n", ctime(&t));

    return 0;
}
