#include <stdio.h>
#include <stdlib.h>

// 从流中读取一个字符
static int file_getc(FILE *f) {
    // 优先从回退缓冲区读取
    if (f->unget_count > 0) {
        return f->unget_buf[--f->unget_count];
    }
    
    unsigned char c;
    // 使用函数指针进行读取
    if (f->read(f, &c, 1) == 1) {
        return c;
    }
    return EOF;
}

// 回退字符到流中
static void file_ungetc(FILE *f, int c) {
    if (c == EOF || f->unget_count >= UNGET_BUFSIZE) return;
    f->unget_buf[f->unget_count++] = (unsigned char)c;
}

int isspace(int c) {
    // 检查字符是否为空白字符
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

int isdigit(int c) {
    // 检查字符是否为数字字符
    return (c >= '0' && c <= '9');
}

int strtol(const char *nptr, char **endptr, int base) {
    // 简单的字符串转整数实现
    const char *p = nptr;
    long result = 0;
    int sign = 1;

    // 跳过空白字符
    while (isspace((unsigned char)*p)) p++;

    // 处理符号
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    // 处理数字
    while (isdigit((unsigned char)*p)) {
        result = result * base + (*p - '0');
        p++;
    }

    if (endptr) *endptr = (char *)p;
    return sign * result;
}

// 跳过空白字符
static void skip_whitespace(FILE *f) {
    int c;
    while ((c = file_getc(f)) != EOF) {
        if (!isspace(c)) {
            file_ungetc(f, c);
            return;
        }
    }
}

// 核心vfscanf实现
int vfscanf(FILE *f, const char *fmt, va_list ap) {
    int count = 0;  // 成功匹配的项目数
    
    while (*fmt) {
        // 处理格式字符串中的空白
        if (isspace(*fmt)) {
            skip_whitespace(f);
            fmt++;
            continue;
        }
        
        // 处理普通字符
        if (*fmt != '%') {
            int c = file_getc(f);
            if (c != *fmt) {
                if (c != EOF) file_ungetc(f, c);
                return count;  // 匹配失败
            }
            fmt++;
            continue;
        }
        
        // 处理格式说明符
        fmt++;  // 跳过'%'
        int width = 0;
        int suppress = 0;
        
        // 解析格式修饰符
        while (*fmt) {
            if (*fmt == '*') {
                suppress = 1;
                fmt++;
            } else if (isdigit(*fmt)) {
                width = strtol(fmt, (char**)&fmt, 10);
            } else {
                break;
            }
        }
        
        switch (*fmt++) {
            case 'd': {  // 整数
                int *ptr = suppress ? NULL : va_arg(ap, int*);
                int sign = 1, value = 0, c;
                
                skip_whitespace(f);
                c = file_getc(f);
                
                // 处理符号
                if (c == '-') {
                    sign = -1;
                    c = file_getc(f);
                } else if (c == '+') {
                    c = file_getc(f);
                }
                
                // 检查数字
                if (!isdigit(c)) {
                    if (c != EOF) file_ungetc(f, c);
                    return count;
                }
                
                // 读取数字
                int digits = 0;
                while (isdigit(c) && (width == 0 || digits < width)) {
                    value = value * 10 + (c - '0');
                    c = file_getc(f);
                    digits++;
                }
                
                if (c != EOF) file_ungetc(f, c);
                
                // 存储结果
                if (!suppress && ptr) {
                    *ptr = value * sign;
                    count++;
                }
                break;
            }
            
            case 's': {  // 字符串
                char *str = suppress ? NULL : va_arg(ap, char*);
                int c;
                
                skip_whitespace(f);
                c = file_getc(f);
                
                // 读取非空白字符
                int chars = 0;
                while (!isspace(c) && c != EOF && (width == 0 || chars < width)) {
                    if (!suppress && str) {
                        *str++ = c;
                    }
                    c = file_getc(f);
                    chars++;
                }
                
                if (c != EOF) file_ungetc(f, c);
                
                // 终止字符串
                if (!suppress && str) {
                    *str = '\0';
                    count++;
                }
                break;
            }
            
            case 'c': {  // 字符
                char *ch = suppress ? NULL : va_arg(ap, char*);
                int c = file_getc(f);
                
                if (c == EOF) return count;
                
                // 处理宽度
                int chars = 1;
                if (width > 1) {
                    if (!suppress && ch) {
                        *ch++ = c;
                    }
                    
                    while (chars < width && (c = file_getc(f)) != EOF) {
                        if (!suppress && ch) {
                            *ch++ = c;
                        }
                        chars++;
                    }
                } else if (!suppress && ch) {
                    *ch = c;
                }
                
                if (!suppress) count++;
                break;
            }
            
            // 其他格式符可在此扩展
            default:
                // 遇到不支持的格式符直接返回
                return count;
        }
    }
    
    return count;
}
