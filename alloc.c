#include "alloc.h"

typedef struct block block;
struct block {
    size_t size;
    block * next;
};

// Metadata
static void * _heap_start;
static size_t _heap_size;
static block * _free_block_list;

void mem_init() {
    _heap_start = sbrk(INIT_HEAP_SIZE);
    _heap_size = INIT_HEAP_SIZE;
    _free_block_list = _heap_start;
    // Init free block list
    _free_block_list->size = INIT_HEAP_SIZE - sizeof(size_t);
    _free_block_list->next = NULL;
}

static void * mem_alloc_block(size_t size) {
    size_t aligned_size = size % ALIGN_SIZE ? ((size / ALIGN_SIZE) + 1) * ALIGN_SIZE : size;
    block *available = NULL;

    block **blk_ref = &_free_block_list;
    while (1) {
        if ((*blk_ref)->size - aligned_size >= sizeof(block)) {
            available = *blk_ref;
            block *new = (block*)((char*)(*blk_ref) + sizeof(size_t) + aligned_size);
            new->size = (*blk_ref)->size - aligned_size - sizeof(size_t);
            new->next = (*blk_ref)->next;
            *blk_ref = new;
            available->size = aligned_size;
            break;
        }
        else {
            if ((*blk_ref)->next) {
                blk_ref = &(*blk_ref)->next;
            }
            else {
                _heap_size += aligned_size * 2 + sizeof(block);
                sbrk(aligned_size * 2 + sizeof(block));
                (*blk_ref)->size += aligned_size * 2 + sizeof(block);
            }
        }
    }

    return (void*)((char*)available + sizeof(size_t));
}

static void mem_free_block(void * ref) {
    block *prev = NULL;
    block *next = NULL;
    block *ref_blk = (block*)((char*)ref - sizeof(size_t));
    block *p = _free_block_list;
    while (1) {
        if (p < ref_blk) {
            prev = p;
            p = p->next;
        }
        else {
            next = p;
            break;
        }
    }

    // Merge into previous free block or next block if possible
    if (prev && (char*)prev + sizeof(size_t) + prev->size == (char*)ref_blk) {
        prev->size += ref_blk->size + sizeof(size_t);
    }
    else if (next && (char*)ref_blk +sizeof(size_t) + ref_blk->size == (char*)next) {
        ref_blk->size += next->size + sizeof(size_t);
        ref_blk->next = next->next;
        if (prev)
            prev->next = ref_blk;
        else
            _free_block_list = ref_blk;
    }
    else {
        ref_blk->next = next;
        if (prev)
            prev->next = ref_blk;
        else
            _free_block_list = ref_blk;
    }
}

void * mem_alloc(size_t size, bool reserved) {
    if (reserved) size *= 2;
    return mem_alloc_block(size);
}

void mem_free(void * mem) {
    mem_free_block(mem);
}

void * mem_realloc(void * mem, size_t size) {
    void * new = mem;
    block * blk = (block*)((char*)mem - sizeof(size_t));
    if (blk->size < size) {
        new = mem_alloc_block(size);
        memcpy(new, mem, blk->size);
        mem_free_block(mem);
    }
    return new;
}
