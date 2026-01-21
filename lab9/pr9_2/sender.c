#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define SHM_SIZE 128
#define KEY 0x1234

int shmid, semid;
char* shm;

/* Захват семафора */
void sem_lock(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

/* Освобождение семафора */
void sem_unlock(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

/* Деинициализация ресурсов */
void cleanup(int sig) {
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    printf("\nSender: resources cleaned up\n");
    _exit(0);
}

int main() {
    shmid = shmget(KEY, SHM_SIZE, IPC_CREAT | 0666);
    semid = semget(KEY, 1, IPC_CREAT | 0666);

    semctl(semid, 0, SETVAL, 1);

    shm = (char*)shmat(shmid, NULL, 0);

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

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
