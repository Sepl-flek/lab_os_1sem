#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define READERS_COUNT 10
#define BUFFER_SIZE 64
#define WRITES_COUNT 20

char shared_buffer[BUFFER_SIZE];
int counter = 0;
int data_version = 0;
int finished = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;

/* Пишущий поток */
void* writer_thread(void* arg) {
    for (int i = 0; i < WRITES_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        counter++;
        data_version++;
        snprintf(shared_buffer, BUFFER_SIZE,
                 "Запись № %d", counter);

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }

    /* Сообщаем читателям о завершении */
    pthread_mutex_lock(&mutex);
    finished = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* Читающий поток */
void* reader_thread(void* arg) {
    long tid = (long)arg;
    int last_version = 0;

    pthread_mutex_lock(&mutex);
    while (!finished) {
        while (last_version == data_version && !finished) {
            pthread_cond_wait(&cond, &mutex);
        }

        if (finished) {
            break;
        }

        last_version = data_version;
        printf("Reader tid=%ld | buffer: \"%s\"\n",
               tid, shared_buffer);
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[READERS_COUNT];

    /* Инициализация */
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    strcpy(shared_buffer, "Пусто");

    /* Создание потоков */
    pthread_create(&writer, NULL, writer_thread, NULL);

    for (long i = 0; i < READERS_COUNT; i++) {
        pthread_create(&readers[i], NULL,
                       reader_thread, (void*)i);
    }

    /* Ожидание writer */
    pthread_join(writer, NULL);

    /* Ожидание всех readers */
    for (int i = 0; i < READERS_COUNT; i++) {
        pthread_join(readers[i], NULL);
    }

    /* Освобождение ресурсов */
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    printf("\nВсе потоки корректно завершены.\n");
    return 0;
}
