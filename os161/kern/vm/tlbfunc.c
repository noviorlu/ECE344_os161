//TLB capsulated functions
#include <coremap.h>
#include <tlbfunc.h>

void tlb_boostrap(){
    tlb_disable_all();
}

void tlb_insert(vaddr_t vaddr, paddr_t paddr){
    int i, spl;
	spl = splhigh();

    vaddr_t vtemp;
    paddr_t ptemp;
    for (i=0; i<NUM_TLB; i++) {
        TLB_Read(&vtemp, &ptemp, i);

        if(ptemp & TLBLO_VALID)
            continue;

        if(vtemp == vaddr)
            continue;

        TLB_Write(vaddr, paddr, i);

        TLB_Read(&vtemp, &ptemp, i);
        if(vtemp != vaddr || ptemp != paddr) panic("TLB (%x, %x) NOT INSTALLED!!!\n", vaddr, paddr);

        splx(spl);
		return;
	}

    TLB_Random(vaddr, paddr);

	splx(spl);
}


void tlb_disable_all(){
    int i, spl;
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void tlb_disable_idx(int idx){
    int spl;
    spl = splhigh();

	TLB_Write(TLBHI_INVALID(idx), TLBLO_INVALID(), idx);

    splx(spl);
}

void tlb_disable_vaddr(vaddr_t vaddr){
    int spl;
    spl = splhigh();

    int idx = TLB_Probe(vaddr, 0);

    if(idx >= 0 || idx < NUM_TLB)
        TLB_Write(TLBHI_INVALID(idx), TLBLO_INVALID(), idx);

    splx(spl);
}
