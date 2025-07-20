#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// define some tests
#define PFH1 1001
#define PFH2 1002
#define FORK1 1101
#define FORK2 1102
#define FORK3 1103
#define FORK4 1104
#define MALLOC 1145
#define READ 1146
#define SIGNAL 1147
#define FILE_SYSTEM 1148

#if defined(USER_MAIN) && !(USER_MAIN > 1000 && USER_MAIN < 1200)
#warning "Invalid definition of USER_MAIN"
#undef USER_MAIN
#endif

#ifndef USER_MAIN
// 你可以修改这一行来提供代码高亮
#define USER_MAIN FILE_SYSTEM
#endif

#define DELAY_TIME 1247

static uint64_t user_clock(void) {
  uint64_t ret;
  asm volatile("rdtime %0" : "=r"(ret));
  return ret / 10;
}

static void delay(unsigned long ms) {
  uint64_t prev_clock = user_clock();
  while (user_clock() - prev_clock < ms * 1000)
    ;
}

#if USER_MAIN == PFH1

int main(void) {
  register const void *const sp asm("sp");

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d, sp = %p]\n", getpid(), sp);
    delay(DELAY_TIME);
  }
}

#elif USER_MAIN == PFH2

const char *const xdigits = "0123456789abcdef";
char space[0x2000] __attribute__((aligned(0x1000)));
size_t i;

int main(void) {
  while (1) {
    i = 0;
    printf("\x1b[44m[U]\x1b[0m [PID = %d] ", getpid());
    while (i < sizeof(space)) {
      space[i] = xdigits[i % 16];
      printf("\x1b[4%cm%c\x1b[0m", xdigits[rand() % 8], space[i]);
      i++;
      delay(1);
    }
    printf("\n");
  }
}

#elif USER_MAIN == FORK1

int var = 0;

int main(void) {
  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";

  while (1) {
    printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK2

int var = 0;
char space[0x2000] __attribute__((aligned(0x1000)));

int main(void) {
  for (int i = 0; i < 3; i++) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME);
  }

  memcpy(&space[0x1000], "ZJU Sys3 Lab5", 14);

  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";

  printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] Message: %s\n", ident, getpid(), &space[0x1000]);
  while (1) {
    printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK3

int var = 0;

int main(void) {
  printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
  fork();
  fork(); // multiple references to one page

  printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
  fork();

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK4

#define LARGE 1000

int var = 0;
long bigarr[LARGE] __attribute__((aligned(0x1000))) = {};

int fib(int times) {
  if (times <= 2) {
    return 1;
  } else {
    return fib(times - 1) + fib(times - 2);
  }
}

const char *suffix(int num) {
  num %= 100;
  int i = num % 10;
  if (i == 1 && num != 11) {
    return "st";
  } else if (i == 2 && num != 12) {
    return "nd";
  } else if (i == 3 && num != 13) {
    return "rd";
  } else {
    return "th";
  }
}

int main(void) {
  for (int i = 0; i < LARGE; i++) {
    bigarr[i] = 3 * i + 1;
  }

  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";
  printf("\x1b[44m[U]\x1b[0m fork returns %d\n", pid);

  while (1) {
    var = 0;
    while (var < LARGE) {
      printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] the %d%s fibonacci number is %d and "
             "the %d%s number in the big array is %ld\n",
             ident, getpid(), var, suffix(var), fib(var), LARGE - 1 - var, suffix(LARGE - 1 - var),
             bigarr[LARGE - 1 - var]);
      var++;
      delay(100);
    }
  }
}

#elif USER_MAIN == MALLOC

int var = 0;

int main(void) {
  unsigned int *ptr = (unsigned int*)malloc(sizeof(unsigned int));
  if (ptr == NULL) {
      printf("test failed: malloc returned NULL\n");
      return 1;
  } else {
      printf("malloc successful, allocated %p\n", ptr);
  }
  
  *ptr = 0xDEADBEEF;
  printf("writing value 0x%X to allocated memory\n", *ptr);
  if (*ptr != 0xDEADBEEF) {
      printf("test failed: memory write/read error\n");
      free(ptr);
      return 1;
  } else {
      printf("memory write/read successful, value = 0x%X\n", *ptr);
  }
  free(ptr);
  printf("memory freed successfully\n");

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == READ

int var = 0;

int main(void) {
  printf("enter an integer: ");
  int input_num;
  int scanf_result = scanf("%d", &input_num);
  printf("\n");
  if (scanf_result == 1) {
    printf("successfully read integer: %d\n", input_num);
  } else {
    printf("failed to read integer\n");
  }

  printf("enter a string: ");
  char input_str[50];
  scanf_result = scanf("%s", input_str);
  printf("\n");
  if (scanf_result == 1) {
    printf("successfully read string: %s\n", input_str);
  } else {
    printf("failed to read string\n");
  }

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME);
  }
}

#elif USER_MAIN == SIGNAL

#include <signal.h>

int var = 0;
const char *ident;

void signal_handler(int sig) {
  printf("\x1b[41m[SIGNAL-%s]\x1b[0m Received signal %d\n", ident, sig);
}

