// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2010 Red Hat, Inc.
 * Copyright (c) 2025 Linux Test Project
 */

/*\
 * [Description]
 *
 * mmap/munmap /dev/zero: a common way of malloc()/free() anonymous
 * memory on Solaris.
 *
 * The basic purpose of this is a to test if it is possible to map and
 * unmap /dev/zero, and to read and write the mapping. Being inspired
 * by two bugs in the past, the design of the test was added some
 * variations based on the reproducers for them. It also accept an
 * option to mmap/munmap anonymous pages.
 *
 * One is to trigger panic with transparent hugepage feature that
 * split_huge_page is very strict in checking the rmap walk was
 * perfect. Keep it strict because if page_mapcount isn't stable and
 * just right, the __split_huge_page_refcount that follows the rmap
 * walk could lead to erratic page_count()s for the subpages. The bug
 * in fork lead to the rmap walk finding the parent huge-pmd twice
 * instead of just one, because the anon_vma_chain objects of the
 * child vma still point to the vma->vm_mm of the parent. That trips
 * on the split_huge_page mapcount vs page_mapcount check leading to a
 * BUG_ON.
 *
 * The other bug is mmap() of /dev/zero results in calling map_zero()
 * which on RHEL5 maps the ZERO_PAGE in every PTE within that virtual
 * address range. Since the application which maps a region from 5M to
 * 16M in size is also multi-threaded the subsequent munmap() of
 * /dev/zero results is TLB shootdowns to all other CPUs. When this
 * happens thousands or millions of times the application performance
 * is terrible. The mapping ZERO_PAGE in every pte within that virtual
 * address range was an optimization to make the subsequent pagefault
 * times faster on RHEL5 that has been removed/changed upstream.
 */

#include "safe_macros_fn.h"
#include "tst_test.h"
#include <sys/wait.h>

#define SIZE (5 * 1024 * 1024)
#define PATH_KSM "/sys/kernel/mm/ksm/"

static int fd;
static char *opt_anon, *opt_ksm;
static long ps;
static char *x;

static void setup(void)
{
	if (opt_ksm) {
		if (access(PATH_KSM, F_OK) == -1)
			tst_brk(TCONF, "KSM configuration is not enabled");
#ifdef HAVE_DECL_MADV_MERGEABLE
		tst_res(TINFO, "add to KSM regions.");
#else
		tst_brk(TCONF, "MADV_MERGEABLE missing in sys/mman.h");
#endif
	}

	if (opt_anon)
		tst_res(TINFO, "use anonymous pages.");
	else
		tst_res(TINFO, "use /dev/zero.");

	ps = SAFE_SYSCONF(_SC_PAGESIZE);
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
}

static void run(void)
{
	pid_t pid;

	tst_res(TINFO, "start tests.");

	if (opt_anon) {
		x = SAFE_MMAP(NULL, SIZE + SIZE - ps, PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	} else {
		fd = SAFE_OPEN("/dev/zero", O_RDWR, 0666);
		x = SAFE_MMAP(NULL, SIZE + SIZE - ps, PROT_READ | PROT_WRITE,
			      MAP_PRIVATE, fd, 0);
	}
#ifdef HAVE_DECL_MADV_MERGEABLE
	if (opt_ksm)
		if (madvise(x, SIZE + SIZE - ps, MADV_MERGEABLE) == -1)
			tst_brk(TBROK | TERRNO, "madvise");
#endif
	x[SIZE] = 0;

	pid = SAFE_FORK();
	if (pid == 0) {
		SAFE_MUNMAP(x + SIZE + ps, SIZE - ps - ps);
		exit(0);
	}

	pid = SAFE_FORK();
	if (pid == 0) {
		SAFE_MUNMAP(x + SIZE + ps, SIZE - ps - ps);
		exit(0);
	} else {
		pid = SAFE_FORK();
		if (pid == 0) {
			SAFE_MUNMAP(x + SIZE + ps, SIZE - ps - ps);
			exit(0);
		}
	}

	SAFE_MUNMAP(x, SIZE + SIZE - ps);

	while (waitpid(-1, &pid, WUNTRACED | WCONTINUED) > 0)
		if (WEXITSTATUS(pid) != 0)
			tst_res(TFAIL, "child exit status is %d",
				WEXITSTATUS(pid));

	tst_res(TPASS, "mmap/munmap operations completed successfully");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.options =
		(struct tst_option[]){
			{ "a", &opt_anon, "Test anonymous pages" },
			{ "s", &opt_ksm, "Add to KSM regions" },
			{},
		},
};
