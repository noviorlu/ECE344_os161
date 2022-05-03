#include <coremap.h>
#include <vm.h>

#include <kern/errno.h>


paddr_t
PPAGE_TO_PADDR(ppage page){
    return core_endpaddr + (page << 12);
}

ppage
PADDR_TO_PPAGE(paddr_t paddr){
    return (paddr - core_endpaddr) >> 12;
}


// 2B 000 ~ 7D 000 82 Usable page
// 2B 000 totally for COREMAP
// 2C 000 ~ 7D 000 82 COREMAP page
void
coremap_preboostrap(){
    // Initalize evict clock arm
    core_clock = 0;

    // Initalize coremap kvaddr position
    paddr_t ram_endpaddr;
    ram_getsize(&core_startpaddr, &ram_endpaddr);
    coremap = (struct coremap_entry*)PADDR_TO_KVADDR(core_startpaddr);

    // Get total Page number without allocating cormap it self
    int totalPageNum = (ram_endpaddr - core_startpaddr) / PAGE_SIZE;

    // Get the coremap ending position (normally coremap will take a full page)
    // Might be utilize using kmalloc_subpage system or some trick?
    core_total = totalPageNum - 1;
    core_endpaddr = core_startpaddr + PAGE_SIZE;

    assert(core_startpaddr != core_endpaddr);

    // Initalize each coremap entry, core_total = totalPageNum - 1
    int i;
    for (i = 0; i < core_total; i++){
        coremap[i].page_stat = FREE;
        coremap[i].entry_ptr = 0;
        coremap[i].vaddr = 0;
        coremap[i].shared = 0;
        coremap[i].modify = 0;
    }
}

void coremap_postboostrap(){
    core_mutex = sem_create("core_mutex", 1);
}

ppage
alloc_ppage(unsigned long npages){
    assert(npages == 1);
    
    // used for bootstrap, sync unusable
    if(curspl != SPL_HIGH)
        P(core_mutex);

    int tick = core_clock;
    do{
        if(coremap[tick].page_stat == FREE){
            core_clock = tick;
            
            if(curspl != SPL_HIGH)
                V(core_mutex);
                
            return tick;
        }
        tick = (tick + 1) % core_total;
    }while(tick != core_clock);

    // SWAP IMPLEMENTATION!!!
    if(curspl != SPL_HIGH)
        V(core_mutex);
    return -ENOMEM;

    // NO FREE PAGE DISCOVERED, NEED SWAP_TO_DISK
    // MIGHT UTILIZE USING REFERENCE BIT
    do{
        tick = (tick + 1) % core_total;
        
        if(coremap[tick].page_stat == KERNEL)
            continue;

        if(coremap[tick].page_stat == USER){
            // off_t swapdisk_offset = store_to_swapdisk(PPAGE_TO_PADDR(tick));
            
            // UPDATE PTENTRY INFO to mention that is on disk with OFFSET swapdisk_offset
            // coremap[tick].entry_ptr->paddr = swapdisk_offset | PT_SWAP;

            free_ppage(PPAGE_TO_PADDR(tick));

            core_clock = tick;

            V(core_mutex);
            return tick;
        }
            
        tick = (tick+1) % core_total;

    }while(tick != core_clock);

    panic("COREMAP ALL KERNEL PAGE!!!\n");
    return -1;
}

void
free_ppage(paddr_t paddr){
    P(core_mutex);
    
    ppage page = PADDR_TO_PPAGE(paddr);
    assert((int)page < core_total);

    // DEBUG(DB_VM,"FREEING COREMAP idx: %d\n", page);

    coremap[page].page_stat = FREE;
    coremap[page].vaddr = 0;
    
    // PTE already kfreed in as_destory if necessary, then free_ppage is called
    coremap[page].entry_ptr = 0;
    // free Multiple page if entry_ptr are same
    // While next vaddr is same, free page
    
    coremap[page].shared = 0;
    coremap[page].modify = 0;

    V(core_mutex);
}

void saveCoremapInfo(
    ppage idx, 
    CORESTATUS page_stat, 
    vaddr_t vaddr, 
    PTE* entry_ptr, 
    int shared, 
    int modify)
{
    if(curspl != SPL_HIGH)
        P(core_mutex);

    coremap[idx].page_stat = page_stat;
    coremap[idx].vaddr = vaddr;
    coremap[idx].entry_ptr = entry_ptr;
    coremap[idx].shared = shared;
    coremap[idx].modify = modify;
    
    if(curspl != SPL_HIGH)
        V(core_mutex);
}
