#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define MAX_FILENAME_LEN 256
#define ARCHIVE_MAGIC "ARCHIVER_V1"
#define MAGIC_SIZE 12

// Структура заголовка файла в архиве
typedef struct {
    char magic[MAGIC_SIZE];           // Магическое число для проверки целостности
    char filename[MAX_FILENAME_LEN];  // Имя файла
    mode_t mode;                     // Права доступа
    uid_t uid;                       // ID пользователя
    gid_t gid;                       // ID группы
    time_t mtime;                    // Время модификации
    off_t size;                      // Размер файла
    off_t offset;                    // Смещение данных файла в архиве
} file_header_t;

// Структура для хранения информации о файлах в архиве
typedef struct {
    file_header_t header;
    int is_deleted;  // Флаг удаления файла
} archive_entry_t;

// Функция для вывода справки
void print_help() {
    printf("Архиватор - примитивный архиватор файлов\n\n");
    printf("Использование:\n");
    printf("  %s <архив> -i|--input <файл>     - добавить файл в архив\n", "archiver");
    printf("  %s <архив> -e|--extract <файл>   - извлечь файл из архива\n", "archiver");
    printf("  %s <архив> -s|--stat             - показать состояние архива\n", "archiver");
    printf("  %s -h|--help                     - показать эту справку\n", "archiver");
    printf("\nПримеры:\n");
    printf("  %s my_archive -i file1.txt\n", "archiver");
    printf("  %s my_archive -e file1.txt\n", "archiver");
    printf("  %s my_archive -s\n", "archiver");
}

// Функция для записи заголовка файла в архив
int write_file_header(int archive_fd, const char *filename, struct stat *file_stat, off_t data_offset) {
    file_header_t header;
    
    // Инициализация заголовка
    memset(&header, 0, sizeof(header));
    strncpy(header.magic, ARCHIVE_MAGIC, MAGIC_SIZE - 1);
    strncpy(header.filename, filename, MAX_FILENAME_LEN - 1);
    header.mode = file_stat->st_mode;
    header.uid = file_stat->st_uid;
    header.gid = file_stat->st_gid;
    header.mtime = file_stat->st_mtime;
    header.size = file_stat->st_size;
    header.offset = data_offset;
    
    // Запись заголовка в архив
    if (write(archive_fd, &header, sizeof(header)) != sizeof(header)) {
        perror("Ошибка записи заголовка файла");
        return -1;
    }
    
    return 0;
}

// Функция для чтения заголовка файла из архива
int read_file_header(int archive_fd, file_header_t *header) {
    if (read(archive_fd, header, sizeof(file_header_t)) != sizeof(file_header_t)) {
        return -1;
    }
    
    // Проверка магического числа
    if (strncmp(header->magic, ARCHIVE_MAGIC, MAGIC_SIZE - 1) != 0) {
        fprintf(stderr, "Ошибка: неверный формат архива\n");
        return -1;
    }
    
    return 0;
}

// Функция для добавления файла в архив
int add_file_to_archive(const char *archive_name, const char *filename) {
    int archive_fd, file_fd;
    struct stat file_stat;
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    off_t data_offset;
    
    // Получение информации о файле
    if (stat(filename, &file_stat) == -1) {
        perror("Ошибка получения информации о файле");
        return -1;
    }
    
    // Проверка, что это обычный файл
    if (!S_ISREG(file_stat.st_mode)) {
        fprintf(stderr, "Ошибка: %s не является обычным файлом\n", filename);
        return -1;
    }
    
    // Открытие архива для записи (создание или дополнение)
    archive_fd = open(archive_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (archive_fd == -1) {
        perror("Ошибка открытия архива");
        return -1;
    }
    
    // Получение текущей позиции в архиве (смещение для данных файла)
    data_offset = lseek(archive_fd, 0, SEEK_END);
    if (data_offset == -1) {
        perror("Ошибка получения позиции в архиве");
        close(archive_fd);
        return -1;
    }
    
    // Открытие файла для чтения
    file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Ошибка открытия файла");
        close(archive_fd);
        return -1;
    }
    
    // Запись заголовка файла
    if (write_file_header(archive_fd, filename, &file_stat, data_offset) == -1) {
        close(archive_fd);
        close(file_fd);
        return -1;
    }
    
    // Копирование содержимого файла в архив
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(archive_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Ошибка записи данных файла");
            close(archive_fd);
            close(file_fd);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        perror("Ошибка чтения файла");
        close(archive_fd);
        close(file_fd);
        return -1;
    }
    
    close(archive_fd);
    close(file_fd);
    
    printf("Файл %s успешно добавлен в архив %s\n", filename, archive_name);
    return 0;
}

// Функция для поиска файла в архиве
int find_file_in_archive(const char *archive_name, const char *filename, file_header_t *header) {
    int archive_fd;
    file_header_t temp_header;
    
    archive_fd = open(archive_name, O_RDONLY);
    if (archive_fd == -1) {
        perror("Ошибка открытия архива");
        return -1;
    }
    
    // Поиск файла в архиве
    while (read_file_header(archive_fd, &temp_header) == 0) {
        if (strcmp(temp_header.filename, filename) == 0) {
            *header = temp_header;
            close(archive_fd);
            return 0;  // Файл найден
        }
        
        // Пропуск данных файла
        if (lseek(archive_fd, temp_header.size, SEEK_CUR) == -1) {
            perror("Ошибка позиционирования в архиве");
            close(archive_fd);
            return -1;
        }
    }
    
    close(archive_fd);
    return -1;  // Файл не найден
}

