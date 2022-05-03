#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <sfs.h>
#include <uio.h>


/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

/*
 * alloc_kpages() and free_kpages() are called by kmalloc() and thus the whole
 * kernel will not boot if these 2 functions are not completed.
 */
void
vm_postbootstrap(void)
{
	/* do nothing */
	coremap_postboostrap();
	swapdisk_boostrap();
}

void
vm_prebootstrap(void)
{
	coremap_preboostrap();
	tlb_boostrap();
}

vaddr_t 
alloc_kpages(int npages)
{
	assert(npages == 1);
	ppage idx = alloc_ppage(npages);
	if(idx < 0) return -idx;

	paddr_t paddr = PPAGE_TO_PADDR(idx);
	vaddr_t kvaddr = PADDR_TO_KVADDR(paddr);
	
	DEBUG(DB_VM, "ALLOC KPAGE: %d\n", idx);

	saveCoremapInfo(idx, KERNEL, kvaddr, 0, 0, 0);

	return kvaddr;
}

void 
free_kpages(vaddr_t addr)
{
	DEBUG(DB_VM, "FREE KPAGE: %d\n", PADDR_TO_PPAGE(KVADDR_TO_PADDR(addr)));
	free_ppage(KVADDR_TO_PADDR(addr));
}

// return 0 on success
int define_addr_region(struct addrspace* as, int faulttype, vaddr_t faultaddress, REGION* addr_region){
	*addr_region = -1;
	if( faultaddress >= as->as_stackbottom && faultaddress < USERSTACK ){
		*addr_region = STACK;
	}
	else if( faultaddress >= as->as_heapvbase && faultaddress < as->as_heapvtop ){
		*addr_region = HEAP;
	}
	else if( 
		faultaddress >= as->as_vbase1 && 
		faultaddress < (as->as_vbase1 + PAGE_SIZE * as->as_npages1) 
	){
		if(faulttype != VM_FAULT_READ){
			DEBUG(DB_VM, "TRY to write to CODE REGION!!!\n");
			return EFAULT;
		}
		*addr_region = BASE1;
	}else if(
		faultaddress >= as->as_vbase2 && 
		faultaddress < (as->as_vbase2 + PAGE_SIZE * as->as_npages2)
	){
		*addr_region = BASE2;
	}else{
		DEBUG(DB_VM,"\nAddr %x NOT IN VM SPACE\n", faultaddress);
		return EFAULT;
	}
	return 0;
}

int vm_fault_READONLY(struct addrspace* as, vaddr_t faultaddress, REGION addr_region){
	(void)as;
	(void)faultaddress;
	(void)addr_region;
	vaddr_t* last_layer_pt;

	tlb_disable_vaddr(faultaddress);

	int idx = readPTE(as->pagetable, faultaddress, &last_layer_pt);

	PTE* entry_ptr = (PTE*)last_layer_pt[idx];
	if(idx == -1) panic("READONLY: PTE_ENTRY NOT EXIST\n");
	
	if(entry_ptr->paddr & PT_SWAP){
		panic("READONLY: SWAP NOT IMPLEMENTED\n");
	}

	if(!(entry_ptr->paddr & PT_VALID)){
		panic("READONLY: SOMETHING WRONG NOT VALID\n");
	}

	// easy case, only one thread sharing
	if(entry_ptr->share == 1){
		entry_ptr->paddr |= TLBLO_DIRTY;
		tlb_insert(faultaddress, entry_ptr->paddr);
		return 0;
	}
	int oldpage = PADDR_TO_PPAGE(entry_ptr->paddr);
	int newpage = alloc_ppage(1);
	if(newpage < 0) return -newpage;

	entry_ptr->share--;

	last_layer_pt[idx] = (vaddr_t)kmalloc(sizeof(PTE));
	((PTE *)last_layer_pt[idx])->paddr = PPAGE_TO_PADDR(newpage) | PT_VALID | PT_READWRITE;
	((PTE *)last_layer_pt[idx])->share = 1;

	memcpy((vaddr_t*)PADDR_TO_KVADDR(PPAGE_TO_PADDR(newpage)), (vaddr_t*)PADDR_TO_KVADDR(PPAGE_TO_PADDR(oldpage)), PAGE_SIZE);

	// kprintf("entry_ptr (%d, %d)\n", PADDR_TO_PPAGE(((PTE *)last_layer_pt[idx])->paddr), entry_ptr->share);
	if(!TLB_Probe(faultaddress, TLBLO_INVALID()))
		tlb_insert(faultaddress, ((PTE *)last_layer_pt[idx])->paddr);
	
	
	return 0;
}

