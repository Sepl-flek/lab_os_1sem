#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define BUFFER_SIZE 64

char shared_buffer[BUFFER_SIZE];
int counter = 0;

sem_t sem;

/* Пишущий поток */
void* writer(void* arg) {
    while (1) {
        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "Запись № %d", counter);
        sleep(1);
    }
    return NULL;
}

/* Читающий поток */
void* reader(void* arg) {
    pthread_t tid = pthread_self();

    while (1) {
        printf("Reader tid=%lu | buffer: %s\n",
               (unsigned long)tid, shared_buffer);
    }
    return NULL;
}

int main() {
    pthread_t w, r;

    sem_init(&sem, 0, 1);
    strcpy(shared_buffer, "Пусто");

    pthread_create(&w, NULL, writer, NULL);
    pthread_create(&r, NULL, reader, NULL);

    pthread_join(w, NULL);
    pthread_join(r, NULL);

    sem_destroy(&sem);
    return 0;
}
