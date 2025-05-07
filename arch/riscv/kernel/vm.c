#include <vm.h>
#include <string.h>
#include <mm.h>
#include <private_kdefs.h>
#include <printk.h>

extern uint8_t _stext[], _stext_end[], _srodata[], _sdata[], _sbss[];

// 用于 setup_vm 进行 1 GiB 的映射
uint64_t early_pgtbl[PGSIZE / 8] __attribute__((__aligned__(PGSIZE)));
// kernel page table 根目录，在 setup_vm_final 进行映射
uint64_t swapper_pg_dir[PGSIZE / 8] __attribute__((__aligned__(PGSIZE)));

void setup_vm(void) {
  memset(early_pgtbl, 0, PGSIZE);

  // 1. 初始化阶段，页大小为 1 GiB，不使用多级页表
  // 2. 将 va 的 64 bit 作如下划分：| 63...39 | 38...30 | 29...0 |
  //    - 63...39 bit 忽略
  //    - 38...30 bit 作为 early_pgtbl 的索引
  //    - 29...0 bit 作为页内偏移，注意到 30 = 9 + 9 + 12，即我们此处只使用根页表，根页表的每个 entry 对应 1 GiB 的页
  // 3. Page Table Entry 的权限为 X W R V

  early_pgtbl[VPN2(PHY_START)] = ((PHY_START >> 30) << 28)  | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
  early_pgtbl[VPN2(VM_START)] = ((PHY_START >> 30) << 28)  | PTE_V | PTE_R | PTE_W | PTE_X | PTE_A | PTE_D;
}

void setup_vm_final(void) {
  memset(swapper_pg_dir, 0, PGSIZE);

  // No OpenSBI mapping required

  // 1. 调用 create_mapping 映射页表
  //    - kernel code: X R
  //    - kernel rodata: R
  //    - other memory: W R
  // 2. 设置 satp，将 swapper_pg_dir 作为内核页表

  create_mapping(swapper_pg_dir, _stext, (void *)((uint64_t)_stext - PA2VA_OFFSET), (uint64_t)_srodata - (uint64_t)_stext,  PTE_R | PTE_X);
  create_mapping(swapper_pg_dir, _srodata, (void *)((uint64_t)_srodata - PA2VA_OFFSET), (uint64_t)_sdata - (uint64_t)_srodata,  PTE_R);
  create_mapping(swapper_pg_dir, _sdata, (void *)((uint64_t)_sdata - PA2VA_OFFSET), PHY_SIZE - ((uint64_t)_sdata - (uint64_t)_stext),  PTE_R | PTE_W);
  
  uint64_t satp = (((uint64_t)swapper_pg_dir - PA2VA_OFFSET) >> 12) | (8ull << 60);
  asm volatile("csrw satp, %0" : : "r"(satp) : "memory");
  // flush TLB
  asm volatile("sfence.vma" ::: "memory");
  return;
}

void create_mapping(uint64_t pgtbl[static PGSIZE / 8], void *va, void *pa, uint64_t sz, uint64_t perm) {
  // TODO：根据 RISC-V Sv39 的要求，创建多级页表映射关系
  //
  // 物理内存需要分页
  // 创建多级页表的时候使用 alloc_page 来获取新的一页作为页表
  // 注意通过 V bit 来判断表项是否存在
  //
  // 重要：阅读手册，注意 A / D 位的设置

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

    uint64_t *pgtbl1 = (uint64_t *)(pgtbl[vpn2] >> 10 << 12);

    if (!(pgtbl1[vpn1] & PTE_V)) {
      uint64_t newpage = VA2PA(alloc_page());
      pgtbl1[vpn1] = (newpage >> 12 << 10) | PTE_V;
    }

    uint64_t *pgtbl0 = (uint64_t *)(pgtbl1[vpn1] >> 10 << 12);
    pgtbl0[vpn0] = (PA >> 12 << 10) | perm | PTE_V | PTE_A | PTE_D;

    VA += PGSIZE;
    PA += PGSIZE;
  }
  
  printk("pgtbl = %p: map [%p, %p) -> [%p, %p), perm = 0x%lx, size = %lu\n", 
         (void *)VA2PA(pgtbl), va, (void *)((uint64_t)va + sz), pa, (void *)((uint64_t)pa + sz), perm, sz);
  return;
}

void vm_create_mapping(uint64_t pgtbl[static PGSIZE / 8], void *va, void *pa, uint64_t sz, uint64_t perm) {

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
