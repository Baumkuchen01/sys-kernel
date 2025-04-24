#include <printk.h>
#include <sbi.h>
#include <private_kdefs.h>

_Noreturn void start_kernel(void) {
  printk("2025 ZJU Computer System III\n");

  // 等待第一次时钟中断
  while (1)
    ;
}
