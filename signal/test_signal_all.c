#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "test.h"

void handler(int signum) {
    printf("Handled signal %d\n", signum);
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handler;

    sigemptyset(&sa.sa_mask);            // 初期化
    sigaddset(&sa.sa_mask, SIGTERM);     // handler中はSIGTERMをブロック

    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);        // SIGINTに設定

    while (1) {
        printf("Running...\n");
        sleep(1);
    }
}