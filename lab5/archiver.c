#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 4096

typedef struct {
    char filename[256];
    mode_t mode;
    off_t size;
    time_t mtime;
} FileHeader;

void print_help() {
    printf("\nПримитивный архиватор\n");
    printf("Использование:\n");
    printf("  ./archiver arch_name -i(--input) file1   - добавить файл в архив\n");
    printf("  ./archiver arch_name -e(--extract) file1 - извлечь файл из архива\n");
    printf("  ./archiver arch_name -s(--stat)          - показать содержимое архива\n");
    printf("  ./archiver -h(--help)                    - показать справку\n\n");
}

void add_file(const char *archive, const char *filename) {
    int arch_fd = open(archive, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (arch_fd < 0) {
        perror("Ошибка открытия архива");
        exit(1);
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("Ошибка открытия файла");
        close(arch_fd);
        exit(1);
    }

    struct stat st;
    fstat(file_fd, &st);

    FileHeader header;
    strncpy(header.filename, filename, sizeof(header.filename));
    header.mode = st.st_mode;
    header.size = st.st_size;
    header.mtime = st.st_mtime;

    write(arch_fd, &header, sizeof(FileHeader));

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        write(arch_fd, buffer, bytes);
    }

    close(file_fd);
    close(arch_fd);

    printf("Файл '%s' добавлен в архив '%s'\n", filename, archive);
}

void extract_file(const char *archive, const char *filename) {
    int arch_fd = open(archive, O_RDWR);
    if (arch_fd < 0) {
        perror("Ошибка открытия архива");
        exit(1);
    }

    FileHeader header;
    off_t offset = 0;
    int found = 0;

    while (read(arch_fd, &header, sizeof(FileHeader)) == sizeof(FileHeader)) {
        if (strcmp(header.filename, filename) == 0) {
            found = 1;
            int out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, header.mode);
            if (out_fd < 0) {
                perror("Ошибка создания файла");
                close(arch_fd);
                exit(1);
            }

            char buffer[BUFFER_SIZE];
            ssize_t remaining = header.size;
            ssize_t bytes;
            while (remaining > 0 && (bytes = read(arch_fd, buffer, BUFFER_SIZE)) > 0) {
                ssize_t to_write = (bytes < remaining) ? bytes : remaining;
                write(out_fd, buffer, to_write);
                remaining -= to_write;
            }

            close(out_fd);
            printf("Файл '%s' извлечен.\n", filename);
            break;
        } else {
            lseek(arch_fd, header.size, SEEK_CUR);
            offset += sizeof(FileHeader) + header.size;
        }
    }

    if (!found) {
        printf("Файл '%s' не найден в архиве.\n", filename);
    }

    close(arch_fd);
}

void show_stat(const char *archive) {
    int arch_fd = open(archive, O_RDONLY);
    if (arch_fd < 0) {
        perror("Ошибка открытия архива");
        exit(1);
    }

    FileHeader header;
    printf("\nСодержимое архива '%s':\n", archive);
    while (read(arch_fd, &header, sizeof(FileHeader)) == sizeof(FileHeader)) {
        printf("  %s (размер: %ld байт, права: %o, время: %s)", header.filename, header.size, header.mode, ctime(&header.mtime));
        lseek(arch_fd, header.size, SEEK_CUR);
    }

    close(arch_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    if (argc < 3) {
        fprintf(stderr, "Недостаточно аргументов.\n");
        return 1;
    }

    const char *archive = argv[1];

    if ((strcmp(argv[2], "-i") == 0 || strcmp(argv[2], "--input") == 0) && argc == 4) {
        add_file(archive, argv[3]);
    } else if ((strcmp(argv[2], "-e") == 0 || strcmp(argv[2], "--extract") == 0) && argc == 4) {
        extract_file(archive, argv[3]);
    } else if (strcmp(argv[2], "-s") == 0 || strcmp(argv[2], "--stat") == 0) {
        show_stat(archive);
    } else {
        print_help();
    }

    return 0;
}
