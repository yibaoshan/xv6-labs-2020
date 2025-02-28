//
// Created by yibaoshan@foxmail.com on 2025/2/27.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void exec_by_child(char *program, char **args) {
    // fork 出一个子进程去执行
    if (fork() == 0) {
        if (exec(program, args) == -1) {
            printf("\nexec faild, not found the %s!\n\n", program);
        }
        exit(0);
    }
    return;
}

int main(int argc, char *argv[]) {
    char buf[1024];                           // 指令缓冲区
    char *args_buf[128];                      // 参数缓冲区
    char *start_ptr = buf, *last_ptr = buf;   // 参数列表的开始、结束指针
    char **args = args_buf;                   // 当前参数的指针

    if (argc < 2) {
        fprintf(2, "Usage: <command> <params> | xargs <command>  <params>\n");
        exit(1);
    }

    // 读取所有传入的参数
    for (int i = 1; i < argc; i++) {
        *args = argv[i];
        args++;
    }

    char **cur_arg = args;

    // 循环读入参数
    while (read(0, start_ptr, 1) != 0) {

        // 遇到空格或换行符，则将参数结束符置为空，并记录参数的结束位置
        if (*start_ptr == ' ' || *start_ptr == '\n') {
            *start_ptr = '\0';          // 将当前字符替换为字符串结束符
            *(cur_arg++) = last_ptr;    // 将当前参数的起始地址存入参数列表
            last_ptr = start_ptr + 1;   // 更新last_ptr指向下一个字符

            // 如果读取到换行符 则执行子进程
            if (*start_ptr == '\n') {
                *cur_arg = 0;
                exec_by_child(argv[1], args_buf);
                cur_arg = args;
            }
        }
        start_ptr++;
    }

    // 如果还有未处理的参数，同样执行子进程
    if (cur_arg != args) {
        *start_ptr = '\0';
        *(cur_arg++) = last_ptr;
        *cur_arg = 0;
        exec_by_child(argv[1], args_buf);
    }

    while (wait(0) != -1) {
        // wait for child processes
    }
    exit(0);
}