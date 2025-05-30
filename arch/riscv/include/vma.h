#ifndef __VMA_H__
#define __VMA_H__

#include <proc.h>

void do_vma_munmap(pagetable_t pgtbl, struct vm_area_struct *vma, unsigned long start, unsigned long end);
void do_vma_mmap(struct vm_area_struct *vma, unsigned long start, unsigned long end);

#endif