//
// Created by yibaoshan@foxmail.com on 2025/2/26.
//
#include "kernel/types.h"
#include "user/user.h"

void sieve(int input_fd) {
    int p;

    // 每个进程只负责一个素数，所以读取到的数肯定是质数，参考 Java 版本的第一行打印函数
    if (read(input_fd, &p, sizeof(int)) <= 0) {
        exit(0); // 无数据或者写端已关闭，退出
    }

    printf("prime %d\n", p);

    int fd_pipe[2];
    pipe(fd_pipe); // 创建新管道

    if (fork() == 0) {
        // 子进程：处理下一层筛法
        close(fd_pipe[1]); // 关闭写端
        sieve(fd_pipe[0]); // 递归调用
        close(fd_pipe[0]);
        exit(0);
    } else {
        // 父进程：过滤当前素数的倍数
        close(fd_pipe[0]); // 关闭读端
        int num;
        while (read(input_fd, &num, sizeof(int)) > 0) {
            if (num % p != 0) {
                write(fd_pipe[1], &num, sizeof(int)); // 写入未被过滤的数，等同于 Java 版本的，向 ret 集合中 add 一条数据。
            }
        }
        close(input_fd);    // 关闭输入管道
        close(fd_pipe[1]);   // 关闭输出管道写端
        wait(0);            // 等待子进程结束
        exit(0);
    }
}

int main() {
    int initial_pipe[2];
    pipe(initial_pipe); // 创建初始管道

    if (fork() == 0) {
        // 子进程：开始筛法
        close(initial_pipe[1]); // 关闭写端
        sieve(initial_pipe[0]);
        close(initial_pipe[0]);
        exit(0);
    } else {
        // 主进程：生成初始数列
        close(initial_pipe[0]); // 关闭读端
        // 第一轮，把 2 ~ 35 通过管道通知给子进程
        for (int i = 2; i <= 35; i++) {
            write(initial_pipe[1], &i, sizeof(int));
        }
        close(initial_pipe[1]); // 关闭写端，无用端
        wait(0);               // 等待子进程结束
        exit(0);
    }
}
