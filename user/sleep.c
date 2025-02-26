//
// Created by yibaoshan@foxmail.com on 2025/2/25.
//
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    // 参数校验，如果输入错误，给用户提示正确用法。
    if (argc != 2) {
        printf("Usage: %s <ticks>\n", argv[0]);
        exit(1);
    }
    // 获取命令行传进来的参数，假如输入： sleep 3
    // argv[0] = sleep，第一个参数是程序名称 sleep。
    // argv[1] = 3，第二个参数是需要暂停多少个 tick
    char *arg = argv[1];
    // string 转 int，如果输入值非阿拉伯数字（比如输入字母或中文）则返回默认值 0
    int ticks = atoi(arg);
    printf("Ready to sleep for %d ticks...\n", ticks);
    // 调用系统提供的函数进行暂停
    sleep(ticks);
    printf("Sleep finished. Goodbye!\n");
    exit(0);
}