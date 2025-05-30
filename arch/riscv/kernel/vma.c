#include <vma.h>
#include <private_kdefs.h>
#include <mm.h>
#include <vm.h>
#include <string.h>

void vm_remove_mapping(pagetable_t pgtbl, unsigned long addr);
void unmap_page(pagetable_t pgtbl, unsigned long start, unsigned long end) {
    for (unsigned long va = start; va < end; va += PGSIZE) {
        uint64_t *pte_addr = walk_page_table(pgtbl, va);
        if (pte_addr) {
            uint64_t pte = *pte_addr;
            void *ppn_pa = (void *)(pte >> 10 << 12), *ppn_va = (void *)PA2VA(ppn_pa);
            if (deref_page(ppn_va) != -1 && (pte & PTE_S)) {
                void *new_page = alloc_page();
                memcpy(new_page, ppn_va, PGSIZE);
                vm_create_mapping(pgtbl, (void *)va, (void *)VA2PA(new_page), PGSIZE, 0);
                pte_addr = walk_page_table(pgtbl, va);
            }
            *pte_addr = 0;
        }
    }
    asm volatile("sfence.vma" ::: "memory");
}
void do_vma_munmap(pagetable_t pgtbl, struct vm_area_struct *vma, unsigned long start, unsigned long end) {
    vma->vm_end = (void *)end;
    unmap_page(pgtbl, start, end);
}

void do_vma_mmap(struct vm_area_struct *vma, unsigned long start, unsigned long end) {
    vma->vm_end = (void *)end;
    // do_mmap(vma->vm_mm, (void *)start, end - start, vma->vm_flags);
}