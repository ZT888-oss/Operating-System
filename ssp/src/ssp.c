#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROCESSES 128
#define MAX_UNKNOWN 128
typedef struct {
  pid_t pid;
  int status;
  int alive;
  char *name;
} ssp_process_t;

static ssp_process_t process_table[MAX_PROCESSES];
static int process_count = 0;

typedef struct {
  pid_t pid;
  char *name;
  int status;
} adopted_process_t;

adopted_process_t adopted_table[MAX_UNKNOWN];
int adopted_count = 0;

void handle_signal(int signum) {
  if (signum != SIGCHLD)
    return; // ignore other signals

  int wstatus;
  pid_t pid;

  // Reap all exited children
  while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
    bool found = false;
    for (int i = 0; i < process_count; i++) {
      if (process_table[i].pid ==
          pid) {                  // look through to find the coressponding pid
        if (WIFEXITED(wstatus)) { // if exit normally
          process_table[i].status = WEXITSTATUS(wstatus);
        } else if (WIFSIGNALED(wstatus)) {
          process_table[i].status =
              128 + WTERMSIG(wstatus); // if the process terminates through a
                                       // signal set the status to be equal to
                                       // the signal number plus 128
        }
        process_table[i].alive = 0;
        found = true;
        break;
      }
    }
    if (!found && adopted_count < MAX_UNKNOWN) {
      //printf("check adopted child\n");
      adopted_table[adopted_count].pid = pid;
      adopted_table[adopted_count].status =
          WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : WTERMSIG(wstatus) + 128;
      adopted_table[adopted_count].name = strdup("<unknown>");
      adopted_count++;
    }
  }
}

void register_signal(int signum) {
  struct sigaction new_action = {0};
  sigemptyset(&new_action.sa_mask); // allow handling other signal while process
                                    // current signal
  new_action.sa_handler =
      handle_signal; // when signal arrives, call handle_signal() function
  new_action.sa_flags =
      SA_RESTART | SA_NOCLDSTOP; // SA_NOCLDSTOP: ignore children
                                 // stopping, only care about exits
                                 // SA_RESTART: maintain system call like read()
                                 // when it is interuppted
  if (sigaction(signum, &new_action, NULL) == -1) {
    int err = errno;
    perror("sigaction");
    exit(err);
  }
}

void ssp_init() {
  memset(process_table, 0,
         sizeof(process_table)); // set all array, buffer, struct to 0
  process_count = 0;

  // set attribute of subreaper:
  if (prctl(PR_SET_CHILD_SUBREAPER, 1) ==
      -1) { //// marks the calling process as a subreaper
    perror("prctl");
    exit(errno);
  }
  register_signal(SIGCHLD);
}

int ssp_create(char *const *argv, int fd0, int fd1, int fd2) {
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(errno);
  }
  if (pid == 0) {
    // set file descriptors 0, 1, and 2 to match the arguments fd0, fd1, and fd2
    if (dup2(fd0, 0) == -1 || // stdin
        dup2(fd1, 1) == -1 || dup2(fd2, 2) == -1) {
      perror("dup2");
      exit(errno);
    }
    // close other fd except 0,1,2
    DIR *dir = opendir("/proc/self/fd");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type ==
          DT_LNK) { // only consider files with d_type set to DT_LNK.
        int fd = atoi(entry->d_name);
        if (fd > 2 && fd != dirfd(dir))
          close(fd);
      }
    }
    closedir(dir);

    execvp(argv[0], argv);
    perror("execvp");
    exit(errno);
  }

  // record process ID of the newly created process, the name string, status
  char *copy_name = strdup(argv[0]);
  int ssp_id = process_count;
  process_table[process_count].pid = pid;
  process_table[process_count].alive = 1;
  process_table[process_count].status = -1; // indicate created process running
  process_table[process_count].name = copy_name;
  process_count++;
  return ssp_id;
}

int ssp_get_status(int ssp_id) {
  if (ssp_id == -1) {
    return -1;
  }
  if (process_table[ssp_id].alive)
    return -1;
  return process_table[ssp_id].status;
}

void ssp_send_signal(int ssp_id, int signum) {
  if (process_table[ssp_id].alive == 1)
    kill(process_table[ssp_id].pid,
         signum); // send a signal signum to the process referred to by ssp_id
}

void ssp_wait() {
  // bloack: wait unitl u know for sure
  while (1) {
    bool all_terminate = true;
    for (int i = 0; i < process_count; i++) {
      if (process_table[i].alive == 1) {
        all_terminate = false;
        break;
      }
    }
    if (all_terminate) { // all process terminate
      return;
    }
  }
}

void ssp_print() {
  pid_t pid;
  char *cmd;
  int status;
  size_t max_len = 0;

  for (int i = 0; i < process_count; i++) {
    size_t len = strlen(process_table[i].name);
    if (len > max_len)
      max_len = len;
  }
  size_t max_len1 = 0;
    for (int i = 0; i < adopted_count; i++) {
      size_t len = strlen(adopted_table[i].name);
      if (len > max_len1)
        max_len1 = len;
    }
size_t max = (max_len > max_len1) ? max_len : max_len1;

  printf("%7s %-*s %s\n", "PID", (int)max, "CMD" , "STATUS");
  for (int i = 0; i < process_count; i++) {
    pid = process_table[i].pid;
    cmd = process_table[i].name;
    status = process_table[i].status;
    if (strlen(cmd) == 2) {
      printf("%7d %-*s  %d\n", pid, (int)max, cmd,status);
    } else if (strlen(cmd) == 1) {
      printf("%7d %-*s   %d\n", pid, (int)max, cmd, status);
    } else if (strlen(cmd) >= 3) {
      printf("%7d %-*s %d\n", pid, (int)max, cmd, status);
    }
    //printf("%7d %-*s %-*d\n", pid, (int)max, cmd, (int)max, status);
  }
  //printf("get info of adopted child\n");
    
    for (int i = 0; i < adopted_count; i++) {
      printf("%7d %-*s %d\n", adopted_table[i].pid, (int)max,
             adopted_table[i].name, adopted_table[i].status);
    }
}