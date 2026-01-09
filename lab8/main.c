#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define READERS_COUNT 10
#define BUFFER_SIZE 64
#define WRITES_COUNT 20

char shared_buffer[BUFFER_SIZE];
int counter = 0;

pthread_mutex_t mutex;

/* Пишущий поток */
void* writer_thread(void* arg) {
    for (int i = 0; i < WRITES_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        counter++;
        snprintf(shared_buffer, BUFFER_SIZE, "Запись № %d", counter);

        pthread_mutex_unlock(&mutex);

        sleep(1);
    }
    return NULL;
}

/* Читающий поток */
void* reader_thread(void* arg) {
    long tid = (long)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        printf("Reader tid=%ld | buffer: \"%s\"\n", tid, shared_buffer);

        pthread_mutex_unlock(&mutex);

        usleep(300000);
    }
    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[READERS_COUNT];

    pthread_mutex_init(&mutex, NULL);
    strcpy(shared_buffer, "Пусто");

    pthread_create(&writer, NULL, writer_thread, NULL);

    for (long i = 0; i < READERS_COUNT; i++) {
        pthread_create(&readers[i], NULL, reader_thread, (void*)i);
    }

    pthread_join(writer, NULL);

    printf("\nWriter завершил работу. Программа завершается.\n");

    pthread_mutex_destroy(&mutex);
    return 0;
}
