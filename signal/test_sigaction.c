#include "test.h"

void handler(int signum) {
    printf("Caught signal %d\n", signum);
}

int main()
{
    struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask); // ブロックするシグナル集合を初期化
    sa.sa_flags = SA_RESTART; // 中断されたシステムコールを再開させる

    sigaction(SIGINT, &sa, NULL); // SIGINT に handler を設定

    while (1) {
        printf("Working...\n");
        sleep(1);
    }
}
