#include <stdint.h>
#include <printk.h>
#include <proc.h>
#include <ksyscalls.h>
#include <private_kdefs.h>
#include <mm.h>
#include <vm.h>
#include <string.h>

extern struct task_struct *current;
extern uint8_t _suapp[];

void clock_set_next_event(void);

static void do_page_fault(uint64_t scause, uint64_t stval) {
  struct vm_area_struct *vma = find_vma(current->mm, (void *)stval);
  if (vma == NULL) {
    printk("Page fault: no vma found for address %p\n", (void *)stval);
    return;
  }
  if (scause == 12 && !(vma->vm_flags & VM_EXEC)) {
    printk("Page fault: execute access to non-executable page at address %p\n", (void *)stval);
    return;
  } else if (scause == 13 && !(vma->vm_flags & VM_READ)) {
    printk("Page fault: read access to non-readable page at address %p\n", (void *)stval);
    return;
  } else if (scause == 15 && !(vma->vm_flags & VM_WRITE)) {
    printk("Page fault: write access to non-writable page at address %p\n", (void *)stval);
    return;
  }
  pagetable_t pgtbl = (pagetable_t)((((uint64_t)current->pgd & 0xfffffffffff) << 12) + PA2VA_OFFSET);
  uint64_t *pte_addr = walk_page_table(pgtbl, stval);
  printk("vma = %p, ", vma);
  if (pte_addr) {
    uint64_t pte = *pte_addr;
    void *ppn_pa = (void *)(pte >> 10 << 12), *ppn_va = (void *)PA2VA(ppn_pa);
    if (pte & PTE_S) {
      if (deref_page(ppn_va) == -1) {
        void *aligned_addr = (void *)PGROUNDDOWN(stval);
        vm_create_mapping(pgtbl, aligned_addr, ppn_pa, PGSIZE, (pte & 0x3ff & ~PTE_S) | PTE_W);
      } else {
        void *new_page = alloc_page(), *aligned_addr = (void *)PGROUNDDOWN(stval);
        printk("SHARED PAGE [PID = %ld], copy %p to %p\n", current->pid, (void *)VA2PA(ppn_va), (void *)VA2PA(new_page));
        memcpy(new_page, ppn_va, PGSIZE);
        vm_create_mapping(pgtbl, aligned_addr, (void *)VA2PA(new_page), PGSIZE, (vma->vm_flags << 1) | PTE_U);
        ref_page(new_page);
      }
    }
  } else {
    void *new_page = alloc_page(), *aligned_addr = (void *)PGROUNDDOWN(stval);
    vm_create_mapping(pgtbl, aligned_addr, (void *)VA2PA(new_page), PGSIZE, (vma->vm_flags << 1) | PTE_U);
    if (!(vma->vm_flags & VM_ANON)) {
      memcpy(aligned_addr, (void *)((uint64_t)_suapp + aligned_addr), PGSIZE);
    }
    ref_page(new_page);
  }
}

unsigned long read_thread_flags(void) {
  return current->flags;
}

void clear_thread_flag(unsigned long flag) {
  current->flags &= ~flag;
}

void recalc_sigpending(void) {
  for (int i = 0; i < _NSIG_WORDS; i++) {
    if (current->signal->sigpending.sig[i]) {
      return;
    }
  }
  clear_thread_flag(_TIF_SIGPENDING);
}

int get_signal(struct ksignal *ksig) {
  struct signal_struct *signal = current->signal;
  for (int i = 1; i < _NSIG; i++) {
    if (signal->sigpending.sig[i / __BITS_PER_LONG] & sigmask(i) &&
        !(signal->blocked.sig[i / __BITS_PER_LONG] & sigmask(i))) {
      ksig->sig = i;
      ksig->ka = current->sighand->action[i - 1];
      // 清除信号
      sigdelsetmask(&signal->sigpending, sigmask(i));
      recalc_sigpending();
      return 1;
    }
  }
  
  return 0;
}

void setup_rt_frame(struct ksignal *ksig, struct pt_regs *regs) {
  struct rt_sigframe *frame = get_sigframe(regs);
  for (int i = 0; i < 32; i++) {
    frame->user_regs.x[i] = regs->x[i];
  }
  frame->user_regs.sepc = regs->sepc;
  regs->sepc = (unsigned long)ksig->ka.sa_handler;
  regs->x[1] = (unsigned long)ksig->ka.sa_restorer;
	regs->x[2] = regs->x[2] - sizeof(struct rt_sigframe);
	regs->x[10] = ksig->sig;
}

void do_signal(struct pt_regs *regs) {
  struct ksignal ksig;
  // struct sighand_struct *sighand = current->sighand;
  if (get_signal(&ksig)) {
    // handle_signal(&ksig, regs);
    setup_rt_frame(&ksig, regs);
  }
}

void syscall_exit_to_user_mode(struct pt_regs *regs) {
  unsigned long ti_work = read_thread_flags();
  if (ti_work & _TIF_SIGPENDING) {
    do_signal(regs);
  }
}

void trap_handler(struct pt_regs *regs, uint64_t scause, uint64_t stval) {
  // 根据 scause 判断 trap 类型
  // 如果是 Supervisor Timer Interrupt：
  // - 打印输出相关信息
  // - 调用 clock_set_next_event 设置下一次时钟中断
  // 其他类型的 trap 可以直接忽略，推荐打印出来供以后调试
  if ((scause >> 63) && ((scause & 0x7fffffffffffffff) == 5)) {
    // printk("[S] Supervisor Mode Timer Interrupt\n");
    clock_set_next_event();
    do_timer();
  }
  else if (scause == 8) {
    syscall_handler(regs);
    regs->sepc += 4;
    syscall_exit_to_user_mode(regs);
  }
  else if (scause == 12 || scause == 13 || scause == 15) {
    char *type = (scause == 12) ? "Instruction" : (scause == 13) ? "Load" : "Store/AMO";
    printk("[S] %s Page Fault: sepc = %p, stval = %p\n", type, (void *)regs->sepc, (void *)stval);
    do_page_fault(scause, stval);
  }
  else {
    printk("[S] Unknown trap: scause = %p, sepc = %p\n, stval = %p\n", (void *)scause, (void *)regs->sepc, (void *)stval);
  }
}
