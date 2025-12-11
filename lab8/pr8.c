#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READERS 10
#define ARRAY_SIZE 64
#define WRITE_ITERATIONS 20
#define WRITE_INTERVAL_US 500000  // 500 ms

char shared_array[ARRAY_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int done = 0; // флаг завершения для читателей

void *writer(void *arg) {
    (void)arg;
    for (int i = 1; i <= WRITE_ITERATIONS; ++i) {
        pthread_mutex_lock(&mutex);

        // Записываем монотонно возрастающий счётчик в массив как строку
        // Очищаем массив и записываем текст "Record: <i>"
        snprintf(shared_array, ARRAY_SIZE, "Record: %d", i);

        pthread_mutex_unlock(&mutex);

        // Для наглядности делаем паузу
        usleep(WRITE_INTERVAL_US);
    }

    // Сказать читателям, что запись закончилась
    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *reader(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    char local_copy[ARRAY_SIZE];

    // Пока не поставлен флаг done — читаем
    while (1) {
        pthread_mutex_lock(&mutex);

        // Сначала проверить флаг done — если установлен и массив пустой/не меняется — выйти
        if (done) {
            // Возьмём финальное состояние массива для вывода (если нужно)
            strncpy(local_copy, shared_array, ARRAY_SIZE);
            pthread_mutex_unlock(&mutex);
            printf("[Reader %2d] final read: \"%s\"\n", idx, local_copy);
            break;
        }

        // Скопировать общий массив локально и отпустить мьютекс как можно быстрее
        strncpy(local_copy, shared_array, ARRAY_SIZE);
        pthread_mutex_unlock(&mutex);

        // Вывести: идентификатор потока (необязательно pthread_t — используем наш idx) и содержимое
        printf("[Reader %2d] read: \"%s\"\n", idx, local_copy);

        // Небольшая пауза, чтобы не засорять вывод
        usleep(150000); // 150 ms
    }

    return NULL;
}

int main(void) {
    pthread_t wthread;
    pthread_t rthreads[NUM_READERS];

    // Инициализация общего массива
    pthread_mutex_lock(&mutex);
    strncpy(shared_array, "Empty", ARRAY_SIZE);
    pthread_mutex_unlock(&mutex);

    // Создать писателя
    if (pthread_create(&wthread, NULL, writer, NULL) != 0) {
        perror("pthread_create writer");
        return 1;
    }

    // Создать читателей
    for (int i = 0; i < NUM_READERS; ++i) {
        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            return 1;
        }
        *arg = i + 1; // читаем удобные индексы 1..NUM_READERS
        if (pthread_create(&rthreads[i], NULL, reader, arg) != 0) {
            perror("pthread_create reader");
            free(arg);
            return 1;
        }
    }

    // Ожидать завершения писателя
    pthread_join(wthread, NULL);

    // Ожидать завершения читателей
    for (int i = 0; i < NUM_READERS; ++i) {
        pthread_join(rthreads[i], NULL);
    }

    // Очистка mutex
    pthread_mutex_destroy(&mutex);

    printf("All threads finished.\n");
    return 0;
}

