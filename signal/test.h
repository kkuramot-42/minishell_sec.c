#ifndef TEST_H
#define TEST_H

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct sigaction
{
    void (*sa_handler)(int);    // 通常の handler
    void (*sa_sigaction)(int, siginfo_t *, void *); // 詳細情報付き
    sigset_t sa_mask;           // ハンドラ実行中にブロックするシグナル集合
    int sa_flags;               // 動作フラグ（例: SA_RESTART）
};

#endif