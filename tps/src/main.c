#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

int isNumber(const char *s){
    if(*s == '\0') return 0;
    for(; *s != '\0'; s++){
        if(!isdigit((unsigned char)*s)) return 0;
    }
    return 1;
}

int main() {
    DIR *dir =opendir("/proc");
    if(!dir){
        perror("opendir");
        exit(1);
    }
    struct dirent *entry;

    //header
    printf("%5s %s\n", "PID", "CMD");
    while((entry = readdir(dir)) != NULL){
        //check if entry->d_name is number by using a helper function
        if(!isNumber(entry->d_name)) continue;
        char path[256];
        //check the file we want to open to get 'Name'(CMD)
        snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
        int fd = open(path, O_RDONLY); //open /proc/<pid>/status file in read-only mdoe

        if(fd <= 0) continue;

        char buf[4096];
        size_t n = read(fd, buf, sizeof(buf) - 1); // size -1 to leave a space for NULL-terminator character (\0)
        close(fd);

        if(n<=0) return 0;
        buf[n] = '\0';   //convert from bytes to C style string for strncmp
        //Extract name after: Name:\t
        char *line = strtok(buf, "\n"); //extract first line since file start with first line
        if(line && strncmp(line, "Name:\t", 6)==0){
            printf("%5s %s\n", entry->d_name, line+6);  //6 position after first character of name
        }
        
    }
    closedir(dir);
    return 0;
}
