#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>

#include <kern/unistd.h>
#include <clock.h>

#include <addrspace.h>
#include <thread.h>
#include <curthread.h>

#include <vm.h>
#include <vfs.h>
#include <kern/limits.h>

#define DB_SBRK 1

int sys_sbrk(int amount, int* retval){
	DEBUG(DB_VM, "SBRK: ammount %x -- ", amount);
	(void)amount;
	(void)retval;

	if(amount & 0x3){
		if(DB_SBRK)
			kprintf("SBRK: AMOUNT not alligned, return EINVAL\n");
		return EINVAL;
	}

	vaddr_t heapoldtop = curthread->t_vmspace->as_heapvtop;
	vaddr_t heapbottom = curthread->t_vmspace->as_heapvbase;
	vaddr_t stackbottom = curthread->t_vmspace->as_stackbottom;
	
	vaddr_t heapnewtop = heapoldtop + amount;

	DEBUG(DB_VM, "heapOldtop %x -- heapnewtop %x -- heapbot %x -- stackbot %x\n", 
		heapoldtop,
		heapnewtop,
		heapbottom,
		stackbottom
	);


	if(heapnewtop > heapoldtop + HEAP_V_SIZE * PAGE_SIZE){
		if(DB_SBRK)
			kprintf("SBRK: heap reaches stack, return ENOMEM\n");
		return ENOMEM;
	}

	if(heapnewtop < heapbottom){
		if(DB_SBRK)
			kprintf("SBRK: heap free code Region, return EINVAL\n");
		return EINVAL;
	}

	curthread->t_vmspace->as_heapvtop = heapnewtop;
	*retval = heapoldtop;
	return 0;
}

void clearKA(char** kern_argv, int narg){
	if(kern_argv == NULL) return;
	
	int i;
	for(i = 0; i < narg; i++){
		if(kern_argv[i] != NULL)
			kfree(kern_argv[i]);
	}

	kfree(kern_argv);
}
/* 
	ENODEV	The device prefix of program did not exist.
	ENOTDIR	A non-final component of program was not a directory.
	ENOENT	program did not exist.
	EISDIR	program is a directory.
	ENOEXEC	program is not in a recognizable executable file format, was for the wrong platform, or contained invalid fields.
	ENOMEM	Insufficient virtual memory is available.
	E2BIG	The total size of the argument strings is too large.
	EIO	A hard I/O error occurred.
	EFAULT	One of the args is an invalid pointer. 
*/
int sys_execv(char *prog, char **argvs){
	(void)argvs;
	//determine if argvs & prog is workable
	//ASSET3-------------------------------------------
	if(prog == NULL)
		return EFAULT;
	if((unsigned int)prog == 0x80000000)
		return EFAULT;
	if((unsigned int)prog == 0x40000000)
		return EFAULT;

	if((*prog) == 0)
		return EINVAL;


	if(argvs == NULL)
		return EFAULT;
	if((unsigned int)argvs == 0x80000000)
		return EFAULT;
	if((unsigned int)argvs == 0x40000000)
		return EFAULT;


	int i = 0, narg = 0, err = 0;
	while(argvs[narg]!=NULL){
		if((unsigned int)argvs[narg] == 0x80000000)
			return EFAULT;
		if((unsigned int)argvs[narg] == 0x40000000)
			return EFAULT;
		narg+=1;
	}

	if (narg >= 16)
		return E2BIG;

	// Since forked userSpace is destroied during program reading,
	// need to kmalloc and copy the argv instructions into kernel
	int proglen = (strlen(prog)+1);
	char * progname = kmalloc(sizeof(char) * proglen );
	if (progname == NULL)
		return ENOMEM;

	//ASST4---------------------------------------------
	kfree(curthread->t_name);
	curthread->t_name = progname;
	//ASST4 END-----------------------------------------

	err = copyin((const_userptr_t)prog, progname, proglen);
	if(err){
		return EFAULT;
	} 

	char** kern_argv = kmalloc(sizeof(char*) * narg);
	if(kern_argv == NULL) {
		return ENOMEM;
	}

	for(i=0; i < narg; i++){
		int len = strlen(argvs[i]) + 1;


		kern_argv[i] = kmalloc(sizeof(char) * len);
		if(kern_argv[i] == NULL) {
			clearKA(kern_argv, narg);
			return ENOMEM;
		}
		
		err = copyin((const_userptr_t)argvs[i], kern_argv[i], len - 1);
		if(err){
			clearKA(kern_argv, narg);
			return EFAULT;
		} 

		kern_argv[i][len-1] = '\0';
	}


	//ASSET3 END---------------------------------------

	// free the t_vmspace allocated in fork
	if(curthread->t_vmspace != NULL){
		as_destroy(curthread->t_vmspace);
		curthread->t_vmspace = NULL;
	}

	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		clearKA(kern_argv, narg);
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		clearKA(kern_argv, narg);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		clearKA(kern_argv, narg);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		clearKA(kern_argv, narg);
		return result;
	}





	//ASSET3-------------------------------------------
	userptr_t* argv_ptrs = kmalloc(sizeof(userptr_t) * narg);

	for(i = narg-1; i>=0; i--){
		int len = strlen(kern_argv[i]) + 1;
		if( (len%4) != 0){
			len = (len/4 + 1) * 4;
		}

		stackptr -= len;
		argv_ptrs[i] = (userptr_t)stackptr;

		err = copyout(kern_argv[i], (userptr_t)stackptr, len);

		if(err){
			kfree(argv_ptrs);
			clearKA(kern_argv, narg);
			return err;
		}
	}
	// set the narg+1 position to null
	stackptr -= 4;
	bzero((void*)stackptr, (sizeof(char)*4));

	for(i = narg-1; i>=0; i--){
		stackptr -= 4;

		err = copyout(&argv_ptrs[i], (userptr_t)stackptr, 4);
		if(err){
			kfree(argv_ptrs);
			clearKA(kern_argv, narg);
			return err;
		}
	}
	kfree(argv_ptrs);
	clearKA(kern_argv, narg);
	//ASSET3 END---------------------------------------


	/* Warp to user mode. */
	md_usermode(narg /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
		    stackptr, entrypoint);
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}

