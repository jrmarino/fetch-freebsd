/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2000-2014 Dag-Erling Smørgrav
 * Copyright (c) 2013 Michael Gmelin <freebsd@grem.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "fetch.h"

#define MINBUFSIZE	16384
#define TIMEOUT		120

#ifndef TCSASOFT
#define TCSASOFT	0
#endif

#ifdef __APPLE__
#include <sys/ioctl.h>
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#endif

/* Option flags */
static int	 A_flag;	/*    -A: do not follow 302 redirects */
static int	 a_flag;	/*    -a: auto retry */
static off_t	 B_size;	/*    -B: buffer size */
static int	 b_flag;	/*!   -b: workaround TCP bug */
static char    *c_dirname;	/*    -c: remote directory */
static int	 d_flag;	/*    -d: direct connection */
static int	 F_flag;	/*    -F: restart without checking mtime  */
static char	*f_filename;	/*    -f: file to fetch */
static char	*h_hostname;	/*    -h: host to fetch from */
static int	 i_flag;	/*    -i: specify file for mtime comparison */
static char	*i_filename;	/*        name of input file */
static int	 l_flag;	/*    -l: link rather than copy file: URLs */
static int	 m_flag;	/* -[Mm]: mirror mode */
static char	*N_filename;	/*    -N: netrc file name */
static int	 n_flag;	/*    -n: do not preserve modification time */
static int	 o_flag;	/*    -o: specify output file */
static int	 o_directory;	/*        output file is a directory */
static char	*o_filename;	/*        name of output file */
static int	 o_stdout;	/*        output file is stdout */
static int	 once_flag;	/*    -1: stop at first successful file */
static int	 p_flag;	/* -[Pp]: use passive FTP */
static int	 R_flag;	/*    -R: don't delete partial files */
static int	 r_flag;	/*    -r: restart previous transfer */
static off_t	 S_size;        /*    -S: require size to match */
static int	 s_flag;        /*    -s: show size, don't fetch */
static long	 T_secs;	/*    -T: transfer timeout in seconds */
static int	 t_flag;	/*!   -t: workaround TCP bug */
static int	 U_flag;	/*    -U: do not use high ports */
static int	 v_level = 1;	/*    -v: verbosity level */
static int	 v_tty;		/*        stdout is a tty */
static int	 v_progress;	/*        whether to display progress */
static pid_t	 pgrp;		/*        our process group */
static long	 w_secs;	/*    -w: retry delay */
static int	 family = PF_UNSPEC;	/* -[46]: address family to use */

static int	 sigalrm;	/* SIGALRM received */
static int	 siginfo;	/* SIGINFO received */
static int	 sigint;	/* SIGINT received */

static long	 ftp_timeout = TIMEOUT;	/* default timeout for FTP transfers */
static long	 http_timeout = TIMEOUT;/* default timeout for HTTP transfers */
static char	*buf;		/* transfer buffer */

enum options
{
	OPTION_BIND_ADDRESS,
	OPTION_NO_FTP_PASSIVE_MODE,
	OPTION_HTTP_REFERER,
	OPTION_HTTP_USER_AGENT,
	OPTION_NO_PROXY,
	OPTION_SSL_CA_CERT_FILE,
	OPTION_SSL_CA_CERT_PATH,
	OPTION_SSL_CLIENT_CERT_FILE,
	OPTION_SSL_CLIENT_KEY_FILE,
	OPTION_SSL_CRL_FILE,
	OPTION_SSL_NO_SSL3,
	OPTION_SSL_NO_TLS1,
	OPTION_SSL_NO_VERIFY_HOSTNAME,
	OPTION_SSL_NO_VERIFY_PEER
};


static struct option longopts[] =
{
	/* mapping to single character argument */
	{ "one-file", no_argument, NULL, '1' },
	{ "ipv4-only", no_argument, NULL, '4' },
	{ "ipv6-only", no_argument, NULL, '6' },
	{ "no-redirect", no_argument, NULL, 'A' },
	{ "retry", no_argument, NULL, 'a' },
	{ "buffer-size", required_argument, NULL, 'B' },
	/* -c not mapped, since it's deprecated */
	{ "direct", no_argument, NULL, 'd' },
	{ "force-restart", no_argument, NULL, 'F' },
	/* -f not mapped, since it's deprecated */
	/* -h not mapped, since it's deprecated */
	{ "if-modified-since", required_argument, NULL, 'i' },
	{ "symlink", no_argument, NULL, 'l' },
	/* -M not mapped since it's the same as -m */
	{ "mirror", no_argument, NULL, 'm' },
	{ "netrc", required_argument, NULL, 'N' },
	{ "no-mtime", no_argument, NULL, 'n' },
	{ "output", required_argument, NULL, 'o' },
	/* -P not mapped since it's the same as -p */
	{ "passive", no_argument, NULL, 'p' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "keep-output", no_argument, NULL, 'R' },
	{ "restart", no_argument, NULL, 'r' },
	{ "require-size", required_argument, NULL, 'S' },
	{ "print-size", no_argument, NULL, 's' },
	{ "timeout", required_argument, NULL, 'T' },
	{ "passive-portrange-default", no_argument, NULL, 'T' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "retry-delay", required_argument, NULL, 'w' },

