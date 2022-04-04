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

struct pid_info {
  char name[512];
  int pid;
  int ppid;
  struct pid_info* next;
};

int get_pid_list(struct pid_info **pid_info_list_o, int *pid_num_o){
  const char* dir_path = "/proc";
  DIR *d = opendir(dir_path);
  int pid_num = 0;
  if (!d) {
    printf("opendir failed, dir path %s", dir_path);
    return -1;
  }
  while (readdir(d) != NULL){
    pid_num++;
  }
  if (pid_num == 0) {
    printf("get pid_num 0");
    return -1;
  }
  rewinddir(d);
  struct pid_info* pid_info_list = (struct pid_info*)malloc(sizeof(struct pid_info)* pid_num);
  int index = 0;
  struct dirent *dir;
  while ((dir = readdir(d)) != NULL){
    int pid = atoi(dir->d_name);
    if (pid <= 0) {
      printf("skip for not legal pid, name %s\n", dir->d_name);
      continue;
    }
    char file_path[512];
    sprintf(file_path, "/proc/%s/stat", dir->d_name);
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
      printf("read pid file failed, file_name /proc/%s\n", dir->d_name);
      return -1;
    }
    char name[512], running_state[32];
    int cur_pid, ppid;
    if (fscanf(fp, "%d%s%s%d", &cur_pid, name, running_state, &ppid) != EOF){
      // consturct the value; 
      strncpy(pid_info_list[index].name, name, 512);
      pid_info_list[index].pid = cur_pid;
      pid_info_list[index].ppid = ppid;
      pid_info_list[index].next = NULL;
      printf("file_name %s, name %s, pid %d, ppid %d, next %p\n", 
        file_path, 
        pid_info_list[index].name, 
        pid_info_list[index].pid, 
        pid_info_list[index].ppid, 
        pid_info_list[index].next);
      index++;
    }
    fclose(fp);
  }
  if (d) {
    free(d);
    d = NULL;
  }
  *pid_num_o = pid_num;
  *pid_info_list_o = pid_info_list;
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
  int pid_num;
  struct pid_info* pid_info_list;
  error_code = get_pid_list(&pid_info_list, &pid_num);
  if(error_code != 0){
    return -1;
  }
  // construct tree, find the root of each pid, and construt the tree;
  
  // print the tree in bfs;
  if (pid_info_list) {
    free(pid_info_list);
    pid_info_list = NULL;
  }
  return 0;
}



