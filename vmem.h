#pragma once

#include "mmu.h"
#include "defs.h"


/* Virtual addrses mapping */
typedef struct VMap {
    void *        virt;
    physical_addr pstart; 
    physical_addr pend;
    int           perm;
} VMap;



void allocate_kernel_vmem();
PDE *setup_kernel_vmem();
void switch_kernel_vmem();
int  deallocate_user_vmem(PDE *page_dir, uint32_t oldsz, uint32_t newsz);
void init_user_vmem(PDE *page_dir, char *init, unsigned int sz);
void free_vmem(PDE *page_dir);
