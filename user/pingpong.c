//
// Created by yibaoshan@foxmail.com on 2025/2/25.
//
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {

    // 题目无关，测试 fork 后，父进程修改普通变量子进程是否会同步修改
    int i = 100;

    // 创建两个管道描述符，前者用于父进程向子进程通信（读和写），后者用于子进程向父进程通信（读和写）
    // 每个管道描述符长度为 2，其中，0 用于读数据，1 用于写数据。
    int fd_p2c[2], fd_c2p[2];

    // 调用 pipe() 系统函数，创建两个管道
    pipe(fd_p2c);
    pipe(fd_c2p);

    // 调用系统函数 fork() 创建新的进程，父子进程都会接着从本行代码开始向下执行，上面的代码就不用管了。
    if (fork() == 0) {
        // 从这里开始，执行的是子进程代码
        sleep(10);
        // 打印当前 pid、i 的地址和值，以及两个管道地址，fork 会复制父进程的文件描述符表，所以父子进程打印的地址是相同的
        printf("here's child process, pid = %d, i = %d, i addr is %pn, fd_p2c addr is %pn, fd_c2p addr is %pn \n\n", getpid(), i, &i, &fd_p2c, &fd_c2p);

        int port_read = fd_p2c[0];
        int port_write = fd_c2p[1];

        char content_receive[1024] = {0};
        char content_send[1024] = {"daddy!\n"};

        // 读取父进程发送的消息，默认阻塞调用
        read(port_read, content_receive, sizeof(content_receive));
        printf("child received: %s", content_receive);
        printf("%d: received ping\n\n", getpid());

        // 写入消息至管道，回复父进程
        write(port_write, content_send, sizeof(content_send));

        sleep(10);

        exit(0);
    } else {
        // 不为 0 则表示是父进程的代码
        i = 101;
        printf("here's parent process, pid = %d, i = %d, i addr is %pn, fd_p2c addr is %pn, fd_c2p addr is %pn \n", getpid(), i, &i, &fd_p2c, &fd_c2p);

        int port_read = fd_c2p[0];
        int port_write = fd_p2c[1];

        char content_receive[1024] = {0};
        char content_send[1024] = {"call me daddy please \n"};

        // 题目要求父进程先发送消息
        write(port_write, content_send, sizeof(content_send));

        // 发完消息等待子进程回复
        read(port_read, content_receive, sizeof(content_receive));
        printf("parent received: %s", content_receive);
        printf("%d: received pong\n\n", getpid());

        sleep(10);

        exit(0);
    }
}