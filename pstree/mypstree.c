// 获取命令行参数，-p -n -V --show-pids --numeric-sort --version
// 获取进程信息，pid name 父子关系
   // parse process name
   // generate an array names[pid]=name
   // generate an array ppid[pid]= parent pid, through function getppid()
// 根据参数组织进程参数
// 打印
#include <stdio.h>
#include <assert.h>

int main(int argc, char* argv[]){
    for (int i = 1; i < argc; i++) {
        assert(argv[i]);
        printf("%s\t", argv[i]);
    }
    printf("%ld", sizeof(int));
    return 0;
}
