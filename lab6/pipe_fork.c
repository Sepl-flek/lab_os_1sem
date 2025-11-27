#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe error");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(1);
    }

    if (pid > 0) {
        // ----- PARENT -----
        close(fd[0]); // close reading side

        time_t t = time(NULL);
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Parent time: %sParent PID: %d\n",
                 ctime(&t), getpid());

        printf("Parent: sending message...\n");

        write(fd[1], msg, strlen(msg) + 1);
        close(fd[1]);

        sleep(5); // must differ by >=5 seconds
    } else {
        // ----- CHILD -----
        close(fd[1]); // close writing side

        char buffer[256];
        read(fd[0], buffer, sizeof(buffer));
        close(fd[0]);

        time_t t = time(NULL);

        printf("Child received:\n%s", buffer);
        printf("Child time: %s\n", ctime(&t));
    }

    return 0;
}
