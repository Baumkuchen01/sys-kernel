#include <mm.h>
#include <proc.h>
#include <private_kdefs.h>
#include <printk.h>
#include <stdlib.h>

static struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此
static struct task_struct *idle;           // idle 线程
struct task_struct *current;               // 当前运行线程

void __dummy(void);
void __switch_to(struct task_struct *prev, struct task_struct *next);

// 在这里添加或实现这些函数：
void dummy_task(void) {
  unsigned local = 0;
  unsigned prev_cnt = 0;
  while (1) {
    if (current->counter != prev_cnt) {
      if (current->counter == 1) {
        // 若 priority 为 1，则线程可见的 counter 永远为 1（为什么？）
        // 通过设置 counter 为 0，避免信息无法打印的问题
        current->counter = 0;
      }
      prev_cnt = current->counter;
      // printk("[PID = %" PRIu64 "] Running. local = %u\n", current->pid, ++local);
      printk("[P=%u] %u\n", current->pid, ++local);
    }
  }
}

void task_init(void){
  srand(2024);

  // 1. 调用 alloc_page() 为 idle 分配一个物理页
  idle = (struct task_struct *)alloc_page();

  // 2. 初始化 idle 线程：
  //   - state 为 TASK_RUNNING
  //   - pid 为 0
  //   - 由于其不参与调度，可以将 priority 和 counter 设为 0

  idle->state = TASK_RUNNING;
  idle->pid = 0;
  idle->priority = 0;
  idle->counter = 0;

  // 3. 将 current 和 task[0] 指向 idle
  current = idle;
  task[0] = idle;

  // 4. 初始化 task[1..NR_TASKS - 1]：
  //    - 分配一个物理页
  //    - state 为 TASK_RUNNING
  //    - pid 为对应线程在 task 数组中的索引
  //    - priority 为 rand() 产生的随机数，控制范围在 [PRIORITY_MIN, PRIORITY_MAX]
  //    - counter 为 0
  //    - 设置 thread_struct 中的 ra 和 sp：
  //      - ra 设置为 __dummy 的地址（见 4.3.2 节）
  //      - sp 设置为该线程申请的物理页的高地址
  for (int i = 1; i < NR_TASKS; i++)
  {
      task[i] = (struct task_struct *)alloc_page();
      task[i]->state = TASK_RUNNING;
      task[i]->pid = i;
      task[i]->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
      task[i]->counter = 0;
      task[i]->thread.ra = __dummy;
      task[i]->thread.sp = (void*)task[i] + 0x1000;
  }

  printk("...task_init done!\n");
}

void do_timer(void) {
  // 1. 如果当前线程时间片耗尽，则直接进行调度
  if (current->counter == 0)
    schedule();
  // 2. 否则将运行剩余时间减 1，若剩余时间仍然大于 0 则直接返回，否则进行调度
  else if (--current->counter == 0)
    schedule();
}

void schedule(void) {
  uint64_t c = 0;
  struct task_struct *next;
  while (1)
  {
    for (int i = 1; i < NR_TASKS; i++)
      if (task[i]->state == TASK_RUNNING && task[i]->counter > c)
      {
        c = task[i]->counter;
        next = task[i];
      }
    if (c)
      break;
    for (int i = 1; i < NR_TASKS; i++)
    {
      task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
      // printk("SET [PID = %" PRIu64 ", PRIORITY = %" PRIu64 ", COUNTER = %" PRIu64 "]\n", task[i]->pid, task[i]->priority, task[i]->counter);
    }
  }
  // printk("switch to [PID = %" PRIu64 ", PRIORITY = %" PRIu64 ", COUNTER = %" PRIu64 "]\n", next->pid, next->priority, next->counter);
  switch_to(next);
}

void switch_to(struct task_struct *next) {
  if (next != current) 
  {
    struct task_struct *prev = current;
    current = next;
    __switch_to(prev, next);
  }
}
