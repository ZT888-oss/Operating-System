#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
void handle_signal(int signum) {
  if (signum != SIGCHLD) {
    printf("Ignoring signal %d\n", signum);

    return;
  }
  printf("Calling wait\n");
  int wstatus;
  pid_t wait_pid = waitpid(-1, &wstatus, WNOHANG);//pid:-1 measn waiting for any process
  if (wait_pid == -1) {
    int err = errno;
    perror("wait_pid");
    exit(err);
  }
  if (WIFEXITED(wstatus)) {
    printf("Wait returned for an exited process! pid: %d, status: %d\n",
           wait_pid, WEXITSTATUS(wstatus));
  } else {
    exit(ECHILD);
  }
      exit(0);
}

void register_signal(int signum) {
  struct sigaction new_action = {0};
  sigemptyset(&new_action.sa_mask);//allow handling other signal while process current signal
  new_action.sa_handler = handle_signal;//when signal arrives, call handle_signal() function
  if (sigaction(signum, &new_action, NULL) == -1) {
    int err = errno;
    perror("sigaction");
    exit(err);
  }
}
int main() {
  //why sigint not be blocked?
  register_signal(SIGINT);
  register_signal(SIGTERM);
  register_signal(SIGCHLD);
  

  pid_t pid = fork();
  if (pid == -1) {
    return errno;
  }
  if (pid == 0) {
    sleep(2);
  } else {
    while (true) {
      printf("Time to go to sleep\n");
      sleep(9999);
    }
  }
  return 0;
}
