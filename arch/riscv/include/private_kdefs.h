#ifndef __PRIVATE_KDEFS_H__
#define __PRIVATE_KDEFS_H__

// QEMU virt 机器的时钟频率为 10 MHz
#define TIMECLOCK 10000000
// #define TIMECLOCK 200000

#define PHY_START 0x80000000
// #define PHY_SIZE 0x400000 // 4 MiB
#define PHY_SIZE 0x8000000 // 128 MiB

#define OPENSBI_SIZE 0x200000

#define VM_START 0xffffffe000000000
#define VM_END 0xffffffff00000000
#define VM_SIZE (VM_END - VM_START)

#define PA2VA_OFFSET (VM_START - PHY_START)

#define PHY_END (PHY_START + PHY_SIZE)

#define PGSIZE 0x1000 // 4 KiB
#define PGROUNDDOWN(addr) ((addr) & ~(PGSIZE - 1))
#define PGROUNDUP(addr) PGROUNDDOWN((addr) + PGSIZE - 1)

#define USER_START 0x0        // user space start virtual address
#define USER_END 0x4000000000 // user space end virtual address

#define TS_OFF 32

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
#define PTE_S 0x100

#define VM_READ 0x01
#define VM_WRITE 0x02
#define VM_EXEC 0x04
#define VM_ANON 0x08

#endif