int vm_fault_READWRITE(struct addrspace* as, vaddr_t faultaddress, REGION addr_region){


	PTE* entry_ptr;
	int err = examingPT(as->pagetable, faultaddress, &entry_ptr);
	// PT/PTE not allocated, must be read from executable
	if(err == -1){
		paddr_t paddr = TLBLO_VALID;

		// enable read/write for stack heap region
		if(addr_region == STACK || addr_region == HEAP || addr_region == BASE2)
			paddr|=TLBLO_DIRTY;

		// alloc Physical page using coremap (mutex for syncProb)
		ppage idx = alloc_ppage(1);
		if(idx < 0) {
			return -idx;
		}
		paddr |= PPAGE_TO_PADDR(idx);
		
		// set PTE physical addr
		entry_ptr->paddr = paddr;
		entry_ptr->share = 1;

		// set coremap info
		saveCoremapInfo(idx, USER, faultaddress, entry_ptr, 1, 1);

		// insert vaddr-paddr to tlb
		tlb_insert(faultaddress, paddr);

		// if comes from code/data, load from execvfile
		if(addr_region == BASE1 || addr_region == BASE2){
			// DEBUG(DB_VM, "load execvfile... ...\n");
			// if(addr_region == BASE1)
			// 	coremap[idx].page_stat = USEREXEC;

			off_t offset;
			vaddr_t base;
			int fz;
			if(addr_region == BASE1){
				offset = as->as_off1;
				base = as->as_vbase1;
				fz = as->as_filesz1;
			}else{
				offset = as->as_off2;
				base = as->as_vbase2;
				fz = as->as_filesz2;
			}

			u_int32_t pgoff = (faultaddress - base) / PAGE_SIZE;
			fz = fz - pgoff * PAGE_SIZE;
			offset = offset + pgoff * PAGE_SIZE;
			if(fz < 0){
				panic("filesize Conversion Mistake!!!\n");
			}
			if(fz > PAGE_SIZE){
				fz = PAGE_SIZE;
			}
			DEBUG(DB_VM, "LOAD execvFilePTR %x -- faultAddr %x -- offset %x -- ppage %d -- memsz %x -- filesz %x\n", 
				(u_int32_t)as->execvfile,
				faultaddress, 
				offset, 
				idx, 
				PAGE_SIZE, 
				fz
			);

			assert(as->execvfile != NULL);

			struct uio u;
			mk_kuio(&u, (void*)PADDR_TO_KVADDR((PPAGE_TO_PADDR(idx))), fz, offset, UIO_READ);
			VOP_READ(as->execvfile, &u);
			// DEBUG(DB_VM, "READ SUCCESS\n");
		}
		else{
			DEBUG(DB_VM, "READ/WRITE stack/heap: faultAddr %x -- ppage %d\n", faultaddress, idx);
		}
		return 0;
	}

	if(entry_ptr->paddr & TLBLO_VALID){
		// DEBUG(DB_VM, "VALID PTE--(%x, %x) adding to TLB\n", faultaddress, entry_ptr->paddr);
		
		tlb_insert(faultaddress, entry_ptr->paddr);
		return 0;
	}

	if(entry_ptr->paddr & PT_SWAP){
		// SWAP IMPLEMENTATION!!!
		panic("SWAP CASE NOT IMPLEMENTED!\n");
		return 0;
	}
	
	panic("Unexpected case happens!!!\n");
	return EFAULT;
}


//VM_FAULT_READ        0   TLB miss on load -> A read was attempted
//VM_FAULT_WRITE       1   TLB miss on store -> A write was attempted
//VM_FAULT_READONLY    2   TLB Modify (write to read-only page) -> A write to a readonly page was attempted
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	/*
	 * Definitely write this.
	 */
	(void)faulttype;
	(void)faultaddress;

	if(faultaddress == 0 || faultaddress == 0x80000000 || faultaddress == 0x40000000){
		DEBUG(DB_VM, "ADDRESS NULL PTR, SEGMENTATION FAULT\n");
		return EFAULT;
	}

	REGION addr_region = -1;
	int err;

	struct addrspace* as = curthread->t_vmspace;


	err = define_addr_region(as, faulttype, faultaddress, &addr_region);
	if(err){
		DEBUG(DB_VM, "INVALID ADDRESS ACCESS!!!\n\n");
		return err;
	} 

	// DEBUG(DB_VM,"Fault type %d Accessing addr %x in REGION %d\n", faulttype, faultaddress, addr_region);

	faultaddress &= PAGE_FRAME;


	if(faulttype == VM_FAULT_READONLY){
		panic("NOT IMPELEMTED\n");
		err = vm_fault_READONLY(as, faultaddress, addr_region);
		if(err) {
			return err;
		}
	}
	else{
		err = vm_fault_READWRITE(as, faultaddress, addr_region);
		if(err) {
			return err;
		}
	}
	return 0;
}

