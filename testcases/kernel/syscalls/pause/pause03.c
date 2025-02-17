// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) Linux Test Project, 2025
 * 07/2001 Ported by Wayne Boyer
 */

/*
 * This test verifies that pause() does not return after receiving a SIGKILL
 * signal, at which point the process should be terminated.
 */

#include "tst_test.h"

void run(void)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		pause();
		tst_res(TFAIL, "Unexpected return from pause()");
		exit(0);
	}

	TST_PROCESS_STATE_WAIT(pid, 'S', 10000);
	kill(pid, SIGKILL);
	SAFE_WAITPID(pid, &status, 0);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL)
		tst_res(TPASS, "pause() did not return after SIGKILL");
	else
		tst_res(TFAIL, "Child exited with %i", WEXITSTATUS(status));
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};
