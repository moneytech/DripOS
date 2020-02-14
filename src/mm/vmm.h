#ifndef VMM_H
#define VMM_H
#include <stdint.h>

#define VMM_PRESENT (1<<0)
#define VMM_WRITE (1<<1)
#define VMM_USER (1<<2)
#define VMM_WRITE_THROUGH (1<<3)
#define VMM_NO_CACHE (1<<4)
#define VMM_ACCESS (1<<5)
#define VMM_DIRTY (1<<6)
#define VMM_HUGE (1<<7)

typedef struct {
    uint64_t p4_off;
    uint64_t p3_off;
    uint64_t p2_off;
    uint64_t p1_off;
} pt_off_t;

typedef struct {
    uint64_t table[512];
} pt_t;

typedef struct {
    pt_t *p4;
    pt_t *p3;
    pt_t *p2;
    pt_t *p1;
} pt_ptr_t;

int vmm_map(void *phys, void *virt, uint64_t count, uint16_t perms);
int vmm_remap(void *phys, void *virt, uint64_t count, uint16_t perms);
int vmm_unmap(void *phys, void *virt, uint64_t count);
int vmm_map_pages(void *phys, void *virt, void *p4, uint64_t count, uint16_t perms);
int vmm_remap_pages(void *phys, void *virt, void *p4, uint64_t count, uint16_t perms);
int vmm_unmap_pages(void *phys, void *virt, void *p4, uint64_t count);
void vmm_set_pml4t(uint64_t new);
void *virt_to_phys(void *virt, pt_t *p4);
uint64_t vmm_get_pml4t();

#endif