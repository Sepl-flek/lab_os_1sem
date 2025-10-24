#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Функция для обработки восьмеричного режима
int parse_octal_mode(const char *mode_str, mode_t *mode) {
    char *endptr;
    long mode_val = strtol(mode_str, &endptr, 8);
    
    if (*endptr != '\0' || mode_val < 0 || mode_val > 0777) {
        return -1;
    }
    
    *mode = (mode_t)mode_val;
    return 0;
}

// Функция для обработки символьного режима
int parse_symbolic_mode(const char *mode_str, mode_t *current_mode, mode_t *new_mode) {
    const char *p = mode_str;
    mode_t who = 0;  // Кому применяются права (u, g, o, a)
    mode_t op = 0;   // Операция (+, -, =)
    mode_t perm = 0; // Какие права (r, w, x)
    
    // Определяем, кому применяются права
    if (*p == 'u') {
        who |= S_IRUSR | S_IWUSR | S_IXUSR;
        p++;
    } else if (*p == 'g') {
        who |= S_IRGRP | S_IWGRP | S_IXGRP;
        p++;
    } else if (*p == 'o') {
        who |= S_IROTH | S_IWOTH | S_IXOTH;
        p++;
    } else if (*p == 'a') {
        who |= S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
        p++;
    } else {
        // Если не указано, применяем ко всем
        who |= S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
    }
    
    // Определяем операцию
    if (*p == '+') {
        op = 1; // Добавить права
        p++;
    } else if (*p == '-') {
        op = 2; // Убрать права
        p++;
    } else if (*p == '=') {
        op = 3; // Установить права точно
        p++;
    } else {
        return -1; // Неверный формат
    }
    
    // Определяем, какие права
    while (*p != '\0') {
        if (*p == 'r') {
            perm |= S_IRUSR | S_IRGRP | S_IROTH;
        } else if (*p == 'w') {
            perm |= S_IWUSR | S_IWGRP | S_IWOTH;
        } else if (*p == 'x') {
            perm |= S_IXUSR | S_IXGRP | S_IXOTH;
        } else {
            return -1; // Неверный символ права
        }
        p++;
    }
    
    // Применяем операцию
    *new_mode = *current_mode;
    
    if (op == 1) { // Добавить права
        *new_mode |= (perm & who);
    } else if (op == 2) { // Убрать права
        *new_mode &= ~(perm & who);
    } else if (op == 3) { // Установить права точно
        *new_mode &= ~who; // Сначала убираем все права для указанных категорий
        *new_mode |= (perm & who); // Затем устанавливаем нужные
    }
    
    return 0;
}

// Функция для обработки сложных символьных режимов (например, ug+rw)
int parse_complex_symbolic_mode(const char *mode_str, mode_t *current_mode, mode_t *new_mode) {
    const char *p = mode_str;
    mode_t who = 0;
    
    // Определяем все категории пользователей
    while (*p != '+' && *p != '-' && *p != '=' && *p != '\0') {
        if (*p == 'u') {
            who |= S_IRUSR | S_IWUSR | S_IXUSR;
        } else if (*p == 'g') {
            who |= S_IRGRP | S_IWGRP | S_IXGRP;
        } else if (*p == 'o') {
            who |= S_IROTH | S_IWOTH | S_IXOTH;
        } else if (*p == 'a') {
            who |= S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
        } else {
            return -1;
        }
        p++;
    }
    
    // Если не указано ни одной категории, применяем ко всем
    if (who == 0) {
        who |= S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
    }
    
    // Определяем операцию
    mode_t op = 0;
    if (*p == '+') {
        op = 1;
        p++;
    } else if (*p == '-') {
        op = 2;
        p++;
    } else if (*p == '=') {
        op = 3;
        p++;
    } else {
        return -1;
    }
    
    // Определяем права
    mode_t perm = 0;
    while (*p != '\0') {
        if (*p == 'r') {
            perm |= S_IRUSR | S_IRGRP | S_IROTH;
        } else if (*p == 'w') {
            perm |= S_IWUSR | S_IWGRP | S_IWOTH;
        } else if (*p == 'x') {
            perm |= S_IXUSR | S_IXGRP | S_IXOTH;
        } else {
            return -1;
        }
        p++;
    }
    
    // Применяем операцию
    *new_mode = *current_mode;
    
    if (op == 1) { // Добавить права
        *new_mode |= (perm & who);
    } else if (op == 2) { // Убрать права
        *new_mode &= ~(perm & who);
    } else if (op == 3) { // Установить права точно
        *new_mode &= ~who;
        *new_mode |= (perm & who);
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s [РЕЖИМ] ФАЙЛ\n", argv[0]);
        return 1;
    }
    
    const char *mode_str = argv[1];
    const char *filename = argv[2];
    
    // Получаем текущие права файла
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        perror("Ошибка при получении информации о файле");
        return 1;
    }
    
    mode_t current_mode = file_stat.st_mode;
    mode_t new_mode = current_mode;
    
    // Определяем тип режима и обрабатываем его
    int result = 0;
    
    // Проверяем, является ли режим восьмеричным
    if (strspn(mode_str, "01234567") == strlen(mode_str) && strlen(mode_str) <= 4) {
        result = parse_octal_mode(mode_str, &new_mode);
    } else {
        // Проверяем, является ли режим простым символьным (например, +x, u-r)
        if (strlen(mode_str) <= 3 && (strchr(mode_str, '+') || strchr(mode_str, '-') || strchr(mode_str, '='))) {
            result = parse_symbolic_mode(mode_str, &current_mode, &new_mode);
        } else {
            // Сложный символьный режим (например, ug+rw)
            result = parse_complex_symbolic_mode(mode_str, &current_mode, &new_mode);
        }
    }
    
    if (result != 0) {
        fprintf(stderr, "Ошибка: неверный формат режима '%s'\n", mode_str);
        return 1;
    }
    
    // Применяем новые права
    if (chmod(filename, new_mode) != 0) {
        perror("Ошибка при изменении прав файла");
        return 1;
    }
    
    return 0;
}
