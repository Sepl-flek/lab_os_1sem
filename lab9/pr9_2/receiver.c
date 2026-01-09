#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>

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
    int shmid = shmget(KEY, SHM_SIZE, 0666);
    int semid = semget(KEY, 1, 0666);

    char* shm = (char*)shmat(shmid, NULL, 0);

    while (1) {
        time_t now = time(NULL);

        sem_lock(semid);
        printf("Receiver pid=%d time=%sReceived: %s\n",
               getpid(), ctime(&now), shm);
        sem_unlock(semid);

        sleep(3);
    }
}