	/* options without a single character equivalent */
	{ "bind-address", required_argument, NULL, OPTION_BIND_ADDRESS },
	{ "no-passive", no_argument, NULL, OPTION_NO_FTP_PASSIVE_MODE },
	{ "referer", required_argument, NULL, OPTION_HTTP_REFERER },
	{ "user-agent", required_argument, NULL, OPTION_HTTP_USER_AGENT },
	{ "no-proxy", required_argument, NULL, OPTION_NO_PROXY },
	{ "ca-cert", required_argument, NULL, OPTION_SSL_CA_CERT_FILE },
	{ "ca-path", required_argument, NULL, OPTION_SSL_CA_CERT_PATH },
	{ "cert", required_argument, NULL, OPTION_SSL_CLIENT_CERT_FILE },
	{ "key", required_argument, NULL, OPTION_SSL_CLIENT_KEY_FILE },
	{ "crl", required_argument, NULL, OPTION_SSL_CRL_FILE },
	{ "no-sslv3", no_argument, NULL, OPTION_SSL_NO_SSL3 },
	{ "no-tlsv1", no_argument, NULL, OPTION_SSL_NO_TLS1 },
	{ "no-verify-hostname", no_argument, NULL, OPTION_SSL_NO_VERIFY_HOSTNAME },
	{ "no-verify-peer", no_argument, NULL, OPTION_SSL_NO_VERIFY_PEER },

	{ NULL, 0, NULL, 0 }
};

/*
 * Signal handler
 */
static void
sig_handler(int sig)
{
	switch (sig) {
	case SIGALRM:
		sigalrm = 1;
		break;
#ifdef USE_SIGPWR
	case SIGPWR:
#else
	case SIGINFO:
#endif
		siginfo = 1;
		break;
	case SIGINT:
		sigint = 1;
		break;
	}
}

struct xferstat {
	char		 name[64];
	struct timeval	 start;		/* start of transfer */
	struct timeval	 last;		/* time of last update */
	struct timeval	 last2;		/* time of previous last update */
	off_t		 size;		/* size of file per HTTP hdr */
	off_t		 offset;	/* starting offset in file */
	off_t		 rcvd;		/* bytes already received */
	off_t		 lastrcvd;	/* bytes received since last update */
};

/*
 * Format a number of seconds as either XXdYYh, XXhYYm, XXmYYs, or XXs
 * depending on its magnitude
 */
static void
stat_seconds(char *str, size_t strsz, long seconds)
{

	if (seconds > 86400)
		snprintf(str, strsz, "%02ldd%02ldh",
		    seconds / 86400, (seconds % 86400) / 3600);
	else if (seconds > 3600)
		snprintf(str, strsz, "%02ldh%02ldm",
		    seconds / 3600, (seconds % 3600) / 60);
	else if (seconds > 60)
		snprintf(str, strsz, "%02ldm%02lds",
		    seconds / 60, seconds % 60);
	else
		snprintf(str, strsz, "   %02lds",
		    seconds);
}

/*
 * Compute and display ETA
 */
static void
stat_eta(char *str, size_t strsz, const struct xferstat *xs)
{
	long elapsed, eta;
	off_t received, expected;

	elapsed = xs->last.tv_sec - xs->start.tv_sec;
	received = xs->rcvd - xs->offset;
	expected = xs->size - xs->rcvd;
	eta = (long)((double)elapsed * expected / received);
	if (eta > 0)
		stat_seconds(str, strsz, eta);
	else
		stat_seconds(str, strsz, elapsed);
}

/*
 * Format a number as "xxxx YB" where Y is ' ', 'k', 'M'...
 */
static const char *prefixes = " kMGTP";
static void
stat_bytes(char *str, size_t strsz, off_t bytes)
{
	const char *prefix = prefixes;

	while (bytes > 9999 && prefix[1] != '\0') {
		bytes /= 1024;
		prefix++;
	}
	snprintf(str, strsz, "%4ju %cB", (uintmax_t)bytes, *prefix);
}

/*
 * Compute and display transfer rate
 */
static void
stat_bps(char *str, size_t strsz, struct xferstat *xs)
{
	char bytes[16];
	double delta, bps;

	delta = ((double)xs->last.tv_sec + (xs->last.tv_usec / 1.e6))
	    - ((double)xs->last2.tv_sec + (xs->last2.tv_usec / 1.e6));

	if (delta == 0.0) {
		snprintf(str, strsz, "?? Bps");
	} else {
		bps = (xs->rcvd - xs->lastrcvd) / delta;
		stat_bytes(bytes, sizeof bytes, (off_t)bps);
		snprintf(str, strsz, "%sps", bytes);
	}
}

/*
 * Update the stats display
 */
