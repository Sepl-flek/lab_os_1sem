#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define SHM_SIZE 128
#define KEY 0x1234

void sem_lock(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

void sem_unlock(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

int main() {
    int shmid = shmget(KEY, SHM_SIZE, IPC_CREAT | 0666);
    int semid = semget(KEY, 1, IPC_CREAT | 0666);

    semctl(semid, 0, SETVAL, 1);

    char* shm = (char*)shmat(shmid, NULL, 0);

    while (1) {
        time_t now = time(NULL);

        sem_lock(semid);
        snprintf(shm, SHM_SIZE,
                 "Sender pid=%d time=%s",
                 getpid(), ctime(&now));
        sem_unlock(semid);

        sleep(3);
    }
}
