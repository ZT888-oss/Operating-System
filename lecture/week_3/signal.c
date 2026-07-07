
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
 void handle_signal(int signum) {
  printf("Ignoring signal %d\n", signum);
}
void register_signal(int signum) {
// Fields in struct sigaction include:
// sa_handler → pointer to the function to call when the signal occurs
// sa_mask → signals to block while handling this signal
// sa_flags → special options
  struct sigaction new_action = {0};  // initialize struct, sigaction is for handling signals
  sigemptyset(&new_action.sa_mask); //clear signal mask, eg: empty list → don’t block anything, multitasking allowed
  new_action.sa_handler = handle_signal;
  if (sigaction(signum, &new_action, NULL) == -1) {//“For signal signum, use this new_action struct as the handler”， NULL means we don’t care about the old action
    int err = errno;
    perror("sigaction");
    exit(err);
  }
}
int main(int argc, char *argv[]) {   //argc: argument count,      argv:argument vector(array of strings)
  if (argc > 2) {//allows argc[0]: read from terminal, argc[1]: read from file,anything else is considered invalid usage
    return EINVAL;     //in <error.h> library, invalid argument
  }
  if (argc == 2) {//one file argument, read from file, eg: ./signal input.txt
    close(0); //since 0 in linux represent stdin, let FD = 0 becomes available, open() will assign FD = 0,Now read(0, ...) reads from the file instead of the keyboard
    int fd = open(argv[1], O_RDONLY); //argv[0] is always the program name, e.g. "./signal".
                                                    //argv[1] is the first argument the user provides, e.g. a filename "input.txt"
    if (fd == -1) {
      int err = errno;
      perror("open");
      return err;
    }
  }

  //argc = 1 → no file argument → read from terminal (stdin)
  register_signal(SIGINT);  //ignore SIGINT and SIGTERM
  register_signal(SIGTERM);
  char buffer[4096];
  ssize_t bytes_read;
  while ((bytes_read = read(0, buffer, sizeof(buffer))) != 0) {
    if (bytes_read == -1) {
      if (errno == EINTR) {    //EINTR: signal interuptted, retry
        continue;
      } else {      //if real erro: stop
        break;     //break will exit the nearest encloseing loop
      }
    }
    ssize_t bytes_written = write(1, buffer, bytes_read);
    if (bytes_written == -1) {
      int err = errno;
      perror("write");
      return err;
    }
    assert(bytes_read == bytes_written);   //if fasle, program terminate immediatly
  }
  if (bytes_read == -1) {
    int err = errno;
    perror("read");
    return err;
  }
  assert(bytes_read == 0);
  return 0;
}
