#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#define SHM_KEY 0x12345
#define LOCKFILE "/tmp/sender.lock"
#define SHM_SIZE 256

int lock_fd = -1;
int shmid = -1;
char *shm = NULL;

// ===== Функция очистки =====
void cleanup() {
    if (shm != NULL)
        shmdt(shm);

    if (lock_fd != -1) {
        close(lock_fd);
        unlink(LOCKFILE);   // удаляем файл блокировки
    }
}

// ===== Обработчик Ctrl+C =====
void signal_handler(int sig) {
    printf("\nSender завершает работу (сигнал %d)...\n", sig);
    cleanup();
    exit(0);
}

int main() {
    // === обработчики сигналов ===
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // === Проверка единственного запуска через lockfile ===
    lock_fd = open(LOCKFILE, O_CREAT | O_EXCL, 0666);
    if (lock_fd == -1) {
        printf("Sender уже запущен! Завершаю программу...\n");
        return 1;
    }

    // === Создание разделяемой памяти ===
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        cleanup();
        return 1;
    }

    shm = (char *) shmat(shmid, NULL, 0);
    if (shm == (char *) -1) {
        perror("shmat");
        cleanup();
        return 1;
    }

    printf("Sender PID=%d запущен. Передаю данные...\n", getpid());

    while (1) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);

        char buffer[SHM_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "FROM SENDER PID=%d TIME=%02d:%02d:%02d",
                 getpid(),
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

        strcpy(shm, buffer);

        sleep(1);
    }

    cleanup();
    return 0;
}
