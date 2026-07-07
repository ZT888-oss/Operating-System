
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
  pid_t pid = fork();//child pid
  if (pid == -1) {
    return errno;
  }
  if (pid == 0) {
    sleep(2);
  } else {
    pid_t wait_pid = 0;//function waitpid() deifines child's termination status write into it
    int wstatus;  // not just an integer, a bit-encoded status word [signal info | core dump | exit code | flags ]
    unsigned int count = 0;
    while (wait_pid == 0) {
      ++count;
      printf("Calling wait (attempt %u)\n", count);
      wait_pid = waitpid(pid, &wstatus, WNOHANG);//waiting for child with PID:pid, &status: child exit status
      //WNOHANG → non-blocking:
     //If child has not exited yet, returns 0 immediately
    //If child has exited, returns child PID
    }
    if (wait_pid == -1) {
      int err = errno;
      perror("wait_pid");
      exit(err);
    }
    if (WIFEXITED(wstatus)) { //true if exit normally
      printf("Wait returned for an exited process! pid: %d, status: %d\n",
             wait_pid, WEXITSTATUS(wstatus)); //WEXITSTATUS(wstatus): gives child exit code
    } else {
      return ECHILD; //child doesn't exist or abnormal termination
    }
  }
  return 0;
}
