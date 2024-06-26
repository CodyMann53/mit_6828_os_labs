// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Perform a backtrace of the kernel stack", mon_backtrace },
	{ "showmappings", "Display information about page mappings for a range of virtual addresses", show_mappings }
};

/***** Implementations of basic kernel monitor commands *****/
int show_mappings(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Error. Wrong number of arguments\n");
                cprintf("Usage: showmappings <start va> <stop va>.\n");
		return 0;
	}

	long start = ROUNDDOWN(strtol(argv[1], NULL, 0), PGSIZE);
	long end = ROUNDDOWN(strtol(argv[2], NULL, 0), PGSIZE);

	if (end < start){
		cprintf("End virtual address 0x%x must be greater than start 0x%x.\n", start, end);
		return 0;
	}

	for(; start <= end; start += PGSIZE){
		pte_t * pgEntry = pgdir_walk(kern_pgdir, (void *) start, false);
		if (!pgEntry){
			cprintf("va: 0x%x, pa: N/A, perms: \n", start);
			continue;
		}
		else{
			cprintf("va: 0x%x, pa: 0x%x, perms: ", start, PTE_ADDR(*pgEntry));
		}
	        
	        if (*pgEntry & 	PTE_P){
			cprintf("PTE_P,");
		}
	        if (*pgEntry & 	PTE_W){
			cprintf("PTE_W,");
		}
	        if (*pgEntry & 	PTE_U){
			cprintf("PTE_U,");
		}
	        if (*pgEntry & 	PTE_PWT){
			cprintf("PTE_PWT,");
		}
	        if (*pgEntry & 	PTE_PCD){
			cprintf("PTE_PCD,");
		}
	        if (*pgEntry & 	PTE_A){
			cprintf("PTE_A,");
		}
	        if (*pgEntry & 	PTE_D){
			cprintf("PTE_D,");
		}
	        if (*pgEntry & 	PTE_PS){
			cprintf("PTE_PS,");
		}
	        if (*pgEntry & 	PTE_G){
			cprintf("PTE_G,");
		}
		cprintf("\n");
	}

	return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp();
	uint32_t * stackFrame;
        while(ebp)
        {
		struct Eipdebuginfo info;
		memset(&info, 0, sizeof(struct Eipdebuginfo));

		stackFrame = (uint32_t *) ebp;
		debuginfo_eip(stackFrame[1], &info);
		cprintf("ebp %08x eip %08x args %08x %08x %08x %08x %08x\n",
			ebp,
                        stackFrame[1],
		        stackFrame[2],
		        stackFrame[3],
		        stackFrame[4],
		        stackFrame[5],
		        stackFrame[6]
		);
		cprintf("\t%s:%d: %.*s+%d\n", info.eip_file, info.eip_line, 
			info.eip_fn_namelen, info.eip_fn_name, 
			(int)(stackFrame[1] - info.eip_fn_addr));
		ebp = stackFrame[0];
        }

	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
