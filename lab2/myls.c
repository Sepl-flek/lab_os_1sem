#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define COLOR_BLUE "\x1b[34m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

typedef struct Options {
    int showAll;   // -a
    int longList;  // -l
} Options;

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-l] [-a] [paths...]\n", prog);
}

typedef struct EntryInfo {
    char *name;            // entry name (without path)
    char *fullPath;        // full path used for stat
    struct stat st;        // lstat data
    int isSymlink;         // 1 if S_ISLNK
} EntryInfo;

static int compare_entries_alpha(const void *a, const void *b) {
    const EntryInfo *ea = (const EntryInfo *)a;
    const EntryInfo *eb = (const EntryInfo *)b;
    return strcoll(ea->name, eb->name);
}

static void free_entry(EntryInfo *e) {
    if (!e) return;
    free(e->name);
    free(e->fullPath);
}

static void mode_to_string(mode_t mode, char out[11]) {
    char typeChar = '-';
    if (S_ISDIR(mode)) typeChar = 'd';
    else if (S_ISLNK(mode)) typeChar = 'l';
    else if (S_ISCHR(mode)) typeChar = 'c';
    else if (S_ISBLK(mode)) typeChar = 'b';
#ifdef S_ISSOCK
    else if (S_ISSOCK(mode)) typeChar = 's';
#endif
    else if (S_ISFIFO(mode)) typeChar = 'p';

    out[0] = typeChar;
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';

#ifdef S_ISUID
    if (mode & S_ISUID) out[3] = (out[3] == 'x') ? 's' : 'S';
#endif
#ifdef S_ISGID
    if (mode & S_ISGID) out[6] = (out[6] == 'x') ? 's' : 'S';
#endif
#ifdef S_ISVTX
    if (mode & S_ISVTX) out[9] = (out[9] == 'x') ? 't' : 'T';
#endif
}

