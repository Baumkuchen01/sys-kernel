#include <mm.h>
#include <proc.h>
#include <private_kdefs.h>
#include <printk.h>
#include <stdlib.h>
#include <sbi.h>
#include <vm.h>
#include <string.h>
#include <elf.h>

extern uint64_t swapper_pg_dir[];
extern uint8_t _suapp[], _euapp[];

static struct task_struct *task[NR_TASKS]; // 线程数组，所有的线程都保存在此
static struct task_struct *idle;           // idle 线程
struct task_struct *current;               // 当前运行线程
long task_num;

void __dummy(void);
void __switch_to(struct task_struct *prev, struct task_struct *next);
void ret_from_fork(void);

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

void load_program(struct task_struct *task) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_suapp;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(_suapp + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr *phdr = phdrs + i;
        if (phdr->p_type == PT_LOAD) {
            // alloc space and copy content
            // do mapping
            // code...
            uint64_t flags = 0;
            if (phdr->p_flags & PF_R) {
                flags |= VM_READ;
            }
            if (phdr->p_flags & PF_W) {
                flags |= VM_WRITE;
            }
            if (phdr->p_flags & PF_X) {
                flags |= VM_EXEC;
            }
            do_mmap(task->mm, phdr->p_vaddr, phdr->p_memsz, phdr->p_offset, phdr->p_filesz, flags);
            printk("Load program: p_vaddr = 0x%lx, p_memsz = 0x%lx, p_offset = 0x%lx, p_filesz = 0x%lx, flags = 0x%lx\n",
                   phdr->p_vaddr, phdr->p_memsz, phdr->p_offset, phdr->p_filesz, flags);
          }
    }
    task->thread.sepc = ehdr->e_entry;
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
  task_num = 1;
  for (int i = 1; i <= task_num; i++)
  {
    task[i] = (struct task_struct *)alloc_page();
    task[i]->mm = (struct mm_struct *)alloc_page();
    task[i]->mm->mmap = NULL;
    load_program(task[i]);

    task[i]->state = TASK_RUNNING;
    task[i]->pid = i;
    task[i]->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
    task[i]->counter = 0;
    task[i]->thread.ra = (uint64_t)__dummy;
    task[i]->thread.sp = (uint64_t)task[i] + PGSIZE;
    // task[i]->thread.sepc = USER_START;
    uint64_t sstatus = csr_read(sstatus);
    sstatus &= ~(1 << 8);
    sstatus |= (1 << 5) | (1 << 18);
    task[i]->thread.sstatus = sstatus;
    
    task[i]->thread.sscratch = USER_END;
    task[i]->thread.stval = 0;
    task[i]->thread.scause = 0;

    task[i]->pgd = (pagetable_t)alloc_page();
    memcpy(task[i]->pgd, swapper_pg_dir, PGSIZE);
    task[i]->pgd = (pagetable_t)((VA2PA(task[i]->pgd) >> 12) | (8ull << 60));

    uint64_t uapp_size = _euapp - _suapp;
    task[i]->mm->start_brk = task[i]->mm->brk = (unsigned long)PGROUNDUP(USER_START + uapp_size);
    
    struct sighand_struct* sighand = (struct sighand_struct *)alloc_page();
    for (int j = 0; j < _NSIG; j++) {
      sighand->action[j].sa_handler = SIG_DFL;
      sighand->action[j].sa_flags = 0;
      for (int k = 0; k < _NSIG_WORDS; k++) {
        sighand->action[j].sa_mask.sig[k] = 0;
      }
    }
    task[i]->sighand = sighand;

    struct signal_struct* signal = (struct signal_struct *)alloc_page();
    for (int j = 0; j < _NSIG_WORDS; j++) {
      signal->sigpending.sig[j] = 0;
      signal->blocked.sig[j] = 0;
    }
    task[i]->signal = signal;
    task[i]->flags = 0;

    task[i]->files = file_init();

    // do_mmap(task[i]->mm, (void *)USER_START, uapp_size, VM_READ | VM_WRITE | VM_EXEC);
    do_mmap(task[i]->mm, USER_END - PGSIZE, PGSIZE, 0, 0, VM_READ | VM_WRITE | VM_ANON);
    do_mmap(task[i]->mm, task[i]->mm->start_brk, 0, 0, 0, VM_READ | VM_WRITE | VM_ANON);
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
    for (int i = 1; i <= task_num; i++)
      if (task[i]->state == TASK_RUNNING && task[i]->counter > c)
      {
        c = task[i]->counter;
        next = task[i];
      }
    if (c)
      break;
    for (int i = 1; i <= task_num; i++)
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

struct vm_area_struct *find_vma(struct mm_struct *mm, uint64_t va) {
  struct vm_area_struct *vma = mm->mmap;
  while (vma && (vma->vm_start > va || vma->vm_end < va))
    vma = vma->vm_next;
  return vma;
}

uint64_t do_mmap(struct mm_struct *mm, uint64_t addr, uint64_t len, uint64_t vm_pgoff, uint64_t vm_filesz, uint64_t flags) {
  struct vm_area_struct *new_vma = (struct vm_area_struct *)alloc_page();
  new_vma->vm_mm = mm;
  new_vma->vm_start = addr;
  new_vma->vm_end = addr + len;
  new_vma->vm_pgoff = vm_pgoff;
  new_vma->vm_filesz = vm_filesz;
  new_vma->vm_flags = flags;
  new_vma->vm_prev = NULL;
  new_vma->vm_next = mm->mmap;
  if (mm->mmap) {
    mm->mmap->vm_prev = new_vma;
  }
  mm->mmap = new_vma;
  return addr;
}

uint64_t *walk_page_table(uint64_t *pgd, uint64_t va) {
  uint64_t vpn0 = VPN0(va), vpn1 = VPN1(va), vpn2 = VPN2(va);
  if (!(pgd[vpn2] & PTE_V)) {
    return NULL;
  }
  uint64_t *pgtbl1 = (uint64_t *)PA2VA((pgd[vpn2] >> 10 << 12));
  if (!(pgtbl1[vpn1] & PTE_V)) {
    return NULL;
  }
  uint64_t *pgtbl0 = (uint64_t *)PA2VA((pgtbl1[vpn1] >> 10 << 12));
  if (!(pgtbl0[vpn0] & PTE_V)) {
    return NULL;
  }
  return &pgtbl0[vpn0];
}

long do_fork(struct pt_regs *regs) {
  long pid = ++task_num;
  // printk("do_fork: %ld -> %ld\n", current->pid, pid);
  struct task_struct *child = (struct task_struct *)alloc_page();
  child->state = TASK_RUNNING;
  child->pid = pid;
  child->priority = PRIORITY_MIN + rand() % (PRIORITY_MAX - PRIORITY_MIN + 1);
  child->counter = 0;
  child->thread.ra = (unsigned long)ret_from_fork;
  child->thread.sp =  (uint64_t)regs + ((uint64_t)child - (uint64_t)current);
  child->thread.sstatus = current->thread.sstatus;
  child->thread.sscratch = csr_read(sscratch);

  for (int i = 0; i < 12; i++) {
      child->thread.s[i] = current->thread.s[i];
  }
  memcpy((void *)child->thread.sp, (void *)regs, (uint64_t)current + PGSIZE - (uint64_t)regs);
  struct pt_regs *pt_regs_child = (struct pt_regs *)child->thread.sp;
  pt_regs_child->x[2] = regs->x[2];
  pt_regs_child->x[10] = 0;
  pt_regs_child->sepc = regs->sepc + 4;

  struct sighand_struct* sighand = (struct sighand_struct *)alloc_page();
  for (int i = 0; i < _NSIG; i++) {
    sighand->action[i] = current->sighand->action[i];
  }
  child->sighand = sighand;

  struct signal_struct* signal = (struct signal_struct *)alloc_page();
  for (int i = 0; i < _NSIG_WORDS; i++) {
    signal->sigpending.sig[i] = current->signal->sigpending.sig[i];
    signal->blocked.sig[i] = current->signal->blocked.sig[i];
  }
  child->signal = signal;
  child->flags = current->flags;
  
  child->mm = (struct mm_struct *)alloc_page();
  struct vm_area_struct *mmap_parent = current->mm->mmap, *mmap_child = NULL;
  while (mmap_parent) {
    mmap_child = (struct vm_area_struct *)alloc_page();
    mmap_child->vm_mm = child->mm;
    mmap_child->vm_start = mmap_parent->vm_start;
    mmap_child->vm_end = mmap_parent->vm_end;
    mmap_child->vm_pgoff = mmap_parent->vm_pgoff;
    mmap_child->vm_filesz = mmap_parent->vm_filesz;
    mmap_child->vm_flags = mmap_parent->vm_flags;
    mmap_child->vm_prev = NULL;
    mmap_child->vm_next = child->mm->mmap;
    if (mmap_child->vm_next) {
      mmap_child->vm_next->vm_prev = mmap_child;
    }
    child->mm->mmap = mmap_child;
    mmap_parent = mmap_parent->vm_next;
  }
  
  pagetable_t pgtbl_parent = (pagetable_t)((((uint64_t)current->pgd & 0xfffffffffff) << 12) + PA2VA_OFFSET);
  mmap_parent = current->mm->mmap;
  child->pgd = (pagetable_t)alloc_page();
  memcpy((void *)child->pgd, (void *)swapper_pg_dir, PGSIZE);

  while (mmap_parent) {
    for (uint64_t va = (uint64_t)mmap_parent->vm_start; va < (uint64_t)mmap_parent->vm_end; va += PGSIZE) {
      uint64_t* pte_addr = walk_page_table(pgtbl_parent, va);
      if (pte_addr) {
        uint64_t pte = *pte_addr, perm;
        void *ppn_pa = (void *)(pte >> 10 << 12), *ppn_va = (void *)PA2VA(ppn_pa);
        ref_page(ppn_va);
        if (pte & PTE_W) {
          perm = (pte & 0x3ff & ~PTE_W) | PTE_S;
          vm_create_mapping(pgtbl_parent, (void *)va, ppn_pa, PGSIZE, perm);
        } else {
          perm = pte & 0x3ff;
        }
        vm_create_mapping(child->pgd, (void *)va, ppn_pa, PGSIZE, perm);
      }
    }
    mmap_parent = mmap_parent->vm_next;
  }

  child->mm->start_brk = current->mm->start_brk;
  child->mm->brk = current->mm->brk;

  child->files = file_init();
  for (int i = 0; i < MAX_FILE_NUMBER; i++) {
    child->files->fd_array[i] = current->files->fd_array[i];
  }

  asm volatile("sfence.vma" ::: "memory");
  child->pgd = (pagetable_t)((VA2PA(child->pgd) >> 12) | (8ull << 60));
  task[pid] = child;

  return pid;
}

struct task_struct *find_task_by_pid(pid_t pid) {
  if (pid <= 0 || pid > (pid_t)task_num) {
      return NULL;
  }
  return task[pid];
}

struct rt_sigframe *get_sigframe(struct pt_regs *regs) {
  unsigned long sp = regs->x[2];
  // sp &= ~0xfUL;
  pagetable_t pgtbl = (pagetable_t)((((uint64_t)current->pgd & 0xfffffffffff) << 12) + PA2VA_OFFSET);
  uint64_t *pte_addr = walk_page_table(pgtbl, sp), pte = *pte_addr;
  void *ppn_pa = (void *)(pte >> 10 << 12), *ppn_va = (void *)PA2VA(ppn_pa);
  return (struct rt_sigframe *)ppn_va;
}