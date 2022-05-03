#include <pagetable.h>
#include <coremap.h>

int readPTE(vaddr_t* pt, vaddr_t faultaddress, vaddr_t** last_layer_pt){
	vaddr_t vpage = faultaddress >> 12;
	int i;

	for (i = 0; i < PTLAYERS; i++){
		vaddr_t* temp = (vaddr_t*)(pt[vpage & PTMASK]);
		if(temp == 0) {
			return -1;
		}
		if(i == PTLAYERS-1) break;

		pt = temp;
		vpage = vpage >> PTSHIFT;
	}
	
	// PTE exist, return true
	*last_layer_pt = pt;

	// kprintf("(%x, %x)\n" ,faultaddress, PADDR_TO_PPAGE( ((PTE*)pt[vpage & PTMASK])->paddr ) );
	return vpage & PTMASK;
}

/* 
 * return 0 if pte found
 * return -1 if upperlevel PT/PTE not created
 */
int examingPT(vaddr_t* pt, vaddr_t faultaddress, PTE** entry_ptr){
	vaddr_t vpage = faultaddress >> 12;
	int i, mark = 5;

	for (i = 0; i < PTLAYERS; i++){
		vaddr_t* temp = (vaddr_t*)(pt[vpage & PTMASK]);
		if(temp == 0){
			mark = i;
			break;
		}
		pt = temp;
		vpage = vpage >> PTSHIFT;
	}
	
	// PTE exist, return true
	if(mark == 5){
		*entry_ptr = (PTE *)pt;
		return 0;
	}

	for(i = mark; i < PTLAYERS-1; i++){
		pt[vpage & PTMASK] = (vaddr_t)kmalloc(sizeof(vaddr_t) * PTENTRY);
		bzero((vaddr_t *)pt[vpage & PTMASK], sizeof(vaddr_t) * PTENTRY);
		
		pt = (vaddr_t*)(pt[vpage & PTMASK]);
		vpage = vpage >> PTSHIFT;
	}

	pt[vpage] = (vaddr_t)kmalloc(sizeof(PTE));

	*entry_ptr = (PTE *)pt[vpage];
	(*entry_ptr)->paddr = 0;
	(*entry_ptr)->share = 1;
	return -1;
}

void printPTE(vaddr_t* pt, vaddr_t faultaddress){
	vaddr_t vpage = faultaddress >> 12;
	
	int i;
	for (i = 0; i < PTLAYERS; i++){
		pt = (vaddr_t*)(pt[vpage & PTMASK]);
		vpage = vpage >> PTSHIFT;
	}

	DEBUG(DB_VM, "In PT: %x ---- %d\n", faultaddress, PADDR_TO_PPAGE(((PTE*)pt)->paddr));
}

// void setPTE(vaddr_t* pt, vaddr_t faultaddress){
// 	vaddr_t vpage = faultaddress >> 12;
	
// 	int i;
// 	for (i = 0; i < PTLAYERS; i++){
// 		pt = (vaddr_t*)(pt[vpage & PTMASK]);
// 		vpage = vpage >> PTSHIFT;
// 	}

// 	((PTE*)pt) = NULL;
// }

void free_PT_helper(vaddr_t vaddr, vaddr_t* pt, int layer){
	if(layer > PTLAYERS){
		panic("SOMETHING IS WRONG!!!!\n");
	}

	if(layer == PTLAYERS){
		if(pt == 0)
			return;

		// pointer points to PTE, free coremap & PTE
		// SWAP IMPLEMENTATION!!!
		paddr_t paddr = ((PTE*)pt)->paddr & PT_PAGE;

		if(((PTE*)pt)->paddr & PT_VALID){
			free_ppage(paddr);
		}
		return;
	}


	int i;
	for(i=0; i < PTENTRY; i++){
		if(pt[i] != 0){
			vaddr |= (i << (layer * PTSHIFT));
			free_PT_helper(vaddr, (vaddr_t*)pt[i], layer+1);

			kfree((vaddr_t*)pt[i]);
			pt[i] = 0;
		}
	}
}

void free_PT(vaddr_t* pt){	
	// When clearing PT, also set coremap
	free_PT_helper(0, pt, 0);
}

void cpy_PT_helper(vaddr_t vaddr, vaddr_t* oldpt, vaddr_t* newpt, int layer){
	int i;
	
	if(layer == PTLAYERS-1){

		for(i=0; i < PTENTRY; i++){
			if(oldpt[i] == 0) continue;
			
			PTE* entry_ptr = (PTE*)newpt[i] = oldpt[i];

			if(entry_ptr->paddr & PT_VALID){
				entry_ptr->paddr &= (~PT_READWRITE);
				entry_ptr->share++;
			}

		}
		return;
	}

	for(i=0; i < PTENTRY; i++){
		if(oldpt[i] != 0){
			vaddr |= (i << (layer * PTSHIFT));

			newpt[i] = (vaddr_t)kmalloc(sizeof(vaddr_t) * PTENTRY);
			bzero((vaddr_t *)newpt[i], sizeof(vaddr_t) * PTENTRY);

			cpy_PT_helper(vaddr, (vaddr_t*)oldpt[i], (vaddr_t*)newpt[i], layer+1);
		}
	}
}

void cpy_PT(vaddr_t* oldpt, vaddr_t* newpt){	
	// When clearing PT, also set coremap
	cpy_PT_helper(0, oldpt, newpt, 0);
	tlb_disable_all();
}
