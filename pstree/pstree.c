#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>

struct Args {
  int show_pids;
  int numeric_sort;
  int version;
};

void print_args(int argc, char *argv[]){
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
}

int process_args(int argc, char *argv[], struct Args *args){
  for (int i = 1; i < argc; i++) {
    char* argv_i = argv[i];
    if (strcmp("-p", argv_i) == 0 || strcmp("--show-pids", argv_i) == 0){
      args->show_pids = 1;
    } else if (strcmp("-n", argv_i) == 0 || strcmp("--numeric-sort", argv_i) == 0){
      args->numeric_sort = 1;
    } else if (strcmp("-V", argv_i) == 0 || strcmp("--version", argv_i) == 0) {
      args->version = 1;
    } else {
      printf("invalid option -- '%s'\n", argv_i);
      return -1;
    }
  }
  return 0;
}

void print_usage(){
  printf("Usage: pstree [-pn]\n");
  printf("pstree -V\n");
  printf("Display a tree of process\n\n");
  printf("-p, --show-pids show pids of each process\n");
  printf("-n, ---numeric-sort show pid from small to big\n");
  printf("-V, --version show the version info"); 
}

void print_version_info(){
  printf("pstree (PSmisc) UNKNOWN\n");
  printf("Copyright (C) 2020~2023 xiaoweilong\n");
}

int get_pid_list(int **pid_o, int *pid_num_o){
  DIR *d;
  struct dirent *dir;
  d = opendir("/proc");
  int pid_num = 0;
  if(d) {
    while ((dir = readdir(d)) != NULL && dir->d_type == DT_REG){
      pid_num++;
    }
  }
  if (pid_num == 0) {
    return -1;
  }
  int* pid_array = (int*)malloc(sizeof(int)*pid_num);
  int index = 0;
  if(d) {
    while ((dir = readdir(d)) != NULL && dir->d_type == DT_REG){
      int pid = atoi(dir->d_name);
      if (pid < 0) {
        printf("ignore error file %s", dir->d_name);
      }
      pid_array[index++] = pid;
    }
  }
  *pid_o = pid_array; 
  *pid_num_o = pid_num;
  return 0;
}

int main(int argc, char *argv[]) {
  // print args;
  print_args(argc, argv);
  // process parameter;
  struct Args args;
  args.numeric_sort = 0;
  args.show_pids = 0;
  args.version = 0;

  int error_code = process_args(argc, argv, &args);
  if (error_code != 0) {
    print_usage();
    return -1;
  }

  if (args.version != 0) {
    print_version_info();
    return 0;
  }

  // get pid list;

  // construct tree, find the root of each pid, and construt the tree;
  // print the pid tree;

  return 0;
}



