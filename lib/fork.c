// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void * addr = (void *) utf->utf_fault_va;
	uint32_t addrUint32 = utf->utf_fault_va;;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR))
	{
		panic("Non-write page fault error code in user fork page fault handler. VA = 0X%x\n", addr);
	} 

	if (!(uvpt[PGNUM(addr)] & PTE_COW))
	{
		panic("Fork page fault handler received page fault for address that was not mapped COW.");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	r = sys_page_alloc(0, (void *) PFTEMP, PTE_W | PTE_U | PTE_P);
	if (r != 0)
	{
		panic("Fork page fault handler failed to allocate temporary page for copying cow page into its address space");
	}

	memcpy((void *) PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);


	r = sys_page_map(0, (void *) PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_W | PTE_U | PTE_P);
	if (r != 0)
	{
		panic("Fork page fault handler failed to map copy of faulting address page.");
	}

	r = sys_page_unmap(0, (void *) PFTEMP);
	if (r != 0)
	{
		panic("Fork page fault handler failed to unmap PFTEMP during cleanup.");
	}

	return;
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte = uvpt[pn];
	uintptr_t va = pn*PGSIZE;
	uint32_t perm = PTE_P | PTE_U;

	if (pte & PTE_SHARE)
	{
		perm = pte & PTE_SYSCALL;
	}
	else if (((pte & PTE_W) || (pte & PTE_COW)))
	{
		perm = PTE_COW | perm;
	}


	r = sys_page_map(0, (void *) va, envid, (void *) va, perm);
	if (r != 0)
	{
		return r;
	}

	r = sys_page_map(0, (void *) va, 0, (void *) va, perm);
	if (r != 0)
	{
		return r;
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// 1. The parent installs pgfault() as the C-level page fault handler, using the set_pgfault_handler()
	set_pgfault_handler(pgfault);

	// 2. The parent calls sys_exofork() to create a child environment.
	envid_t childEnvid = sys_exofork();

	if (childEnvid < 0)
	{
		cprintf("Failure to create a child environment in fork() call. Error: %e\n", childEnvid);
		return childEnvid;
	}
	else if (childEnvid == 0)
	{
		envid_t thisEnvId = sys_getenvid();

		thisenv = &envs[ENVX(thisEnvId)];
		return 0;
	}

	/* 
		3. For each writable or copy-on-write page in its address space below UTOP, the parent calls duppage, 
		which should map the page copy-on-write into the address space of the child and then remap the page 
		copy-on-write in its own address space. [ Note: The ordering here (i.e., marking a page as COW in the 
		child before marking it in the parent) actually matters! Can you see why? Try to think of a specific 
		case where reversing the order could cause trouble. ] duppage sets both PTEs so that the page is not writeable, 
		and to contain PTE_COW in the "avail" field to distinguish copy-on-write pages from genuine read-only pages.

		The exception stack is not remapped this way, however. Instead you need to allocate a fresh page in the child for 
		the exception stack. Since the page fault handler will be doing the actual copying and the page fault handler runs 
		on the exception stack, the exception stack cannot be made copy-on-write: who would copy it?

		fork() also needs to handle pages that are present, but not writable or copy-on-write.
        */

	int ret = sys_page_alloc(childEnvid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);
	if (ret != 0)
	{
		cprintf("Failed to allocate user exception stack page for child. Error: %e\n", ret);
		return ret;
	}

   	for (unsigned int pgNum = 0; pgNum < PGNUM(USTACKTOP); pgNum++)
	{
		// First check to make sure the page table page is mapped
		pde_t pdEntry = uvpd[PDX(pgNum*PGSIZE)];
		if (!(pdEntry & PTE_P))
		{
			continue;
		}

		// Check if the page is mapped
		pte_t ptEntry = uvpt[pgNum];
		if (!(ptEntry & PTE_P))
		{
			continue;
		}

		int ret = duppage(childEnvid, pgNum);
		if (ret != 0)
		{
			cprintf("Failure to duplicate page into the child environment in fork() call. Error: %e\n", ret);
			return ret;
		}
	}

	// 4. The parent sets the user page fault entrypoint for the child to look like its own.
	ret = sys_env_set_pgfault_upcall(childEnvid, _pgfault_upcall);
	if (ret != 0)
	{
		cprintf("Failure to setup the child's page fault entrypoint in fork() call. Error: %e\n", ret);
		return ret;
	}

	// 5. The child is now ready to run, so the parent marks it runnable.
	ret = sys_env_set_status(childEnvid, ENV_RUNNABLE);
	if (ret != 0)
	{
		cprintf("Failed to set child runnable in fork() call. Error: %e\n", ret);
		return ret;
	}

	return childEnvid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
