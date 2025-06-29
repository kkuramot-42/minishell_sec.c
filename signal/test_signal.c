#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handler(int signum) {
    printf("Received signal %d\n", signum);
}

int main() {
    signal(SIGINT, handler); // Ctrl+C に対して handler を呼ぶ
    while (1) {
        printf("Running...\n");
        sleep(1);
    }
}