int sys_waitpid(int pid, int *status, int options, int* retval){	
	if(status == NULL){	
		*retval = -1;	
		return EFAULT;	
	}	
	if((unsigned int)status == 0x80000000)
		return EFAULT;
	if((unsigned int)status == 0x40000000)
		return EFAULT;
		
	if(options != 0){	
		*retval = -1;	
		return EINVAL;	
	}	

	if(pid < 0 || pid >= MAX_PROCESS){
		*retval = -1;
		return EINVAL;
	}

	P(arrayMutex);

	if(parent_table[pid] != curthread->pid){
		V(arrayMutex);

		*retval = -1;

		return EINVAL;
	}

	V(arrayMutex);

	P(waitP_table[pid]);
	P(arrayMutex);

	
	int err = copyout(&exitProcessCode[pid], (userptr_t)status, sizeof(int));
	
	if(err){
		V(arrayMutex);
		V(waitP_table[pid]);
		*retval = -1;	

		return err;	
	}

	exitProcessCode[pid] = -1;
	parent_table[pid] = -1;

	V(arrayMutex);
	V(waitP_table[pid]);

	*retval = pid;
	return 0;
}

void
sys__exit(int exitcode){
	// kprintf("thread:%x exit safely\n", (vaddr_t)curthread);
	P(arrayMutex);
	exitProcessCode[curthread->pid] = exitcode;
	
	V(waitP_table[curthread->pid]);
	
	V(arrayMutex);
	
	// Special case where called sys__exit but use with thread_join
	// thread_exit will check the waitP_table again
	thread_exit();
}

/*
 * write for console output similarly use kprintf
 */
int 
sys_write(int fd, const void *buf, size_t size, int* retval){

	if(buf == NULL) return EFAULT;
	
	// normally write to FD
	// current write to console

	if(fd != STDOUT_FILENO && fd != STDERR_FILENO){
		return EBADF;
	}

	char* s = kmalloc(sizeof(char) * (size));
	int err = copyin((const_userptr_t)buf, s, size);
	
	if(err){
		kfree(s);
		s = NULL;
		return err;
	}
	else{
		s[size] = '\0';
		kprintf("%s", s);
		*retval = strlen(s); 

		kfree(s);
		s = NULL;

		return 0;
	}
}


/*
 * read for console input, get typein from user
 */
int 
sys_read(int fd, void *buf, size_t size, int* retval){
	if(buf == NULL){
		*retval = -1;
		return EFAULT;
	} 
	if(fd != STDIN_FILENO){
		*retval = -1;
		return EBADF;
	}
	if(size <= 0||size > 1){
		*retval = -1;
		return EIO;
	}

	char* s = kmalloc(sizeof(char) * (size));

	size_t i = 0;
	while(i < size){
		s[i] = (char)getch();
		i++;
	}

	int err = copyout(s, (userptr_t)buf, size);

	if(err){
		kfree(s);
		s = NULL;

		return err;
	}
	else{
		*retval = strlen(s); 

		kfree(s);
		s = NULL;

		return 0;
	}
}


