#include <stdio.h>
#include <printk.h>
#include <sbi.h> 
#include <private_kdefs.h>

#define VA2PA(x) ((uint64_t)(x) - PA2VA_OFFSET)
#define PA2VA(x) ((uint64_t)(x) + PA2VA_OFFSET)

static size_t printk_sbi_write(FILE *restrict fp, const void *restrict buf, size_t len) {
  (void)fp;

  // 调用 SBI 接口输出 buf 中长度为 len 的内容
  // 返回实际输出的字节数
  // Hint：阅读 SBI v2.0 规范！
  uint64_t base_addr_lo = VA2PA(buf);
  uint64_t base_addr_hi = 0;
  struct sbiret result = sbi_ecall(0x4442434e, 0, len, base_addr_lo, base_addr_hi, 0, 0, 0);
  return result.value;
}

void printk(const char *fmt, ...) {
  FILE printk_out = {
      .write = printk_sbi_write,
  };

  va_list ap;
  va_start(ap, fmt);
  vfprintf(&printk_out, fmt, ap);
  va_end(ap);
}
