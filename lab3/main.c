#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

static void on_process_exit(void) {
    pid_t pid = getpid();
    fprintf(stdout, "[atexit] Процесс %ld завершает работу\n", (long)pid);
    fflush(stdout);
}

static void handle_sigint(int signo) {
    const char *desc = strsignal(signo);
    fprintf(stdout, "[signal] Получен сигнал %d (%s) в процессе PID=%ld, PPID=%ld\n",
            signo, desc ? desc : "?", (long)getpid(), (long)getppid());
    fflush(stdout);
}

static void handle_sigterm(int signo, siginfo_t *info, void *ucontext) {
    (void)ucontext;
    const char *desc = strsignal(signo);
    fprintf(stdout,
            "[sigaction] Получен сигнал %d (%s) в процессе PID=%ld, PPID=%ld, si_pid=%ld\n",
            signo, desc ? desc : "?", (long)getpid(), (long)getppid(),
            info ? (long)info->si_pid : -1L);
    fflush(stdout);
}

static int setup_signal_handlers(void) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("signal(SIGINT)");
        return -1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_sigterm;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM)");
        return -1;
    }
    return 0;
}

int main(void) {
    fprintf(stdout, "Старт программы. PID=%ld, PPID=%ld\n", (long)getpid(), (long)getppid());
    fflush(stdout);

    if (atexit(on_process_exit) != 0) {
        fprintf(stderr, "Не удалось зарегистрировать atexit()\n");
        return EXIT_FAILURE;
    }

    if (setup_signal_handlers() != 0) {
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        
        fprintf(stdout, "[child] Привет! PID=%ld, PPID=%ld\n", (long)getpid(), (long)getppid());
        fflush(stdout);

        if (atexit(on_process_exit) != 0) {
            fprintf(stderr, "[child] Не удалось зарегистрировать atexit()\n");
            _exit(1);
        }

        for (int i = 0; i < 3; ++i) {
            fprintf(stdout, "[child] Работаю... i=%d\n", i);
            fflush(stdout);
            sleep(1);
        }
        int child_exit_code = 42; 
        fprintf(stdout, "[child] Завершаюсь с кодом %d\n", child_exit_code);
        fflush(stdout);
        _exit(child_exit_code);
    } else {
        
        fprintf(stdout, "[parent] Создан дочерний PID=%ld; мой PID=%ld, PPID=%ld\n",
                (long)pid, (long)getpid(), (long)getppid());
        fflush(stdout);

        int status = 0;
        pid_t w = waitpid(pid, &status, 0);
        if (w == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            fprintf(stdout, "[parent] Дочерний процесс %ld завершился. Код=%d\n", (long)w, code);
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            fprintf(stdout, "[parent] Дочерний процесс %ld завершён сигналом %d (%s)\n",
                    (long)w, sig, strsignal(sig));
        } else {
            fprintf(stdout, "[parent] Дочерний процесс %ld завершился в неопределённом состоянии\n", (long)w);
        }
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}