static int is_executable(const struct stat *st) {
    return (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
}

static void print_name_with_color(const EntryInfo *e) {
    const mode_t m = e->st.st_mode;
    if (S_ISDIR(m)) {
        printf(COLOR_BLUE "%s" COLOR_RESET, e->name);
    } else if (S_ISLNK(m)) {
        printf(COLOR_CYAN "%s" COLOR_RESET, e->name);
    } else if (S_ISREG(m) && is_executable(&e->st)) {
        printf(COLOR_GREEN "%s" COLOR_RESET, e->name);
    } else {
        printf("%s", e->name);
    }
}

static void print_long_entry(const EntryInfo *e, int w_links, int w_user, int w_group, int w_size) {
    char modeStr[11];
    mode_to_string(e->st.st_mode, modeStr);

    struct passwd *pw = getpwuid(e->st.st_uid);
    struct group *gr = getgrgid(e->st.st_gid);
    const char *user = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";

    char timeBuf[64];
    struct tm *lt = localtime(&e->st.st_mtime);
    if (lt) {
        strftime(timeBuf, sizeof(timeBuf), "%b %e %H:%M", lt);
    } else {
        strncpy(timeBuf, "??? ?? ??:??", sizeof(timeBuf));
        timeBuf[sizeof(timeBuf)-1] = '\0';
    }

    printf("%s %*lu %-*s %-*s %*lld %s ",
           modeStr,
           w_links, (unsigned long)e->st.st_nlink,
           w_user, user,
           w_group, group,
           w_size, (long long)e->st.st_size,
           timeBuf);

    print_name_with_color(e);

    if (S_ISLNK(e->st.st_mode)) {
        char target[PATH_MAX];
        ssize_t n = readlink(e->fullPath, target, sizeof(target) - 1);
        if (n >= 0) {
            target[n] = '\0';
            printf(" -> %s", target);
        }
    }
    putchar('\n');
}

typedef struct Widths {
    int w_links;
    int w_user;
    int w_group;
    int w_size;
} Widths;

static int num_digits_unsigned(unsigned long long v) {
    int c = 1;
    while (v >= 10) { v /= 10; c++; }
    return c;
}

static void compute_widths(const EntryInfo *arr, size_t n, Widths *w) {
    memset(w, 0, sizeof(*w));
    for (size_t i = 0; i < n; ++i) {
        const EntryInfo *e = &arr[i];
        if (num_digits_unsigned((unsigned long long)e->st.st_nlink) > w->w_links)
            w->w_links = num_digits_unsigned((unsigned long long)e->st.st_nlink);

        struct passwd *pw = getpwuid(e->st.st_uid);
        struct group *gr = getgrgid(e->st.st_gid);
        int ulen = (int)strlen(pw ? pw->pw_name : "?");
        int glen = (int)strlen(gr ? gr->gr_name : "?");
        if (ulen > w->w_user) w->w_user = ulen;
        if (glen > w->w_group) w->w_group = glen;

        int sz = num_digits_unsigned((unsigned long long)e->st.st_size);
        if (sz > w->w_size) w->w_size = sz;
    }
}

static int list_directory(const char *path, const Options *opts, int print_header) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "myls: cannot access '%s': %s\n", path, strerror(errno));
        return 1;
    }

    if (print_header) {
        printf("%s:\n", path);
    }

    EntryInfo *entries = NULL;
    size_t cap = 0, len = 0;

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        const char *name = de->d_name;
        if (!opts->showAll && name[0] == '.') continue;

        if (len == cap) {
            size_t newCap = cap ? cap * 2 : 64;
            EntryInfo *tmp = (EntryInfo *)realloc(entries, newCap * sizeof(EntryInfo));
            if (!tmp) {
                perror("realloc");
                closedir(dir);
                for (size_t i = 0; i < len; ++i) free_entry(&entries[i]);
                free(entries);
                return 1;
            }
            entries = tmp;
            cap = newCap;
        }

        EntryInfo *e = &entries[len];
        memset(e, 0, sizeof(*e));
        e->name = strdup(name);

        size_t pathLen = strlen(path);
        size_t nameLen = strlen(name);
        int needSlash = (pathLen > 0 && path[pathLen - 1] != '/');
        e->fullPath = (char *)malloc(pathLen + needSlash + nameLen + 1);
        if (!e->fullPath || !e->name) {
            perror("malloc");
            closedir(dir);
            for (size_t i = 0; i < len; ++i) free_entry(&entries[i]);
            free(entries);
            return 1;
        }
        strcpy(e->fullPath, path);
        if (needSlash) strcat(e->fullPath, "/");
        strcat(e->fullPath, name);

        if (lstat(e->fullPath, &e->st) != 0) {
            fprintf(stderr, "myls: cannot stat '%s': %s\n", e->fullPath, strerror(errno));
            free_entry(e);
            continue;
        }
        e->isSymlink = S_ISLNK(e->st.st_mode);
        len++;
    }
    closedir(dir);

    qsort(entries, len, sizeof(EntryInfo), compare_entries_alpha);

    if (opts->longList) {
        long long totalBlocks = 0;
        for (size_t i = 0; i < len; ++i) totalBlocks += entries[i].st.st_blocks;
       
        printf("total %lld\n", totalBlocks / 2);
    }

    Widths w;
    if (opts->longList) compute_widths(entries, len, &w);

    for (size_t i = 0; i < len; ++i) {
        if (opts->longList) {
            print_long_entry(&entries[i], w.w_links, w.w_user, w.w_group, w.w_size);
        } else {
            print_name_with_color(&entries[i]);
            putchar('\n');
        }
    }

    for (size_t i = 0; i < len; ++i) free_entry(&entries[i]);
    free(entries);

    return 0;
}