// Функция для извлечения файла из архива
int extract_file_from_archive(const char *archive_name, const char *filename) {
    int archive_fd, file_fd;
    file_header_t header;
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    off_t current_pos;
    
    // Поиск файла в архиве
    if (find_file_in_archive(archive_name, filename, &header) == -1) {
        fprintf(stderr, "Файл %s не найден в архиве %s\n", filename, archive_name);
        return -1;
    }
    
    // Открытие архива для чтения
    archive_fd = open(archive_name, O_RDONLY);
    if (archive_fd == -1) {
        perror("Ошибка открытия архива");
        return -1;
    }
    
    // Переход к данным файла
    if (lseek(archive_fd, header.offset, SEEK_SET) == -1) {
        perror("Ошибка позиционирования в архиве");
        close(archive_fd);
        return -1;
    }
    
    // Создание файла для записи
    file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, header.mode);
    if (file_fd == -1) {
        perror("Ошибка создания файла");
        close(archive_fd);
        return -1;
    }
    
    // Копирование данных из архива в файл
    off_t remaining = header.size;
    while (remaining > 0) {
        size_t to_read = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        bytes_read = read(archive_fd, buffer, to_read);
        
        if (bytes_read <= 0) {
            perror("Ошибка чтения из архива");
            close(archive_fd);
            close(file_fd);
            return -1;
        }
        
        bytes_written = write(file_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("Ошибка записи в файл");
            close(archive_fd);
            close(file_fd);
            return -1;
        }
        
        remaining -= bytes_read;
    }
    
    // Восстановление атрибутов файла
    if (fchown(file_fd, header.uid, header.gid) == -1) {
        perror("Предупреждение: не удалось восстановить владельца файла");
    }
    
    if (fchmod(file_fd, header.mode) == -1) {
        perror("Предупреждение: не удалось восстановить права доступа");
    }
    
    // Восстановление времени модификации
    struct timespec times[2];
    times[0].tv_sec = header.mtime;
    times[0].tv_nsec = 0;
    times[1].tv_sec = header.mtime;
    times[1].tv_nsec = 0;
    
    if (futimens(file_fd, times) == -1) {
        perror("Предупреждение: не удалось восстановить время модификации");
    }
    
    close(archive_fd);
    close(file_fd);
    
    printf("Файл %s успешно извлечен из архива %s\n", filename, archive_name);
    return 0;
}

// Функция для показа состояния архива
int show_archive_status(const char *archive_name) {
    int archive_fd;
    file_header_t header;
    int file_count = 0;
    off_t total_size = 0;
    
    archive_fd = open(archive_name, O_RDONLY);
    if (archive_fd == -1) {
        perror("Ошибка открытия архива");
        return -1;
    }
    
    printf("Состояние архива: %s\n", archive_name);
    printf("=====================================\n");
    
    // Чтение всех файлов в архиве
    while (read_file_header(archive_fd, &header) == 0) {
        file_count++;
        total_size += header.size;
        
        printf("Файл: %s\n", header.filename);
        printf("  Размер: %ld байт\n", (long)header.size);
        printf("  Права: %o\n", header.mode);
        printf("  Владелец: %d:%d\n", header.uid, header.gid);
        printf("  Время модификации: %s", ctime(&header.mtime));
        printf("  Смещение в архиве: %ld\n", (long)header.offset);
        printf("\n");
        
        // Пропуск данных файла
        if (lseek(archive_fd, header.size, SEEK_CUR) == -1) {
            perror("Ошибка позиционирования в архиве");
            close(archive_fd);
            return -1;
        }
    }
    
    printf("=====================================\n");
    printf("Всего файлов: %d\n", file_count);
    printf("Общий размер данных: %ld байт\n", (long)total_size);
    
    close(archive_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Ошибка: недостаточно аргументов\n");
        print_help();
        return 1;
    }
    
    // Обработка флага справки
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }
    
    if (argc < 3) {
        fprintf(stderr, "Ошибка: недостаточно аргументов\n");
        print_help();
        return 1;
    }
    
    const char *archive_name = argv[1];
    const char *command = argv[2];
    
    if (strcmp(command, "-i") == 0 || strcmp(command, "--input") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Ошибка: не указан файл для добавления\n");
            return 1;
        }
        return add_file_to_archive(archive_name, argv[3]);
    }
    else if (strcmp(command, "-e") == 0 || strcmp(command, "--extract") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Ошибка: не указан файл для извлечения\n");
            return 1;
        }
        return extract_file_from_archive(archive_name, argv[3]);
    }
    else if (strcmp(command, "-s") == 0 || strcmp(command, "--stat") == 0) {
        return show_archive_status(archive_name);
    }
    else {
        fprintf(stderr, "Ошибка: неизвестная команда %s\n", command);
        print_help();
        return 1;
    }
    
    return 0;
}
