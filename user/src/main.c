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

#if defined(USER_MAIN) && !(USER_MAIN > 1000 && USER_MAIN < 1200)
#warning "Invalid definition of USER_MAIN"
#undef USER_MAIN
#endif

#ifndef USER_MAIN
// 你可以修改这一行来提供代码高亮
#define USER_MAIN SIGNAL
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
  // Test 1: Basic allocation and release
  unsigned int *ptr = (unsigned int*)malloc(sizeof(unsigned int));
  if (ptr == NULL) {
      printf("Test failed: malloc returned NULL\n");
      return 1;
  }
  
  *ptr = 0xDEADBEEF;  // Write test value
  if (*ptr != 0xDEADBEEF) {
      printf("Test failed: memory write/read error\n");
      free(ptr);
      return 1;
  }
  free(ptr);
  printf("Test 1 passed: basic allocation/release successful\n");

  // Test 2: Multiple allocation and release
  int *arr[10];
  for (int i = 0; i < 10; i++) {
      arr[i] = (int*)malloc(1024 * sizeof(int));  // Allocate large memory blocks
      if (arr[i] == NULL) {
          printf("Test failed: allocation #%d failed\n", i+1);
          // Free already allocated memory
          for (int j = 0; j < i; j++) free(arr[j]);
          return 1;
      }
      arr[i][0] = i;  // Write data
  }
  
  // Validate and release
  for (int i = 0; i < 10; i++) {
      if (arr[i][0] != i) {
          printf("Test failed: data validation error @ block %d\n", i);
          for (int j = 0; j < 10; j++) free(arr[j]);
          return 1;
      }
      free(arr[i]);
  }
  printf("Test 2 passed: multiple allocation/release successful\n");

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == READ

int var = 0;
char buffer[9];
int fd;

int main(void) {
  printf("\x1b[44m[U]\x1b[0m [PID = %d] Testing read() system call\n", getpid());
  
  // Test 1: Read from stdin (file descriptor 0)
  printf("Test 1: Reading from stdin (enter some text): ");
  
  ssize_t bytes_read = read(0, buffer, sizeof(buffer) - 1);
  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';  // Null terminate
    printf("Read %zd bytes: %s", bytes_read, buffer);
  } else if (bytes_read == 0) {
    printf("EOF reached\n");
  } else {
    printf("Read error\n");
  }
  
  // Test 2: Try to read from an invalid file descriptor
  printf("Test 2: Reading from invalid fd...\n");
  bytes_read = read(999, buffer, sizeof(buffer));
  if (bytes_read < 0) {
    printf("Expected error: read from invalid fd failed\n");
  } else {
    printf("Unexpected: read from invalid fd succeeded\n");
  }
  
  printf("Read tests completed\n");
  // Test 3: Testing scanf functionality
  printf("Test 3: Testing scanf - enter an integer: ");
  int input_num;
  int scanf_result = scanf("%d", &input_num);
  if (scanf_result == 1) {
    printf("Successfully read integer: %d\n", input_num);
  } else {
    printf("Failed to read integer\n");
  }

  printf("Test 4: Testing scanf - enter a string: ");
  char input_str[50];
  scanf_result = scanf("%s", input_str);
  if (scanf_result == 1) {
    printf("Successfully read string: %s\n", input_str);
  } else {
    printf("Failed to read string\n");
  }

  printf("Scanf tests completed\n");

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME);
  }
}

#elif USER_MAIN == SIGNAL

#include <signal.h>

int var = 0;
volatile int signal_received = 0;

void signal_handler(int sig) {
  printf("\x1b[41m[SIGNAL]\x1b[0m Received signal %d\n", sig);
  signal_received = 1;
}

int main(void) {
  printf("\x1b[44m[U]\x1b[0m [PID = %d] Testing signal() system call\n", getpid());
  
  sighandler_t old_handler = signal(SIGINT, signal_handler);
  printf("old_handler = %p\n", old_handler);
  // Test 1: Register signal handler for SIGINT (Ctrl+C)
  if (old_handler == SIG_ERR) {
    printf("Failed to register SIGINT handler\n");
    return 1;
  }
  printf("Test 1: SIGINT handler registered.\n");
  
  printf("signal_handler = %p\n", signal(SIGINT, signal_handler));

  kill(getpid(), SIGINT); // Trigger the signal handler immediately
  if (signal_received) {
    printf("Signal handler executed successfully.\n");
  } else {
    printf("Signal handler did not execute as expected.\n");
  }

  // Main loop
  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d, signal_received = %d\n", 
         getpid(), var++, signal_received);
    
    if (signal_received) {
      printf("Signal was caught! Resetting flag.\n");
      signal_received = 0;
    }
    
    delay(DELAY_TIME);
  }
}
#endif