static void
stat_display(struct xferstat *xs, int force)
{
	char bytes[16], bps[16], eta[16];
	struct timeval now;
	int ctty_pgrp;

	/* check if we're the foreground process */
	if (ioctl(STDERR_FILENO, TIOCGPGRP, &ctty_pgrp) != 0 ||
	    (pid_t)ctty_pgrp != pgrp)
		return;

	gettimeofday(&now, NULL);
	if (!force && now.tv_sec <= xs->last.tv_sec)
		return;
	xs->last2 = xs->last;
	xs->last = now;

	fprintf(stderr, "\r%-46.46s", xs->name);
	if (xs->rcvd >= xs->size) {
		stat_bytes(bytes, sizeof bytes, xs->rcvd);
#ifdef USE_SETPROCTITLE
		setproctitle("%s [%s]", xs->name, bytes);
#endif
		fprintf(stderr, "        %s", bytes);
	} else {
		stat_bytes(bytes, sizeof bytes, xs->size);
#ifdef USE_SETPROCTITLE
		setproctitle("%s [%d%% of %s]", xs->name,
		    (int)((100.0 * xs->rcvd) / xs->size),
		    bytes);
#endif
		fprintf(stderr, "%3d%% of %s",
		    (int)((100.0 * xs->rcvd) / xs->size),
		    bytes);
	}
	if (force == 2) {
		xs->lastrcvd = xs->offset;
		xs->last2 = xs->start;
	}
	stat_bps(bps, sizeof bps, xs);
	fprintf(stderr, " %s", bps);
	if ((xs->size > 0 && xs->rcvd > 0 &&
	     xs->last.tv_sec >= xs->start.tv_sec + 3) ||
	    force == 2) {
		stat_eta(eta, sizeof eta, xs);
		fprintf(stderr, " %s", eta);
	}
	xs->lastrcvd = xs->rcvd;
}

/*
 * Initialize the transfer statistics
 */
static void
stat_start(struct xferstat *xs, const char *name, off_t size, off_t offset)
{

	memset(xs, 0, sizeof *xs);
	snprintf(xs->name, sizeof xs->name, "%s", name);
	gettimeofday(&xs->start, NULL);
	xs->last2 = xs->last = xs->start;
	xs->size = size;
	xs->offset = offset;
	xs->rcvd = offset;
	xs->lastrcvd = offset;
	if (v_progress)
		stat_display(xs, 1);
	else if (v_level > 0)
		fprintf(stderr, "%-46s", xs->name);
}

/*
 * Update the transfer statistics
 */
static void
stat_update(struct xferstat *xs, off_t rcvd)
{

	xs->rcvd = rcvd;
	if (v_progress)
		stat_display(xs, 0);
}

/*
 * Finalize the transfer statistics
 */
static void
stat_end(struct xferstat *xs)
{
	char bytes[16], bps[16], eta[16];

	gettimeofday(&xs->last, NULL);
	if (v_progress) {
		stat_display(xs, 2);
		putc('\n', stderr);
	} else if (v_level > 0) {
		stat_bytes(bytes, sizeof bytes, xs->rcvd);
		stat_bps(bps, sizeof bps, xs);
		stat_eta(eta, sizeof eta, xs);
		fprintf(stderr, "        %s %s %s\n", bytes, bps, eta);
	}
}

/*
 * Ask the user for authentication details
 */
static int
query_auth(struct url *URL)
{
	struct termios tios;
	tcflag_t saved_flags;
	int i, nopwd;

	fprintf(stderr, "Authentication required for <%s://%s:%d/>!\n",
	    URL->scheme, URL->host, URL->port);

	fprintf(stderr, "Login: ");
	if (fgets(URL->user, sizeof URL->user, stdin) == NULL)
		return (-1);
	for (i = strlen(URL->user); i >= 0; --i)
		if (URL->user[i] == '\r' || URL->user[i] == '\n')
			URL->user[i] = '\0';

	fprintf(stderr, "Password: ");
	if (tcgetattr(STDIN_FILENO, &tios) == 0) {
		saved_flags = tios.c_lflag;
		tios.c_lflag &= ~ECHO;
		tios.c_lflag |= ECHONL|ICANON;
		tcsetattr(STDIN_FILENO, TCSAFLUSH|TCSASOFT, &tios);
		nopwd = (fgets(URL->pwd, sizeof URL->pwd, stdin) == NULL);
		tios.c_lflag = saved_flags;
		tcsetattr(STDIN_FILENO, TCSANOW|TCSASOFT, &tios);
	} else {
		nopwd = (fgets(URL->pwd, sizeof URL->pwd, stdin) == NULL);
	}
	if (nopwd)
		return (-1);
	for (i = strlen(URL->pwd); i >= 0; --i)
		if (URL->pwd[i] == '\r' || URL->pwd[i] == '\n')
			URL->pwd[i] = '\0';

	return (0);
}

/*
 * Fetch a file
 */
