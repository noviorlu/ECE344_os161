#include <coremap.h>
#include <curthread.h>
#include <vfs.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <uio.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <thread.h>

#include <swapdisk.h>

void
swapdisk_boostrap(){

    // convert (const char*) -> (char*)
	char* swapdisk_name = kstrdup("lhd0raw:");
    int err = vfs_open(swapdisk_name, O_RDWR, &swapdisk_v);
    if(err){
        panic("SWAP DISK setup FAILED\n");
    }

    struct stat swapdisk_stat;
    VOP_STAT(swapdisk_v, &swapdisk_stat);

    swapdisk_total = swapdisk_stat.st_size / PAGE_SIZE;
    swapdisk_bitmap = bitmap_create(swapdisk_total);

    kfree(swapdisk_name);

    swapdisk_mutex = sem_create("swapdisk_mutex", 1);
}

off_t 
store_to_swapdisk(paddr_t paddr){
    (void)paddr;
     int diskIdx;
    bitmap_alloc(swapdisk_bitmap, &diskIdx);
    // catch diskIdx not exist swapdisk full case

    off_t diskoff = diskIdx * PAGE_SIZE;
    (void)diskoff;
    return diskoff;
}

void 
read_from_swapdisk(paddr_t paddr, int offset){
    (void)paddr;
    (void)offset;
}
