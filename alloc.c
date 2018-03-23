#include "alloc.h"

// Define chunk and block structure
typedef struct {
    unsigned char data[CHUNK_SIZE];
} chunk;

typedef struct {
    chunk flag_chunk;
    chunk data_chunk[CHUNK_SIZE];
} chunk_group;

typedef struct block block;
struct block {
    size_t size;
    block *next;
};

// Metadata
static void *_heap_start;
static void *_block_area_start;
static chunk_group *_chunk_area_start;
static size_t _chunk_amount;
static size_t _free_chunk_amount;
static size_t _chunk_area_size;
static size_t _block_area_size;
static int _chunk_group_amount;
static int _next_chunk_idx;
static int _next_chunk_grp_idx;
static block *_free_block_list;

void mem_init() {
    _chunk_group_amount = DEFAULT_CHUNK_GROUPS;
    _chunk_amount = CHUNK_SIZE * DEFAULT_CHUNK_GROUPS;
    _free_chunk_amount = _chunk_amount;
    _chunk_area_size = sizeof(chunk_group) * DEFAULT_CHUNK_GROUPS;
    _block_area_size = DEFAULT_BLOCK_AREA_SIZE;
    _heap_start = sbrk(_chunk_area_size + _block_area_size);
    _chunk_area_start = (chunk_group*)_heap_start;
    _block_area_start = (char*)_heap_start + _chunk_area_size;

    _next_chunk_idx = 0;
    _next_chunk_grp_idx = 0;

    _free_block_list = _block_area_start;
    // Init free block list
    _free_block_list->size = _block_area_size - sizeof(size_t);
    _free_block_list->next = NULL;

    // Init indexed chunk
    for (int i = 0; i < DEFAULT_CHUNK_GROUPS; i++) {
        memset(_chunk_area_start + i, 0, sizeof(chunk_group));
    }
}

static void *mem_alloc_chunk() {
    chunk *available = NULL;

    for (int i = _next_chunk_grp_idx; i != _next_chunk_grp_idx; i = (i + 1) % _chunk_group_amount) {
        chunk_group *group = _chunk_area_start + i;
        chunk *flag_chunk = &group->flag_chunk;
        for (int j = _next_chunk_idx; j != _next_chunk_idx; j = (j + 1) % CHUNK_SIZE) {
            if (flag_chunk->data[j] == '0') {
                flag_chunk->data[j] = '1';
                available = group->data_chunk + j;
                _next_chunk_idx = j + 1;
                _next_chunk_grp_idx = i;
                goto final;
            }
        }
    }

    final:
    _free_chunk_amount--;
    return (void *)available;
}

static void mem_free_chunk(void *ref) {
    size_t idx = (chunk*)ref - (chunk*)_chunk_area_start;
    int grp_idx = idx / (CHUNK_SIZE + 1);
    int act_idx = idx % (CHUNK_SIZE + 1) - 1;

    chunk_group *grp = _chunk_area_start + grp_idx;
    grp->flag_chunk.data[act_idx] = '0';
    _free_chunk_amount++;
}

static void *mem_alloc_block(size_t size) {
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
                sbrk(aligned_size * 2 + sizeof(block));
                (*blk_ref)->size += aligned_size * 2 + sizeof(block);
            }
        }
    }

    return (void*)((char*)available + sizeof(size_t));
}

static void mem_free_block(void *ref) {
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

void *mem_alloc(size_t size, bool reserved) {
    void *mem = NULL;
    if (reserved) size *= 2;

    if (size <= CHUNK_SIZE && _free_chunk_amount > 0) {
        mem = mem_alloc_chunk();
    }
    else {
        mem = mem_alloc_block(size);
    }
    return mem;
}

void mem_free(void *mem) {
    if (mem < _block_area_start) {
        mem_free_chunk(mem);
    }
    else {
        mem_free_block(mem);
    }
}

void *mem_realloc(void *mem, size_t size) {
    void *new = mem;

    if (mem < _block_area_start) {
        if (size > CHUNK_SIZE) {
            new = mem_alloc_block(size);
            memcpy(new, mem, CHUNK_SIZE);
            mem_free_chunk(mem);
        }
    }
    else {
        block *blk = (block*)((char*)mem - sizeof(size_t));
        if (blk->size < size) {
            new = mem_alloc_block(size);
            memcpy(new, mem, blk->size);
            mem_free_block(mem);
        }
    }

    return new;
}
