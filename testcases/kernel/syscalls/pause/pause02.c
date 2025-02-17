// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) 2012 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) Linux Test Project, 2025
 */

/*
 * Verify that, pause() returns -1 and sets errno to EINTR after receipt of a
 * signal which is caught by the calling process. Also, verify that the calling
 * process will resume execution from the point of suspension.
 */

#include "tst_test.h"

static void sig_handler(int sig)
{
}

static void do_child(void)
{
	SAFE_SIGNAL(SIGALRM, SIG_DFL);
	SAFE_SIGNAL(SIGINT, sig_handler);

	/* Commit suicide after 10 seconds */
	alarm(10);

	TEST(pause());
	if (TST_RET == -1 && TST_ERR == EINTR)
		exit(0);
	else
		tst_res(TFAIL | TTERRNO, "pause() unexpected errno");

	exit(1);
}

void run(void)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid)
		do_child();

	TST_PROCESS_STATE_WAIT(pid, 'S', 10000);
	kill(pid, SIGINT);
	SAFE_WAITPID(pid, &status, 0);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0)
			tst_res(TPASS, "pause() was interrupted correctly");
		else
			tst_res(TFAIL, "Child exited with %i",
				WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		if (WTERMSIG(status) == SIGALRM)
			tst_res(TFAIL,
				"Timeout: SIGINT wasn't received by child");
		else
			tst_res(TFAIL, "Child killed by signal");
	} else {
		tst_res(TFAIL, "pause() was not interrupted");
	}
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};
