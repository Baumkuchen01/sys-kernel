#include <stdlib.h>
#include <unistd.h>

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};

typedef union header header_t;

header_t *head, *tail;

header_t *get_free_block(size_t size) {
    header_t *current = head;
    while (current) {
        if (current->s.is_free && current->s.size >= size) {
            return current;
        }
        current = current->s.next;
    }
    return NULL;
}

void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    header_t *header = get_free_block(size);
    if (header) {
        header->s.is_free = 0;
        return (void *)(header + 1);
    }

    size_t total_size = sizeof(header_t) + size;
    header_t *new_block = (header_t *)sbrk(total_size);
    if (new_block == (void *)-1) {
        return NULL;
    }
    new_block->s.size = size;
    new_block->s.is_free = 0;
    new_block->s.next = NULL;
    if (head == NULL) {
        head = new_block;
    } else {
        tail->s.next = new_block;
    }
    tail = new_block;
    
    return (void *)(new_block + 1);
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    header_t *header = (header_t *)ptr - 1;
    void *programbreak = sbrk(0);
    if (ptr + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            header_t *current = head;
            while (current->s.next != tail) {
                current = current->s.next;
            }
            current->s.next = NULL;
            tail = current;
        }
        sbrk(-((long)header->s.size + sizeof(header_t)));
    } else {
        header->s.is_free = 1;
    }
    return;
}