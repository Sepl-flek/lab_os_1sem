#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define SHM_KEY 0x12345
#define SHM_SIZE 256

int main() {
    // === Подключение к существующей разделяемой памяти ===
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid < 0) {
        printf("Sender еще не запущен! Нет разделяемой памяти.\n");
        return 1;
    }

    char *shm = (char *) shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        return 1;
    }

    printf("Receiver PID=%d запущен.\n", getpid());

    while (1) {
        // текущее время этого процесса
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);

        printf("RECEIVER PID=%d TIME=%02d:%02d:%02d | RECEIVED: %s\n",
               getpid(),
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               shm);

        sleep(1);
    }

    // недостижимо:
    shmdt(shm);
    return 0;
}