int main(void) {
  signal(SIGINT, signal_handler);
  printf("signal handler registered\n");
  signal(SIGINT, signal_handler);
  pid_t pid = fork();
  ident = pid ? "PARN" : "CHLD";
  printf("raise(SIGINT) in [PID = %d]\n", getpid());
  raise(SIGINT);
  if (pid == 0) {
    while (1) {
      printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
      delay(DELAY_TIME);
    }
  } else {
    while (1) {
      printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
      kill(pid, SIGINT);
      delay(DELAY_TIME);
    }
  }
}

#elif USER_MAIN == FILE_SYSTEM

#include "stdio.h"
#include "unistd.h"
#include "string.h"

#define CAT_BUF_SIZE 509

char string_buf[2048];
char filename[2048];

int atoi(char* str) {
    int ret = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        ret = ret * 10 + str[i] - '0';
    }
    return ret;
}

char *get_param(char *cmd) {
    while (*cmd == ' ') {
        cmd++;
    }
    int pos = 0;
    while (*cmd != '\0' && *cmd != ' ') {
        string_buf[pos++] = *(cmd++);
    }
    string_buf[pos] = '\0';
    return string_buf;
}

char *get_string(char *cmd) {
    while (*cmd == ' ') {
        cmd++;
    }

    if (*cmd == '"') { // quote wrapped
        cmd++;
        int pos = 0;
        while (*cmd != '"') {
            string_buf[pos++] = *(cmd++);
        }
        string_buf[pos] = '\0';
        return string_buf;
    } else {
        return get_param(cmd);
    }
}

void parse_cmd(char *cmd, int len) {
    if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o') {
        cmd += 4;
        char *echo_content = get_string(cmd);
        len = strlen(echo_content);
        cmd += len;
        write(1, echo_content, len);
        write(1, "\n", 1);
    // } else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't') {
    //     char *filename = get_param(cmd + 3);
    //     char last_char;
    //     int fd = open(filename, O_RDONLY);
    //     if (fd == -1) {
    //         printf("can't open file: %s\n", filename);
    //         return;
    //     }
    //     char cat_buf[CAT_BUF_SIZE];
    //     while (1) {
    //         int num_chars = read(fd, cat_buf, CAT_BUF_SIZE);
    //         if (num_chars == 0) {
    //             if (last_char != '\n') {
    //                 printf("$\n");
    //             }
    //             break;
    //         }
    //         for (int i = 0; i < num_chars; i++) {
    //             if (cat_buf[i] == 0) {
    //                 write(1, "x", 1);
    //             } else {
    //                 write(1, &cat_buf[i], 1);
    //             }
    //             last_char = cat_buf[i];
    //         }
    //     }
    //     close(fd);
    // } else if (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't' ) {
    //     cmd += 4;
    //     while (*cmd == ' ' && *cmd != '\0') {
    //         cmd++;
    //     }
    //     char* temp = get_param(cmd);
    //     int len = strlen(temp); 
    //     char filename[len + 1];
    //     for (int i = 0; i < len; i++) {
    //         filename[i] = temp[i];
    //     }
    //     filename[len] = '\0';
    //     cmd += len;

    //     while (*cmd == ' ' && *cmd != '\0') {
    //         cmd++;
    //     }
    //     temp = get_param(cmd);
    //     len = strlen(temp);
    //     char offset[len + 1];
    //     for (int i = 0; i < len; i++) {
    //         offset[i] = temp[i];
    //     }
    //     offset[len] = '\0';
    //     cmd += len;

    //     while (*cmd == ' ' && *cmd != '\0') {
    //         cmd++;
    //     }
    //     temp = get_string(cmd);
    //     len = strlen(temp);
    //     char content[len + 1];
    //     for (int i = 0; i < len; i++) {
    //         content[i] = temp[i];
    //     }
    //     content[len] = '\0';
    //     cmd += len;

    //     int offset_int = atoi(offset);

    //     int fd = open(filename, O_RDWR);
    //     lseek(fd, offset_int, SEEK_SET);
    //     write(fd, content, len);
    //     close(fd);
    } else {
        printf("command not found: %s\n", cmd);
    }
}

int main() {
    write(1, "hello, stdout!\n", 15);
    write(2, "hello, stderr!\n", 15);

    char read_buf[2];
    char line_buf[128];
    int char_in_line = 0;
    printf(YELLOW "SHELL > " CLEAR);
    while (1) {
        read(0, read_buf, 1);
        if (read_buf[0] == '\r') {
            write(1, "\n", 1);
        } else if (read_buf[0] == 0x7f) {
            if (char_in_line > 0) {
                write(1, "\b \b", 3);
                char_in_line--;
            }
            continue;
        }
        write(1, read_buf, 1);
        if (read_buf[0] == '\r') {
            line_buf[char_in_line] = '\0';
            parse_cmd(line_buf, char_in_line);
            char_in_line = 0;
            printf(YELLOW "SHELL > " CLEAR);
        } else {
            line_buf[char_in_line++] = read_buf[0];
        }
    }
    return 0;
}

#endif