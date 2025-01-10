// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2000 Juan Quintela <quintela@fi.udc.es>
 *                    Aaron Laffin <alaffin@sgi.com>
 * Copyright (c) 2025 Linux Test Project
 */

/*\
 * [Description]
 *
 * This test will map a big file to memory and write to it once,
 * making sure nothing bad happened in between such as a SIGSEGV.
 */

#include "tst_test.h"

static int fd = -1;
static int m_opt = 1000;
static char *m_copt;

static void setup(void)
{
	if (tst_parse_int(m_copt, &m_opt, 1, INT_MAX))
		tst_brk(TBROK, "Invalid size of mmap '%s'", m_copt);
}

static void run(void)
{
	pid_t pid;
	int status;
	char *array;
	unsigned int i;
	unsigned int pages, memsize;

	memsize = m_opt;
	pages = m_opt;
	memsize *= getpagesize();

	tst_res(TINFO, "mmap()ing file of %u pages or %u bytes", pages,
		memsize);

	fd = SAFE_OPEN("testfile", O_RDWR | O_CREAT);
	SAFE_LSEEK(fd, memsize, SEEK_SET);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, "\0", 1);

	pid = SAFE_FORK();
	if (pid == 0) {
		array = SAFE_MMAP(NULL, memsize, PROT_WRITE, MAP_SHARED, fd, 0);
		tst_res(TINFO, "touching mmapped memory");
		for (i = 0; i < memsize; i++)
			array[i] = (char)i;
		SAFE_MSYNC(array, memsize, MS_SYNC);
		SAFE_MUNMAP(array, memsize);
		exit(0);
	} else {
		SAFE_WAITPID(pid, &status, 0);
		if (WIFSIGNALED(status) && WEXITSTATUS(status) == SIGSEGV)
			tst_res(TFAIL, "test was killed by SIGSEGV");
		else
			tst_res(TPASS,
				"memory was mapped and written to successfully");
	}
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.cleanup = cleanup,
	.forks_child = 1,
	.options =
		(struct tst_option[]){
			{ "m:", &m_copt,
			  "Size of mmap in pages (default 1000)" },
			{},
		},
};