static int list_file(const char *path, const Options *opts) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "myls: cannot access '%s': %s\n", path, strerror(errno));
        return 1;
    }
    EntryInfo e;
    memset(&e, 0, sizeof(e));
    const char *slash = strrchr(path, '/');
    e.name = strdup(slash ? slash + 1 : path);
    e.fullPath = strdup(path);
    e.st = st;
    e.isSymlink = S_ISLNK(st.st_mode);

    int rc = 0;
    if (opts->longList) {
        Widths w = (Widths){0};

        w.w_links = num_digits_unsigned((unsigned long long)st.st_nlink);
        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);
        w.w_user = (int)strlen(pw ? pw->pw_name : "?");
        w.w_group = (int)strlen(gr ? gr->gr_name : "?");
        w.w_size = num_digits_unsigned((unsigned long long)st.st_size);
        print_long_entry(&e, w.w_links, w.w_user, w.w_group, w.w_size);
    } else {
        print_name_with_color(&e);
        putchar('\n');
    }

    free_entry(&e);
    return rc;
}

int main(int argc, char **argv) {
    Options opts;
    memset(&opts, 0, sizeof(opts));

    char **paths = NULL;
    int pathsCount = 0;
    int pathsCap = 0;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--") == 0) {
    
            for (int j = i + 1; j < argc; ++j) {
                if (pathsCount == pathsCap) {
                    int newCap = pathsCap ? pathsCap * 2 : 8;
                    char **tmp = (char **)realloc(paths, (size_t)newCap * sizeof(char *));
                    if (!tmp) { perror("realloc"); free(paths); return 1; }
                    paths = tmp; pathsCap = newCap;
                }
                paths[pathsCount++] = argv[j];
            }
            break;
        }
        if (arg[0] == '-' && arg[1] != '\0') {
            // Опции пачкой: -la == -l -a
            for (int k = 1; arg[k] != '\0'; ++k) {
                char c = arg[k];
                if (c == 'l') opts.longList = 1;
                else if (c == 'a') opts.showAll = 1;
                else {
                    fprintf(stderr, "myls: invalid option -- '%c'\n", c);
                    print_usage(argv[0]);
                    free(paths);
                    return 1;
                }
            }
        } else {
            // Это путь
            if (pathsCount == pathsCap) {
                int newCap = pathsCap ? pathsCap * 2 : 8;
                char **tmp = (char **)realloc(paths, (size_t)newCap * sizeof(char *));
                if (!tmp) { perror("realloc"); free(paths); return 1; }
                paths = tmp; pathsCap = newCap;
            }
            paths[pathsCount++] = argv[i];
        }
    }

    int exitCode = 0;

    if (pathsCount <= 0) {
        exitCode |= list_directory(".", &opts, 0);
        free(paths);
        return exitCode ? 1 : 0;
    }

    // Отсортируем пути альфавитно
    for (int i = 0; i < pathsCount; ++i) {
        for (int j = i + 1; j < pathsCount; ++j) {
            if (strcoll(paths[i], paths[j]) > 0) {
                char *tmp = paths[i];
                paths[i] = paths[j];
                paths[j] = tmp;
            }
        }
    }

    // Pass 1: файлы
    for (int i = 0; i < pathsCount; ++i) {
        struct stat st;
        if (lstat(paths[i], &st) != 0) {
            fprintf(stderr, "myls: cannot access '%s': %s\n", paths[i], strerror(errno));
            exitCode = 1;
            continue;
        }
        if (!S_ISDIR(st.st_mode)) {
            exitCode |= list_file(paths[i], &opts);
        }
    }

    // Пустая строка между файлами и директориями, если есть и то, и другое
    int hasFiles = 0, hasDirs = 0;
    for (int i = 0; i < pathsCount; ++i) {
        struct stat st;
        if (lstat(paths[i], &st) == 0) {
            if (S_ISDIR(st.st_mode)) hasDirs = 1; else hasFiles = 1;
        }
    }
    if (hasFiles && hasDirs) puts("");

    // Pass 2: директории
    int multiple = (pathsCount > 1);
    int printedHeaderBefore = 0;
    for (int i = 0; i < pathsCount; ++i) {
        struct stat st;
        if (lstat(paths[i], &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            int header = multiple || printedHeaderBefore;
            exitCode |= list_directory(paths[i], &opts, header || multiple);
            if (i < pathsCount - 1) puts("");
            printedHeaderBefore = 1;
        }
    }

    free(paths);
    return exitCode ? 1 : 0;
}


