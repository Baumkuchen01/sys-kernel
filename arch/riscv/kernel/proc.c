#include <mm.h>
#include <proc.h>
#include <private_kdefs.h>
#include <printk.h>
#include <stdlib.h>
#include <sbi.h>
#include <string.h>

#define VPN0(x) (((uint64_t)(x) >> 12) & 0x1ff)
#define VPN1(x) (((uint64_t)(x) >> 21) & 0x1ff)
#define VPN2(x) (((uint64_t)(x) >> 30) & 0x1ff)
#define VA2PA(x) ((uint64_t)(x) - PA2VA_OFFSET)
#define PA2VA(x) ((uint64_t)(x) + PA2VA_OFFSET)

#define PTE_V 0x001
#define PTE_R 0x002
#define PTE_W 0x004
#define PTE_X 0x008
#define PTE_U 0x010
#define PTE_G 0x020
#define PTE_A 0x040
#define PTE_D 0x080

extern uint64_t swapper_pg_dir[];
extern uint8_t _suapp[], _euapp[];

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
      printk("[PID = %" PRIu64 "] Running. local = %u\n", current->pid, ++local);
      // printk("[P=%u] %u\n", current->pid, ++local);
    }
  }
}

void task_init_create_mapping(uint64_t pgtbl[static PGSIZE / 8], void *va, void *pa, uint64_t sz, uint64_t perm) {

  uint64_t VA = (uint64_t)va, PA = (uint64_t)pa;
  while (VA < (uint64_t)va + sz)
  {
    uint64_t vpn0 = VPN0(VA);
    uint64_t vpn1 = VPN1(VA);
    uint64_t vpn2 = VPN2(VA);

    if (!(pgtbl[vpn2] & PTE_V)) {
      uint64_t newpage = VA2PA(alloc_page());
      pgtbl[vpn2] = (newpage >> 12 << 10) | PTE_V;
    }

    uint64_t *pgtbl1 = (uint64_t *)PA2VA((pgtbl[vpn2] >> 10 << 12));

    if (!(pgtbl1[vpn1] & PTE_V)) {
      uint64_t newpage = VA2PA(alloc_page());
      pgtbl1[vpn1] = (newpage >> 12 << 10) | PTE_V;
    }

    uint64_t *pgtbl0 = (uint64_t *)PA2VA((pgtbl1[vpn1] >> 10 << 12));
    pgtbl0[vpn0] = (PA >> 12 << 10) | perm | PTE_V | PTE_A | PTE_D;

    VA += PGSIZE;
    PA += PGSIZE;
  }
  
  printk("pgtbl = %p: map [%p, %p) -> [%p, %p), perm = 0x%lx, size = %lu\n", 
         (void *)VA2PA(pgtbl), va, (void *)((uint64_t)va + sz), pa, (void *)((uint64_t)pa + sz), perm, sz);
  return;
}

void task_init(void){
  srand(2025);

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
    task[i]->thread.ra = (uint64_t)__dummy;
    task[i]->thread.sp = (uint64_t)task[i] + 0x1000;
    task[i]->thread.sepc = USER_START;
    uint64_t sstatus = csr_read(sstatus);
    sstatus &= ~(1 << 8);
    sstatus |= (1 << 5) | (1 << 18);
    task[i]->thread.sstatus = sstatus;
    
    task[i]->thread.sscratch = USER_END;
    task[i]->thread.stval = 0;
    task[i]->thread.scause = 0;

    task[i]->pgd = (pagetable_t)alloc_page();
    memcpy(task[i]->pgd, swapper_pg_dir, PGSIZE);
    uint64_t umode_stack_pa = VA2PA(alloc_page()), umode_stack_va = USER_END - PGSIZE;
    task_init_create_mapping(task[i]->pgd, (void *)umode_stack_va, (void *)umode_stack_pa, PGSIZE, PTE_R | PTE_W | PTE_U);

    uint64_t uapp_size = _euapp - _suapp;
    void *uapp_copy = alloc_pages((uapp_size - 1) / PGSIZE + 1);
    memcpy(uapp_copy, _suapp, uapp_size);
    task_init_create_mapping(task[i]->pgd, (void *)USER_START, (void *)VA2PA(uapp_copy), uapp_size, PTE_R | PTE_W | PTE_X | PTE_U);
    task[i]->pgd = (pagetable_t)((VA2PA(task[i]->pgd) >> 12) | (8ull << 60));
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
      printk("SET [PID = %" PRIu64 ", PRIORITY = %" PRIu64 ", COUNTER = %" PRIu64 "]\n", task[i]->pid, task[i]->priority, task[i]->counter);
    }
  }
  printk("switch to [PID = %" PRIu64 ", PRIORITY = %" PRIu64 ", COUNTER = %" PRIu64 "]\n", next->pid, next->priority, next->counter);
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
