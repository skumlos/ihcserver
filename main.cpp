/**
 * Copyright (c) 2013, Martin Hejnfelt (martin@hejnfelt.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * The main function that instantiates IHCServer and
 * daemonize if needed.
 *
 * January 2013, Martin Hejnfelt (martin@hejnfelt.com)
 */

#include "IHCServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>

/* Change this to whatever your daemon is called */
#define DAEMON_NAME "ihcserver"

/* Change this to the user under which to run */
#define RUN_AS_USER "root"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


static void child_handler(int signum)
{
	switch(signum) {
		case SIGALRM: exit(EXIT_FAILURE); break;
		case SIGUSR1: exit(EXIT_SUCCESS); break;
		case SIGCHLD: exit(EXIT_FAILURE); break;
	}
}

void daemonize( const char *lockfile )
{
	pid_t pid, sid, parent;
	int lfp = -1;

	/* already a daemon */
	if ( getppid() == 1 ) return;

	/* Create the lock file as the current user */
	if ( lockfile && lockfile[0] ) {
		lfp = open(lockfile,O_RDWR|O_CREAT,0640);
		if ( lfp < 0 ) {
			syslog( LOG_ERR, "unable to create lock file %s, code=%d (%s)",
								lockfile, errno, strerror(errno) );
			exit(EXIT_FAILURE);
		}
	}

	/* Drop user if there is one, and we were run as root */
	if ( getuid() == 0 || geteuid() == 0 ) {
		struct passwd *pw = getpwnam(RUN_AS_USER);
		if ( pw ) {
			syslog( LOG_NOTICE, "setting user to " RUN_AS_USER );
			setuid( pw->pw_uid );
		}
	}

	/* Trap signals that we expect to recieve */
	signal(SIGCHLD,child_handler);
	signal(SIGUSR1,child_handler);
	signal(SIGALRM,child_handler);

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		syslog( LOG_ERR, "unable to fork daemon, code=%d (%s)",
			errno, strerror(errno) );
		exit(EXIT_FAILURE);
	}

	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		/* Wait for confirmation from the child via SIGTERM or SIGCHLD, or
		for two seconds to elapse (SIGALRM).  pause() should not return. */
		alarm(2);
		pause();

		exit(EXIT_FAILURE);
	}

	/* At this point we are executing as the child process */
	parent = getppid();

	/* Cancel certain signals */
	signal(SIGCHLD,SIG_DFL); /* A child process dies */
	signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
	signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		syslog( LOG_ERR, "unable to create a new session, code %d (%s)",
                					errno, strerror(errno) );
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		syslog( LOG_ERR, "unable to change directory to %s, code %d (%s)",
					"/", errno, strerror(errno) );
		exit(EXIT_FAILURE);
	}

	/* Redirect standard files to /dev/null */
	freopen( "/dev/null", "r", stdin);
	freopen( "/dev/null", "w", stdout);
	freopen( "/dev/null", "w", stderr);

	/* Tell the parent process that we are A-okay */
	kill( parent, SIGUSR1 );
}

int main(int argc, char* argv[]) {
	if(argc > 1 && strcmp(argv[1],"-d") == 0) {
		/* Initialize the logging interface */
		openlog( DAEMON_NAME, LOG_PID, LOG_LOCAL5 );
		syslog( LOG_INFO, "starting" );
		/* Daemonize */
		daemonize( "/var/lock/" DAEMON_NAME );
	}
	IHCServer* m_ihcserver = new IHCServer();
	while(m_ihcserver->isRunning()) {
		sleep(100);
	}
	delete m_ihcserver;
};
