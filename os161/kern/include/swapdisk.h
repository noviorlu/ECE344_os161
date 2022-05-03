#ifndef _SWAPDISK_H_
#define _SWAPDISK_H_

#include <lib.h>
#include <vnode.h>
#include <bitmap.h>
#include <synch.h>


struct vnode* swapdisk_v;
int swapdisk_total;
struct bitmap* swapdisk_bitmap;
struct semaphore* swapdisk_mutex;

typedef enum{
    NOTONDISK,
    EXECVFILE,
    SWAPDISK
}DISKSTATUS;


void swapdisk_boostrap();
int store_to_swapdisk(paddr_t paddr);
void read_from_swapdisk(paddr_t paddr, int offset);

#endif /* _SWAPDISK_H_ */
