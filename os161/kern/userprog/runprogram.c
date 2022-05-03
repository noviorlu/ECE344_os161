/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
// int
// runprogram_origin(char *progname, char** args, int nargs)
// {
// 	//ASSET3-------------------------------------------
// 	if(progname == NULL)
// 		return EFAULT;
// 	if(args == NULL)
// 	 	return EFAULT;

// 	int i = 0, err = 0;
// 	for(i = 0; i<nargs; i++){
// 		if(args[i] == NULL)
// 	 		return EFAULT;
// 	}
	
// 	if (nargs >= 16)
// 		return E2BIG;
// 	//ASSET3 END---------------------------------------



// 	struct vnode *v;
// 	vaddr_t entrypoint, stackptr;
// 	int result;

// 	/* Open the file. */
// 	result = vfs_open(progname, O_RDONLY, &v);
// 	if (result) {
// 		return result;
// 	}

// 	/* We should be a new thread. */
// 	assert(curthread->t_vmspace == NULL);

// 	/* Create a new address space. */
// 	curthread->t_vmspace = as_create();
// 	if (curthread->t_vmspace==NULL) {
// 		vfs_close(v);
// 		return ENOMEM;
// 	}

// 	/* Activate it. */
// 	as_activate(curthread->t_vmspace);

// 	/* Load the executable. */
// 	result = load_elf(v, &entrypoint);
// 	if (result) {
// 		/* thread_exit destroys curthread->t_vmspace */
// 		vfs_close(v);
// 		return result;
// 	}

// 	/* Done with the file now. */
// 	//vfs_close(v);

// 	/* Define the user stack in the address space */
// 	result = as_define_stack(curthread->t_vmspace, &stackptr);
// 	if (result) {
// 		/* thread_exit destroys curthread->t_vmspace */
// 		return result;
// 	}

	
// 	//ASSET3-------------------------------------------
// 	userptr_t* argv_ptrs = kmalloc(sizeof(userptr_t) * nargs);

// 	for(i = nargs-1; i>=0; i--){
// 		int len = strlen(args[i]) + 1;
// 		if( (len%4) != 0){
// 			len = (len/4 + 1) * 4;
// 		}

// 		stackptr -= len;
// 		argv_ptrs[i] = (userptr_t)stackptr;

// 		err = copyout(args[i], (userptr_t)stackptr, len);

// 		if(err){
// 			kfree(argv_ptrs);
// 			return err;
// 		}
// 	}
// 	// set the nargs+1 position to null
// 	stackptr -= 4;
// 	bzero((void*)stackptr, (sizeof(char)*4));

// 	for(i = nargs-1; i>=0; i--){
// 		stackptr -= 4;

// 		err = copyout(&argv_ptrs[i], (userptr_t)stackptr, 4);
// 		if(err){
// 			kfree(argv_ptrs);
// 			return err;
// 		}
// 	}
// 	kfree(argv_ptrs);
// 	//ASSET3 END---------------------------------------






// 	/* Warp to user mode. */
// 	md_usermode(nargs /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
// 		    stackptr, entrypoint);

// 	/* md_usermode does not return */
// 	panic("md_usermode returned\n");
// 	return EINVAL;
// }

int
runprogram(char *progname, char** args, int nargs)
{	
	//ASSET3-------------------------------------------
	if(progname == NULL)
		return EFAULT;
	if(args == NULL)
	 	return EFAULT;

	int i = 0, err = 0;
	for(i = 0; i<nargs; i++){
		if(args[i] == NULL)
	 		return EFAULT;
	}
	
	if (nargs >= 16)
		return E2BIG;
	//ASSET3 END---------------------------------------

	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();

	if (curthread->t_vmspace==NULL) {
		return ENOMEM;
	}

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	curthread->t_vmspace->execvfile = v;

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}


	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}


	DEBUG(DB_VM, "\nREGION1: base %x\tpage %d\toff %x fz %d\nREGION2: base %x\tpage %d\toff %x fz %d\n", 
		curthread->t_vmspace->as_vbase1,
		curthread->t_vmspace->as_npages1,
		curthread->t_vmspace->as_off1,
		curthread->t_vmspace->as_filesz1,
		curthread->t_vmspace->as_vbase2,
		curthread->t_vmspace->as_npages2,
		curthread->t_vmspace->as_off2,
		curthread->t_vmspace->as_filesz2
	);
	DEBUG(DB_VM, "HEAP: base %x\ttop %x\n", 
		curthread->t_vmspace->as_heapvtop,
		curthread->t_vmspace->as_heapvbase
	);

	DEBUG(DB_VM, "STACK: bottom %x\n\n", 
		curthread->t_vmspace->as_stackbottom
	);
	
	//ASSET3-------------------------------------------
	userptr_t* argv_ptrs = kmalloc(sizeof(userptr_t) * nargs);

	for(i = nargs-1; i>=0; i--){
		int len = strlen(args[i]) + 1;
		if( (len%4) != 0){
			len = (len/4 + 1) * 4;
		}

		stackptr -= len;
		argv_ptrs[i] = (userptr_t)stackptr;

		err = copyout(args[i], (userptr_t)stackptr, len);

		if(err){
			kfree(argv_ptrs);
			return err;
		}
	}
	// set the nargs+1 position to null
	stackptr -= 4;
	bzero((void*)stackptr, (sizeof(char)*4));

	for(i = nargs-1; i>=0; i--){
		stackptr -= 4;

		err = copyout(&argv_ptrs[i], (userptr_t)stackptr, 4);
		if(err){
			kfree(argv_ptrs);
			return err;
		}
	}
	kfree(argv_ptrs);
	//ASSET3 END---------------------------------------


	/* Warp to user mode. */
	md_usermode(nargs /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
		    stackptr, entrypoint);

	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}
