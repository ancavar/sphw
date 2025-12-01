#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

/*
 * Пинг-понг с использованием сигналов BSD (через POSIX sigaction)
 * 
 * Процесс A (родитель)  и процесс B (потомок)  чередуют состояния.
 * Синхронизация достигается с использованием SIGUSR1.
 * sigsuspend() используется для предотвращения гонок между проверкой флага и паузой.
 */

volatile sig_atomic_t sig_caught = 0;

void handle_sigusr1(int sig) {
    sig_caught = 1;
}

int main() {
    pid_t pid;
    struct sigaction sa;
    sigset_t mask, oldmask;

    // Мы блокируем SIGUSR1 изначально, чтобы если сигнал придет до того, как мы будем готовы 
    // спать (sigsuspend), он остался в ожидании, а не был потерян или прервал выполнение некорректно.
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    
    // Блокируем SIGUSR1
    if (sigprocmask(SIG_BLOCK, &mask, &oldmask) < 0) {
        perror("sigprocmask");
        exit(1);
    }

    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    // Процесс B
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid > 0) {
        printf("Process A (PID %d) started.\n", getpid());
        
        sleep(1); 

        while (1) {
            printf("[A] Ready -> Sending signal to B\n");
            kill(pid, SIGUSR1);
            
            printf("[A] Sleep...\n");
            
            // Ждем сигнал от B.
            // sigsuspend временно заменяет маску на oldmask (где SIGUSR1 разблокирован)
            // и приостанавливает процесс до получения сигнала.
            while (!sig_caught) {
                sigsuspend(&oldmask);
            }
            sig_caught = 0; 
            
            printf("[A] Signal received \n");
            sleep(1); 
        }

    } else {
        printf("Process B (PID %d) started.\n", getpid());
        pid_t ppid = getppid();

        while (1) {
            printf("[B] Sleep...\n");
            
            while (!sig_caught) {
                sigsuspend(&oldmask);
            }
            sig_caught = 0; 

            printf("[B] Signal received -> Ready\n");
            
            printf("[B] Sending signal to A\n");
            kill(ppid, SIGUSR1);
        }
    }

    return 0;
}