static int
fetch(char *URL, const char *path)
{
	struct url *url;
	struct url_stat us;
	struct stat sb, nsb;
	struct xferstat xs;
	FXRETTYPE f;
	FXRETTYPE of;
	size_t size, readcnt, wr;
	size_t len;
	off_t count;
	char flags[8];
	const char *slash;
	char *tmppath;
	int r;
	unsigned timeout;
	char *ptr;

	f = of = NULL;
	tmppath = NULL;

	timeout = 0;
	*flags = 0;
	count = 0;

	/* set verbosity level */
	if (v_level > 1)
		strcat(flags, "v");
	if (v_level > 2)
		fetchDebug = 1;

	/* parse URL */
	url = NULL;
	if (*URL == '\0') {
		fprintf(stderr, "empty URL\n");
		goto failure;
	}
	if ((url = fetchParseURL(URL)) == NULL) {
		fprintf(stderr, "%s: parse error\n", URL);
		goto failure;
	}

	/* if no scheme was specified, take a guess */
	if (!*url->scheme) {
		if (!*url->host)
			strcpy(url->scheme, SCHEME_FILE);
		else if (strncasecmp(url->host, "ftp.", 4) == 0)
			strcpy(url->scheme, SCHEME_FTP);
		else if (strncasecmp(url->host, "www.", 4) == 0)
			strcpy(url->scheme, SCHEME_HTTP);
	}

	/* common flags */
	switch (family) {
	case PF_INET:
		strcat(flags, "4");
		break;
	case PF_INET6:
		strcat(flags, "6");
		break;
	}

	/* FTP specific flags */
	if (strcmp(url->scheme, SCHEME_FTP) == 0) {
		if (p_flag)
			strcat(flags, "p");
		if (d_flag)
			strcat(flags, "d");
		if (U_flag)
			strcat(flags, "l");
		timeout = T_secs ? T_secs : ftp_timeout;
	}

	/* HTTP specific flags */
	if (strcmp(url->scheme, SCHEME_HTTP) == 0 ||
	    strcmp(url->scheme, SCHEME_HTTPS) == 0) {
		if (d_flag)
			strcat(flags, "d");
		if (A_flag)
			strcat(flags, "A");
		timeout = T_secs ? T_secs : http_timeout;
		if (i_flag) {
			if (stat(i_filename, &sb)) {
				fprintf(stderr, "%s: stat(): %s\n",
				  i_filename, strerror(errno));
				goto failure;
			}
			url->ims_time = sb.st_mtime;
			strcat(flags, "i");
		}
	}

	/* set the protocol timeout. */
	fetchTimeout = timeout;

	/* just print size */
	if (s_flag) {
		if (timeout)
			alarm(timeout);
		r = fetchStat(url, &us, flags);
		if (timeout)
			alarm(0);
		if (sigalrm || sigint)
			goto signal;
		if (r == -1) {
			fprintf(stderr, "%s\n", fetchLastErrString);
			goto failure;
		}
		if (us.size == -1)
			printf("Unknown\n");
		else
			printf("%jd\n", (intmax_t)us.size);
		goto success;
	}

	/*
	 * If the -r flag was specified, we have to compare the local
	 * and remote files, so we should really do a fetchStat()
	 * first, but I know of at least one HTTP server that only
	 * sends the content size in response to GET requests, and
	 * leaves it out of replies to HEAD requests.  Also, in the
	 * (frequent) case that the local and remote files match but
	 * the local file is truncated, we have sufficient information
	 * before the compare to issue a correct request.  Therefore,
	 * we always issue a GET request as if we were sure the local
	 * file was a truncated copy of the remote file; we can drop
	 * the connection later if we change our minds.
	 */
	sb.st_size = -1;
	if (!o_stdout) {
		r = stat(path, &sb);
		if (r == 0 && r_flag && S_ISREG(sb.st_mode)) {
			url->offset = sb.st_size;
		} else if (r == -1 || !S_ISREG(sb.st_mode)) {
			/*
			 * Whatever value sb.st_size has now is either
			 * wrong (if stat(2) failed) or irrelevant (if the
			 * path does not refer to a regular file)
			 */
			sb.st_size = -1;
		}
		if (r == -1 && errno != ENOENT) {
			fprintf(stderr, "%s: stat()\n", path);
			goto failure;
		}
	}

	/* start the transfer */
	if (timeout)
		alarm(timeout);
	f = fetchXGet(url, &us, flags);
	if (timeout)
		alarm(0);
	if (sigalrm || sigint)
		goto signal;
	if (f == NULL) {
		fprintf(stderr, "%s: %s\n", URL, fetchLastErrString);
		if (i_flag && (strcmp(url->scheme, SCHEME_HTTP) == 0 ||
		    strcmp(url->scheme, SCHEME_HTTPS) == 0) &&
		    fetchLastErrCode == FETCH_OK &&
		    strcmp(fetchLastErrString, "Not Modified") == 0) {
			/* HTTP Not Modified Response, return OK. */
			r = 0;
			goto done;
		} else
			goto failure;
	}
	if (sigint)
		goto signal;

	/* check that size is as expected */
	if (S_size) {
		if (us.size == -1) {
			fprintf(stderr, "%s: size unknown\n", URL);
		} else if (us.size != S_size) {
			fprintf(stderr,
			    "%s: size mismatch: expected %jd, actual %jd\n",
			    URL, (intmax_t)S_size, (intmax_t)us.size);
			goto failure;
		}
	}

	/* symlink instead of copy */
	if (l_flag && strcmp(url->scheme, "file") == 0 && !o_stdout) {
		if (symlink(url->doc, path) == -1) {
			fprintf(stderr, "%s: symlink(): %s\n", path,
			  strerror(errno));
			goto failure;
		}
		goto success;
	}

	if (us.size == -1 && !o_stdout && v_level > 0)
		fprintf(stderr, "%s: size of remote file is not known\n", URL);
	if (v_level > 1) {
		if (sb.st_size != -1)
			fprintf(stderr, "local size / mtime: %jd / %ld\n",
			    (intmax_t)sb.st_size, (long)sb.st_mtime);
		if (us.size != -1)
			fprintf(stderr, "remote size / mtime: %jd / %ld\n",
			    (intmax_t)us.size, (long)us.mtime);
	}

	/* open output file */
	if (o_stdout) {
		/* output to FXSTDOUT */
		of = FXSTDOUT;
	} else if (r_flag && sb.st_size != -1) {
		/* resume mode, local file exists */
		if (!F_flag && us.mtime && sb.st_mtime != us.mtime) {
			/* no match! have to refetch */
			FXCLOSE(f);
			/* if precious, warn the user and give up */
			if (R_flag) {
				fprintf(stderr, "%s: local modification time "
				    "does not match remote\n", path);
				goto failure_keep;
			}
		} else if (url->offset > sb.st_size) {
			/* gap between what we asked for and what we got */
			fprintf(stderr, "%s: gap in resume mode\n", URL);
			FXCLOSE(of);
			of = NULL;
			/* picked up again later */
		} else if (us.size != -1) {
			if (us.size == sb.st_size)
				/* nothing to do */
				goto success;
			if (sb.st_size > us.size) {
				/* local file too long! */
				fprintf(stderr, "%s: local file (%jd bytes) is longer "
				    "than remote file (%jd bytes)\n", path,
				    (intmax_t)sb.st_size, (intmax_t)us.size);
				goto failure;
			}
			/* we got it, open local file */
			if ((of = FXOPEN(path, "r+")) == NULL) {
				fprintf(stderr, "%s: FXOPEN(): %s\n", path,
				  strerror(errno));
				goto failure;
			}
			/* check that it didn't move under our feet */
			if (fstat(FXFILENO(of), &nsb) == -1) {
				/* can't happen! */
				fprintf(stderr, "%s: fstat(): %s\n", path,
				  strerror(errno));
				goto failure;
			}
			if (nsb.st_dev != sb.st_dev ||
			    nsb.st_ino != sb.st_ino ||
			    nsb.st_size != sb.st_size) {
				fprintf(stderr, "%s: file has changed\n", URL);
				FXCLOSE(of);
				of = NULL;
				sb = nsb;
				/* picked up again later */
			}
		}
		/* seek to where we left off */
		if (of != NULL && FXSEEKO(of, url->offset, SEEK_SET) != 0) {
			fprintf(stderr, "%s: FXSEEKO(): %s\n", path,
			  strerror(errno));
			FXCLOSE(of);
			of = NULL;
			/* picked up again later */
		}
	} else if (m_flag && sb.st_size != -1) {
		/* mirror mode, local file exists */
		if (sb.st_size == us.size && sb.st_mtime == us.mtime)
			goto success;
	}

	if (of == NULL) {
		/*
		 * We don't yet have an output file; either this is a
		 * vanilla run with no special flags, or the local and
		 * remote files didn't match.
		 */

		if (url->offset > 0) {
			/*
			 * We tried to restart a transfer, but for
			 * some reason gave up - so we have to restart
			 * from scratch if we want the whole file
			 */
			url->offset = 0;
			if ((f = fetchXGet(url, &us, flags)) == NULL) {
				fprintf(stderr, "%s: %s\n", URL,
				  fetchLastErrString);
				goto failure;
			}
			if (sigint)
				goto signal;
		}

		/* construct a temp file name */
		if (sb.st_size != -1 && S_ISREG(sb.st_mode)) {
			if ((slash = strrchr(path, '/')) == NULL)
				slash = path;
			else
				++slash;
			len = 1 + 14;
			len += (int)(slash - path);
			len += strlen(slash);
			tmppath = (char *) malloc(len);
			if (tmppath != NULL)
				if (snprintf (tmppath, len,
				   "%.*s.fetch.%s.XXXXXX",
				   (int)(slash - path),
				   path,
				   slash) < 0) {
					fprintf(stderr,
					  "%s: tmppath template: %s\n",
					  path, strerror(errno));
					goto failure;
				}
			if (tmppath != NULL) {
				if (mkstemp(tmppath) == -1) {
					fprintf(stderr, "%s: mkstemp(): %s\n",
					  path, strerror(errno));
					goto failure;
				}
				of = FXOPEN(tmppath, "w");
				chown(tmppath, sb.st_uid, sb.st_gid);
				chmod(tmppath, sb.st_mode & ALLPERMS);
			}
		}
		if (of == NULL)
			of = FXOPEN(path, "w");
		if (of == NULL) {
			fprintf(stderr, "%s: FXOPEN(): %s\n", path,
			  strerror(errno));
			goto failure;
		}
	}
	count = url->offset;

	/* start the counter */
	stat_start(&xs, path, us.size, count);

	sigalrm = siginfo = sigint = 0;

	/* suck in the data */
	FXSETVBUF(f, NULL, _IOFBF, B_size);
#ifdef SIGPWR
	signal(SIGPWR, sig_handler);
#else
	signal(SIGINFO, sig_handler);
#endif
	while (!sigint) {
		if (us.size != -1 && us.size - count < B_size &&
		    us.size - count >= 0)
			size = us.size - count;
		else
			size = B_size;
		if (siginfo) {
			stat_end(&xs);
			siginfo = 0;
		}

		if (size == 0)
			break;

		if ((readcnt = FXFREAD(buf, 1, size, f)) < size) {
			if (FXERROR(f) && errno == EINTR && !sigint)
				FXCLEARERR(f);
			else if (readcnt == 0)
				break;
		}

		stat_update(&xs, count += readcnt);
		for (ptr = buf; readcnt > 0; ptr += wr, readcnt -= wr)
			if ((wr = FXFWRITE(ptr, 1, readcnt, of)) < readcnt) {
				if (FXERROR(of) && errno == EINTR && !sigint)
					FXCLEARERR(of);
				else
					break;
			}
		if (readcnt != 0)
			break;
	}
	if (!sigalrm)
		sigalrm = FXERROR(f) && errno == ETIMEDOUT;
#ifdef USE_SIGPWR
	signal(SIGPWR, SIG_DFL);
#else
	signal(SIGINFO, SIG_DFL);
#endif

	stat_end(&xs);

	/*
	 * If the transfer timed out or was interrupted, we still want to
	 * set the mtime in case the file is not removed (-r or -R) and
	 * the user later restarts the transfer.
	 */
 signal:
	/* set mtime of local file */
	if (!n_flag && us.mtime && !o_stdout && of != NULL &&
	    (stat(path, &sb) != -1) && sb.st_mode & S_IFREG) {
		struct timeval tv[2];

		FXFLUSH(of);
		tv[0].tv_sec = (long)(us.atime ? us.atime : us.mtime);
		tv[1].tv_sec = (long)us.mtime;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		if (utimes(tmppath ? tmppath : path, tv))
			fprintf(stderr, "%s: utimes(): %s\n",
			  tmppath ? tmppath : path,
			  strerror(errno));
	}

	/* timed out or interrupted? */
	if (sigalrm)
		fprintf(stderr, "transfer timed out\n");
	if (sigint) {
		fprintf(stderr, "transfer interrupted\n");
		goto failure;
	}

	/* timeout / interrupt before connection completley established? */
	if (f == NULL)
		goto failure;

	if (!sigalrm) {
		/* check the status of our files */
		if (FXERROR(f))
			fprintf(stderr, "%s: %s\n", URL, strerror(errno));
		if (FXERROR(of))
			fprintf(stderr, "%s: %s\n", path, strerror(errno));
		if (FXERROR(f) || FXERROR(of))
			goto failure;
	}

	/* did the transfer complete normally? */
	if (us.size != -1 && count < us.size) {
		fprintf(stderr, "%s appears to be truncated: %jd/%jd bytes\n",
		    path, (intmax_t)count, (intmax_t)us.size);
		goto failure_keep;
	}

	/*
	 * If the transfer timed out and we didn't know how much to
	 * expect, assume the worst (i.e. we didn't get all of it)
	 */
	if (sigalrm && us.size == -1) {
		fprintf(stderr, "%s may be truncated\n", path);
		goto failure_keep;
	}

 success:
	r = 0;
	if (tmppath != NULL && rename(tmppath, path) == -1) {
		fprintf(stderr, "%s: rename(): %s\n", path, strerror(errno));
		goto failure_keep;
	}
	goto done;
 failure:
	if (of && of != FXSTDOUT && !R_flag && !r_flag)
		if (stat(path, &sb) != -1 && (sb.st_mode & S_IFREG))
			unlink(tmppath ? tmppath : path);
	if (R_flag && tmppath != NULL && sb.st_size == -1)
		rename(tmppath, path); /* ignore errors here */
 failure_keep:
	r = -1;
	goto done;
 done:
	if (f)
		FXCLOSE(f);
	if (of && of != FXSTDOUT)
		FXCLOSE(of);
	if (url)
		fetchFreeURL(url);
	if (tmppath != NULL)
		free(tmppath);
	return (r);
}