void 
sys_sleep(int seconds){
	clocksleep(seconds);
}

// time in second is returned
// If seconds and/or nanoseconds are non-null, the corresponding
// components of the time are stored through those pointers. 
int
sys__time(time_t *seconds, unsigned long *nanoseconds, int* retval){
	
	if(seconds == NULL && nanoseconds == NULL)
		return EFAULT;

	time_t sec;
	u_int32_t nsec;
	gettime(&sec, &nsec);
	
	int err = 0;
	if(seconds != NULL){
		err = copyout(&sec, (userptr_t)seconds, sizeof(time_t));
		if(err) return err;
	}
	if(nanoseconds != NULL){
		err = copyout(&nsec, (userptr_t)nanoseconds, sizeof(u_int32_t));
		if(err) return err;
	}
	*retval = sec;
	return 0;
}

// ASST3 -----------------------------------
void
md_forkentry(void* orign_tf, unsigned long cpy_tvmspace)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */
	
	// Trapframe should be the frist thing in User Level Stack
	if((struct addrspace *)cpy_tvmspace == NULL)
		panic("Tvm_space NULL for pid: %d\n", curthread->pid);
	
	struct trapframe tf;
	
	//tf = *((struct trapframe *)orign_tf);
	memcpy(&tf, orign_tf, sizeof(struct trapframe));
	
	kfree(orign_tf);

	curthread->t_vmspace = (struct addrspace *)cpy_tvmspace;
	as_activate(curthread->t_vmspace);


	tf.tf_v0 = 0;
	tf.tf_a3 = 0;
	tf.tf_epc += 4;

	mips_usermode(&tf);
}

// EAGAIN	Too many processes already exist.
// ENOMEM	Sufficient virtual memory for the new process was not available.
int sys_fork(struct trapframe* tf, int* retval){
	// copy the virtual memporyspace
	int err = 0;

	//User Virtual Memory Space must be allocated in Kernel Level Heap
	struct addrspace * cpy_tvmspace;
	err = as_copy(curthread->t_vmspace, &cpy_tvmspace);
	if(err != 0){
		retval = 0;
		return err;
	}

	struct trapframe* cpytf = kmalloc(sizeof(struct trapframe));
	if(cpytf == NULL){
		as_destroy(cpy_tvmspace);
		return ENOMEM;
	}
	memcpy(cpytf, tf, sizeof(struct trapframe));
	
	struct thread* new = NULL;
	err = thread_fork(
		"newProcess", 
		(void*)cpytf, 
		(unsigned long)cpy_tvmspace, 
		md_forkentry, 
		&new
	);

	if(err != 0){
		as_destroy(cpy_tvmspace);
		kfree(cpytf);
		
		*retval = 0;
		return err;
	}
	else{
		*retval = new->pid;

		P(arrayMutex);
		parent_table[new->pid] = curthread->pid;
		V(arrayMutex);


		return 0;
	}
	
}

void sys_getpid(int * retval){
	*retval = curthread->pid;
}
// ASST3 END -----------------------------------

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;
	err = 0;
	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    /* Add stuff here */
		case SYS__exit:
			sys__exit(tf->tf_a0);
			
		break;

		case SYS_write:
			err = sys_write(tf->tf_a0, (const void*)tf->tf_a1, (size_t)tf->tf_a2, &retval);
		break;
		
		case SYS_read:
			err = sys_read(tf->tf_a0, (void*)tf->tf_a1, (size_t)tf->tf_a2, &retval);
		break;	

		case SYS_sleep:
			sys_sleep(tf->tf_a0);
		break;	

		case SYS___time:
			err = sys__time((time_t*)tf->tf_a0, (unsigned long*)tf->tf_a1, &retval);
		break;	
	
		// ASST3 -----------------------------------
		case SYS_fork:
			err = sys_fork(tf, &retval);
		break;

		case SYS_getpid:
			sys_getpid(&retval);
		break;

		case SYS_waitpid:	
			err = sys_waitpid((int)tf->tf_a0, (int*)tf->tf_a1, (int)tf->tf_a2, &retval);	
		break;

		case SYS_execv:
			err = sys_execv((char *)tf->tf_a0, (char**)tf->tf_a1);
		break;

		case SYS_sbrk:
			err = sys_sbrk((u_int32_t)tf->tf_a0, &retval);
		break;

		//--------------------------------------------

	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}

	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}
