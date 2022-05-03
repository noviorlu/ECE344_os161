#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <pagetable.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	as->as_vbase1 = 0;
	as->as_npages1 = 0;

	as->as_vbase2 = 0;
	as->as_npages2 = 0;

	as->as_heapvbase = 0;
	as->as_heapvtop = 0;

	as->execvfile = NULL;

	int i;
	for(i=0; i<PTENTRY; i++){
		as->pagetable[i] = 0;
	}
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
	// REMEMBER TO UPDATE TLB TO READONLY!!!!!!
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_off1 = old->as_off1;
	new->as_filesz1 = old->as_filesz1;


	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	new->as_off2 = old->as_off2;
	new->as_filesz2 = old->as_filesz2;

	new->as_heapvbase = old->as_heapvbase;
	new->as_heapvtop = old->as_heapvtop;

	new->execvfile = old->execvfile;

	new->as_stackbottom = old->as_stackbottom;

	cpy_PT(old->pagetable, new->pagetable);

	*ret = new;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	
	free_PT(as->pagetable);
	vfs_close(as->execvfile);
	
	kfree(as);

}

int lastPID = -1;

void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	(void)as;  // suppress warning until code gets written
	tlb_disable_all();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, size_t fz, off_t offset,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;


	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		as->as_off1 = offset;
		as->as_filesz1 = fz;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;		
		as->as_off2 = offset;
		as->as_filesz2 = fz;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

// noneed?
int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	(void)as;
	return 0;
}

// noneed?
int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	(void)as;
	return 0;
}

// Define stack and heap
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	as->as_stackbottom = USERSTACK - (PAGE_SIZE * STACK_V_SIZE);
	as->as_heapvtop = as->as_vbase2 + PAGE_SIZE;
	as->as_heapvbase = as->as_heapvtop;
	return 0;
}