static void
usage(void)
{
	fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
"usage: fetch [-146AadFlMmnPpqRrsUv] [-B bytes] [--bind-address=host]",
"       [--ca-cert=file] [--ca-path=dir] [--cert=file] [--crl=file]",
"       [-i file] [--key=file] [-N file] [--no-passive] [--no-proxy=list]",
"       [--no-sslv3] [--no-tlsv1] [--no-verify-hostname] [--no-verify-peer]",
"       [-o file] [--referer=URL] [-S bytes] [-T seconds]",
"       [--user-agent=agent-string] [-w seconds] URL ...",
"       fetch [-146AadFlMmnPpqRrsUv] [-B bytes] [--bind-address=host]",
"       [--ca-cert=file] [--ca-path=dir] [--cert=file] [--crl=file]",
"       [-i file] [--key=file] [-N file] [--no-passive] [--no-proxy=list]",
"       [--no-sslv3] [--no-tlsv1] [--no-verify-hostname] [--no-verify-peer]",
"       [-o file] [--referer=URL] [-S bytes] [-T seconds]",
"       [--user-agent=agent-string] [-w seconds] -h host -f file [-c dir]");
}


/*
 * Entry point
 */
int
main(int argc, char *argv[])
{
	struct stat sb;
	struct sigaction sa;
	const char *p, *s;
	char *end, *q;
	int c, e, r;
	size_t len;


	while ((c = getopt_long(argc, argv,
	    "146AaB:bc:dFf:Hh:i:lMmN:nPpo:qRrS:sT:tUvw:",
	    longopts, NULL)) != -1)
		switch (c) {
		case '1':
			once_flag = 1;
			break;
		case '4':
			family = PF_INET;
			break;
		case '6':
			family = PF_INET6;
			break;
		case 'A':
			A_flag = 1;
			break;
		case 'a':
			a_flag = 1;
			break;
		case 'B':
			B_size = (off_t)strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0') {
				fprintf(stderr, "invalid buffer size (%s)\n",
				  optarg);
				exit(1);
			}
			break;
		case 'b':
			fprintf(stderr,
			  "warning: the -b option is deprecated\n");
			b_flag = 1;
			break;
		case 'c':
			c_dirname = optarg;
			break;
		case 'd':
			d_flag = 1;
			break;
		case 'F':
			F_flag = 1;
			break;
		case 'f':
			f_filename = optarg;
			break;
		case 'H':
			fprintf(stderr, "the -H option is now implicit, "
			    "use -U to disable\n");
			break;
		case 'h':
			h_hostname = optarg;
			break;
		case 'i':
			i_flag = 1;
			i_filename = optarg;
			break;
		case 'l':
			l_flag = 1;
			break;
		case 'o':
			o_flag = 1;
			o_filename = optarg;
			break;
		case 'M':
		case 'm':
			if (r_flag) {
				fprintf(stderr, "the -m and -r flags "
				  "are mutually exclusive\n");
				exit(1);
			}
			m_flag = 1;
			break;
		case 'N':
			N_filename = optarg;
			break;
		case 'n':
			n_flag = 1;
			break;
		case 'P':
		case 'p':
			p_flag = 1;
			break;
		case 'q':
			v_level = 0;
			break;
		case 'R':
			R_flag = 1;
			break;
		case 'r':
			if (m_flag) {
				fprintf(stderr, "the -m and -r flags "
				  "are mutually exclusive\n");
				exit(1);
			}
			r_flag = 1;
			break;
		case 'S':
			S_size = (off_t)strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0') {
				fprintf(stderr, "invalid size (%s)\n", optarg);
				exit(1);
			}
			break;
		case 's':
			s_flag = 1;
			break;
		case 'T':
			T_secs = strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0') {
				fprintf(stderr, "invalid timeout (%s)\n",
				  optarg);
				exit(1);
			}
			break;
		case 't':
			t_flag = 1;
			fprintf(stderr, "warning: "
			  "the -t option is deprecated\n");
			break;
		case 'U':
			U_flag = 1;
			break;
		case 'v':
			v_level++;
			break;
		case 'w':
			a_flag = 1;
			w_secs = strtol(optarg, &end, 10);
			if (*optarg == '\0' || *end != '\0') {
				fprintf(stderr, "invalid delay (%s)\n", optarg);
				exit(1);
			}
			break;
		case OPTION_BIND_ADDRESS:
			setenv("FETCH_BIND_ADDRESS", optarg, 1);
			break;
		case OPTION_NO_FTP_PASSIVE_MODE:
			setenv("FTP_PASSIVE_MODE", "no", 1);
			break;
		case OPTION_HTTP_REFERER:
			setenv("HTTP_REFERER", optarg, 1);
			break;
		case OPTION_HTTP_USER_AGENT:
			setenv("HTTP_USER_AGENT", optarg, 1);
			break;
		case OPTION_NO_PROXY:
			setenv("NO_PROXY", optarg, 1);
			break;
		case OPTION_SSL_CA_CERT_FILE:
			setenv("SSL_CA_CERT_FILE", optarg, 1);
			break;
		case OPTION_SSL_CA_CERT_PATH:
			setenv("SSL_CA_CERT_PATH", optarg, 1);
			break;
		case OPTION_SSL_CLIENT_CERT_FILE:
			setenv("SSL_CLIENT_CERT_FILE", optarg, 1);
			break;
		case OPTION_SSL_CLIENT_KEY_FILE:
			setenv("SSL_CLIENT_KEY_FILE", optarg, 1);
			break;
		case OPTION_SSL_CRL_FILE:
			setenv("SSL_CLIENT_CRL_FILE", optarg, 1);
			break;
		case OPTION_SSL_NO_SSL3:
			setenv("SSL_NO_SSL3", "", 1);
			break;
		case OPTION_SSL_NO_TLS1:
			setenv("SSL_NO_TLS1", "", 1);
			break;
		case OPTION_SSL_NO_VERIFY_HOSTNAME:
			setenv("SSL_NO_VERIFY_HOSTNAME", "", 1);
			break;
		case OPTION_SSL_NO_VERIFY_PEER:
			setenv("SSL_NO_VERIFY_PEER", "", 1);
			break;
		default:
			usage();
			exit(1);
		}

	argc -= optind;
	argv += optind;

	if (h_hostname || f_filename || c_dirname) {
		if (!h_hostname || !f_filename || argc) {
			usage();
			exit(1);
		}
		/* XXX this is a hack. */
		if (strcspn(h_hostname, "@:/") != strlen(h_hostname)) {
			fprintf(stderr, "invalid hostname\n");
			exit(1);
		}
		len = 1 + 8;
		len += strlen(h_hostname);
		len += c_dirname ? strlen(c_dirname) : 0;
		len += strlen(f_filename);
		argv[argc] = (char *) malloc(len);
		if (argv[argc] == NULL) {
			fprintf(stderr,
			  "failed to allocate memory for -f/-h option\n");
			exit (1);
		}
		if (snprintf(argv[argc], len, "ftp://%s/%s/%s",
		    h_hostname,
		    c_dirname ? c_dirname : "",
		    f_filename) < 0) {
			fprintf(stderr, "failed to compose -f/-h option: %s\n",
			  strerror(errno));
			exit (1);
		}
		argc++;
	}

	if (!argc) {
		usage();
		exit(1);
	}

	/* allocate buffer */
	if (B_size < MINBUFSIZE)
		B_size = MINBUFSIZE;
	if ((buf = malloc(B_size)) == NULL) {
		fprintf(stderr, "%s\n", strerror(ENOMEM));
		exit(1);
	}

	/* timeouts */
	if ((s = getenv("FTP_TIMEOUT")) != NULL) {
		ftp_timeout = strtol(s, &end, 10);
		if (*s == '\0' || *end != '\0' || ftp_timeout < 0) {
			fprintf(stderr, "FTP_TIMEOUT (%s) is "
			  "not a positive integer\n", s);
			ftp_timeout = 0;
		}
	}
	if ((s = getenv("HTTP_TIMEOUT")) != NULL) {
		http_timeout = strtol(s, &end, 10);
		if (*s == '\0' || *end != '\0' || http_timeout < 0) {
			fprintf(stderr, "HTTP_TIMEOUT (%s) is "
			  "not a positive integer\n", s);
			http_timeout = 0;
		}
	}

	/* signal handling */
	sa.sa_flags = 0;
	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sa, NULL);
	fetchRestartCalls = 0;

	/* output file */
	if (o_flag) {
		if (strcmp(o_filename, "-") == 0) {
			o_stdout = 1;
		} else if (stat(o_filename, &sb) == -1) {
			if (errno == ENOENT) {
				if (argc > 1) {
					fprintf(stderr,
					  "%s is not a directory\n",
					  o_filename);
					exit(1);
				}
			} else {
				fprintf(stderr, "%s: %s\n", o_filename,
				  strerror(errno));
				exit(1);
			}
		} else {
			if (sb.st_mode & S_IFDIR)
				o_directory = 1;
		}
	}

	/* check if output is to a tty (for progress report) */
	v_tty = isatty(STDERR_FILENO);
	v_progress = v_tty && v_level > 0;
	if (v_progress)
		pgrp = getpgrp();

	r = 0;

	/* authentication */
	if (v_tty)
		fetchAuthMethod = query_auth;
	if (N_filename != NULL)
		if (setenv("NETRC", N_filename, 1) == -1) {
			fprintf(stderr, "setenv: cannot set NETRC=%s: %s\n",
			  N_filename, strerror(errno));
			exit(1);
		}

	while (argc) {
		if ((p = strrchr(*argv, '/')) == NULL)
			p = *argv;
		else
			p++;

		if (!*p)
			p = "fetch.out";

		fetchLastErrCode = 0;

		if (o_flag) {
			if (o_stdout) {
				e = fetch(*argv, "-");
			} else if (o_directory) {
				len = 1 + 1;
				len += strlen(o_filename);
				len += strlen(p);
				q = (char *) malloc(len);
				if (q == NULL) {
					fprintf(stderr,
					  "failed memory alloc at o_flag\n");
					exit(1);
				}
				if (snprintf(q, len, "%s/%s", o_filename, p) <0) {
					free(q);
					fprintf(stderr,
					  "failed q def of o_flag: %s\n",
					  strerror(errno));
					exit(1);
				}
				e = fetch(*argv, q);
				free(q);
			} else {
				e = fetch(*argv, o_filename);
			}
		} else {
			e = fetch(*argv, p);
		}

		if (sigint)
			kill(getpid(), SIGINT);

		if (e == 0 && once_flag)
			exit(0);

		if (e) {
			r = 1;
			if ((fetchLastErrCode
			    && fetchLastErrCode != FETCH_UNAVAIL
			    && fetchLastErrCode != FETCH_MOVED
			    && fetchLastErrCode != FETCH_URL
			    && fetchLastErrCode != FETCH_RESOLV
			    && fetchLastErrCode != FETCH_UNKNOWN)) {
				if (w_secs && v_level)
					fprintf(stderr, "Waiting %ld seconds "
					    "before retrying\n", w_secs);
				if (w_secs)
					sleep(w_secs);
				if (a_flag)
					continue;
			}
		}

		argc--, argv++;
	}

	exit(r);
}
