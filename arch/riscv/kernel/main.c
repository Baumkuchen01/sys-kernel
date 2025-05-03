#include <printk.h>
#include <sbi.h>
#include <private_kdefs.h>
#include <proc.h>

_Noreturn void start_kernel(void) {
  printk("2025 ZJU Computer System III\n");

  schedule();
  while (1)
    ;
}
