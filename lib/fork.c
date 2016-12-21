// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR))
		panic("pgfault: FEC_WR");

	pte_t pte = uvpt[PGNUM(addr)];
	if ((pte & (PTE_P | PTE_U | PTE_COW)) != (PTE_P | PTE_U | PTE_COW))
		panic("pgfault: PTE_P | PTE_U | PTE_COW)");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if  ((r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_P | PTE_W)) < 0)
		panic("pgfault: sys_page_alloc");
	void *va = (void *)ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, va, PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, va, PTE_U | PTE_P | PTE_W)) < 0)
		panic("pgfault: sys_page_map");
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("pgfault: sys_page_unmap");
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
	if ((pte & (PTE_U | PTE_P)) != (PTE_U | PTE_P))
		panic("duppage: permission");
	void *va = (void*)(pn * PGSIZE);

	// shared pages
	if ((pte & PTE_SHARE) == PTE_SHARE) {
		if ((r = sys_page_map(0, va, envid, va, pte & PTE_SYSCALL)) < 0)
			panic("duppage: shared sys_page_map %e", r);
		return 0;
	}

	// read-only pages
	if (!(pte & (PTE_W | PTE_COW))) {
		if ((r = sys_page_map(0, va, envid, va, PTE_P | (pte & PTE_U))) < 0)
			panic("duppage: readonly sys_page_map %e", r);
		return 0;
	}

	// writeable or copy-on-write pages
	uint32_t perm = PTE_P | PTE_COW | (pte & PTE_U);
	if ((r = sys_page_map(0, va, envid, va, perm)) < 0)
		panic("duppage: child sys_page_map %e, envid=%x", r, envid);
	if ((r = sys_page_map(0, va, 0, va, perm)) < 0)
		panic("duppage: parent sys_page_map %e", r);

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
	int rc;
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0)
		panic("fork: sys_exofork %e", envid);
	if (envid == 0) {
		// child
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// parent
	for (unsigned pn = 0; pn < PGNUM(UTOP)-1; ++pn) {
		unsigned pdx = pn / NPDENTRIES;
		if ((uvpd[pdx] & PTE_P) == PTE_P && (uvpt[pn] & PTE_P) == PTE_P)
			if ((rc = duppage(envid, pn)) < 0)
				panic("fork: duppage %e", rc);
	}
	if ((rc = sys_page_alloc(envid, (void*)UXSTACKTOP-PGSIZE, PTE_P | PTE_U | PTE_W)) < 0)
		panic("fork: sys_page_alloc %e", rc);
	if ((rc = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall)) < 0)
		panic("fork: sys_env_set_pgfault_upcall %e", rc);
	if ((rc = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("fork: sys_env_set_status %e", rc);
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
