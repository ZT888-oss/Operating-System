
#include <assert.h>     //If this assumption is false, something is seriously wrong.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
int main() {
  char buffer[4096];
  ssize_t bytes_read;  //signed szie type, since read() can rturn -1

  //read() first tells kernel copy whatever the user types into my buffer
  while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0) { //read() return bytes read, max read 4096 characters
    ssize_t bytes_written = write(1, buffer, bytes_read);
    if (bytes_written == -1) {
      int err = errno;
      perror("write");
      return err;
    }
    assert(bytes_read == bytes_written); //assert() checks if "number of bytes read == number of bytes written"
  }
  if (bytes_read == -1) {
    int err = errno;
    perror("read");
    return err;
  }
  assert(bytes_read == 0);// bytes_read == 0 mean EOF
  return 0;
}
