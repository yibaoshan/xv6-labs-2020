//
// Created by yibaoshan@foxmail.com on 2025/2/27.
//
#include "kernel/types.h"
#include "kernel/stat.h" 
#include "user/user.h"   
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// 递归查找指定目录下所有匹配目标文件名的文件
void find(char *path, const char *target_name) {

    char buf[1024];           // 路径缓冲区
    struct dirent de;        // 目录条目结构体
    struct stat st;          // 文件状态结构体
    int fd;                  // 文件描述符

    // 以只读模式打开目录，并返回文件描述符 fd，失败则直接 return
    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "error: cannot open %s\n", path);
        return;
    }

    // 获取文件的状态，stat 结构体包含：类型、索引号、文件的类型（文件、目录、设备）等信息
    if (fstat(fd, &st) < 0) {
        fprintf(2, "error: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 检查当前路径是否是目录
    if (st.type != T_DIR) {
        fprintf(2, "error: %s is not a directory\n", path);
        close(fd);
        return;
    }

    // 读取目录中的每一个条目
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // 跳过无效条目（inode号为0）
        if (de.inum == 0)
            continue;

        // 跳过 "." 和 ".." 目录（避免无限递归）
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;

        // 拼接完整路径：当前目录路径 + "/" + 文件名
        // 例如：path="a", de.name="b" → "a/b"
        strcpy(buf, path);
        char *p = buf + strlen(buf);
        *p++ = '/';
        strcpy(p, de.name);

        // 获取当前文件的状态
        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }

        // 如果是目录，递归处理
        if (st.type == T_DIR) {
            find(buf, target_name);     // 递归查找子目录
        } else if (st.type == T_FILE) { // 如果是文件，检查文件名是否匹配
            if (strcmp(de.name, target_name) == 0) {
                printf("%s\n", buf);    // 打印匹配的完整路径
            }
        }
    }

    close(fd); // 关闭目录文件描述符
}

int main(int argc, char *argv[]) {
    // 参数校验
    if (argc != 3) {
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}