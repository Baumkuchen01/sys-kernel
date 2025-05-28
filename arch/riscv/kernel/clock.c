#include <sbi.h>
#include <private_kdefs.h>

void sbi_set_timer(uint64_t stime_value) {
  sbi_ecall(0x54494d45, 0, stime_value, 0, 0, 0, 0, 0);
}

// void clock_set_next_event(void) {
//   uint64_t time;

//   // 1. 使用 rdtime 指令读取当前时间
//   asm volatile(
//     "rdtime %[time]"
//     : [time] "=r" (time)
//     : : "memory"
//   );

//   // 2. 计算下一次中断的时间
//   uint64_t next = time + TIMECLOCK;

//   // 3. 调用 sbi_set_timer 设置下一次时钟中断
//   sbi_set_timer(next);
// }

void clock_set_next_event(void) {
  sbi_ecall(0x54494d45, 0, TIMECLOCK, 0, 0, 0, 0, 0);
}
