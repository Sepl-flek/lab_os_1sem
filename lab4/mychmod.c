// mychmod.c

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

void usage(const char *prog) {
    fprintf(stderr,
        "Использование: %s MODE FILE\n"
        "MODE может быть:\n"
        "  числовой oktal (например 766 или 0755)\n"
        "  символьный (например u+r, g-w, a+x, ug+rw, uga=rwx)\n"
        "  несколько символьных выражений через запятую: u+r,g-w\n",
        prog);
}

int is_octal_string(const char *s) {
    // считаем октальным, если все символы 0-7 (с возможным ведущим 0)
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '7') return 0;
    }
    return 1;
}

mode_t parse_octal(const char *s) {
    // strtol с base 8
    char *end;
    long val = strtol(s, &end, 8);
    return (mode_t) val;
}

int apply_clause(mode_t *mode, const char *clause, mode_t orig_mode) {
    // clause: [ugoa]*[+-=][rwx]+
    // возвращает 0 при успехе, -1 при ошибке
    const char *p = clause;
    int who_mask = 0;
    // соберём who
    while (*p && (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a')) {
        if (*p == 'u') who_mask |= 0700;
        else if (*p == 'g') who_mask |= 0070;
        else if (*p == 'o') who_mask |= 0007;
        else if (*p == 'a') who_mask |= (0700|0070|0007);
        p++;
    }
    if (who_mask == 0) {
        // по умолчанию 'a' (как в chmod)
        who_mask = (0700|0070|0007);
    }

    if (!*p) return -1;
    char op = *p++;
    if (!(op == '+' || op == '-' || op == '=')) return -1;

    if (!*p) return -1;

    int perm_bits_u = 0, perm_bits_g = 0, perm_bits_o = 0;
    // обработаем набор прав r,w,x
    while (*p) {
        if (*p == ',') { // если встретили запятую - конец этого clause (парсинг на уровне вызова)
            break;
        }
        char c = *p++;
        if (c == 'r') {
            perm_bits_u |= 0400; perm_bits_g |= 0040; perm_bits_o |= 0004;
        } else if (c == 'w') {
            perm_bits_u |= 0200; perm_bits_g |= 0020; perm_bits_o |= 0002;
        } else if (c == 'x') {
            perm_bits_u |= 0100; perm_bits_g |= 0010; perm_bits_o |= 0001;
        } else {
            // неподдерживаемый символ
            return -1;
        }
    }

    // соберём итоговую маску прав для применения (по who)
    mode_t add_mask = 0;
    if (who_mask & 0700) add_mask |= (perm_bits_u & 0700);
    if (who_mask & 0070) add_mask |= (perm_bits_g & 0070);
    if (who_mask & 0007) add_mask |= (perm_bits_o & 0007);

    if (op == '+') {
        *mode |= add_mask;
    } else if (op == '-') {
        *mode &= ~add_mask;
    } else if (op == '=') {
        // для '=' сначала очистим соответствующие биты у/g/o, затем выставим только add_mask (для тех кто указаны)
        if (who_mask & 0700) *mode &= ~0700;
        if (who_mask & 0070) *mode &= ~0070;
        if (who_mask & 0007) *mode &= ~0007;
        *mode |= add_mask;
    }

    return 0;
}

int apply_symbolic(mode_t *mode, const char *spec, mode_t orig_mode) {
    // spec может содержать несколько clause разделённых запятыми
    char *copy = strdup(spec);
    if (!copy) return -1;
    char *saveptr = NULL;
    char *token = strtok_r(copy, ",", &saveptr);
    int res = 0;
    while (token) {
        // trim spaces
        while (*token && isspace((unsigned char)*token)) token++;
        if (*token == '\0') { token = strtok_r(NULL, ",", &saveptr); continue; }
        if (apply_clause(mode, token, orig_mode) != 0) {
            res = -1;
            break;
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    free(copy);
    return res;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }

    const char *mode_str = argv[1];
    const char *path = argv[2];

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "Ошибка stat('%s'): %s\n", path, strerror(errno));
        return 2;
    }

    mode_t new_mode = st.st_mode & 07777; // текущие биты (включая setuid/setgid/sticky если есть)
    mode_t orig_mode = new_mode;

    if (is_octal_string(mode_str)) {
        // числовой режим
        mode_t parsed = parse_octal(mode_str);
        // сохранить специальные биты (setuid, setgid, sticky) из старого режима, если пользователь не указывает их явно:
        // Поведение стандартного chmod: если пользователь передаёт три цифры (rwx для u/g/o), то специальные биты сбрасываются.
        // Но обычно chmod 4755 устанавливает setuid — то есть если пользователь явно дал 4-значный число, оно учитывается.
        // Упростим: если длина строки == 3 — считаем, что пользователь не указывает спецбиты, т.е. чистые 3 цифры.
        size_t len = strlen(mode_str);
        if (len == 3) {
            new_mode = (st.st_mode & ~0777) | (parsed & 0777); // сохранить спецбиты
        } else {
            // если длина >=4, возьмём всё что дали (включая спецбиты)
            new_mode = parsed;
        }
    } else {
        // символьный режим
        if (apply_symbolic(&new_mode, mode_str, orig_mode) != 0) {
            fprintf(stderr, "Неверный символьный режим: %s\n", mode_str);
            usage(argv[0]);
            return 2;
        }
    }

    if (chmod(path, new_mode) != 0) {
        fprintf(stderr, "Ошибка chmod('%s', %o): %s\n", path, new_mode & 07777, strerror(errno));
        return 2;
    }

    return 0;
}
