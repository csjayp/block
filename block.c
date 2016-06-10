/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <csjp@sqrt.ca> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Christian SJ Peron
 * ----------------------------------------------------------------------------
 *
 * This utilily [attempts] meets U.S. DoD 5220.22-M Standard for Disk Sanitization.
 *
 */
#include <sys/types.h>

#ifdef __FreeBSD__
#include <libgeom.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static off_t blocksize = 4096;
off_t cbytes, tbytes;
static int npasses = 1;
static int verbose;
static int rflag;
static off_t dsize;
char fmt[32];

#define P_PROGRESS() do {						\
	cbytes += blocksize;						\
	if ((k++ % 100) == 0 || cbytes == tbytes) {			\
		printf(" %s pattern: 0x%02X %ju/%ju Mb (%.0f%%) complete\r",	\
		    (p % 2) == 0 ? "*" : " ",			\
		    b, cbytes / (1024 * 1024), tbytes / (1024 *1024),	\
		    100 * ((float)cbytes / (float)tbytes));		\
		fflush(stdout);						\
		p++;							\
	}								\
} while (0)

static off_t
get_disk_size(char *dev)
{
#ifdef __FreeBSD__
	int gfd;
	off_t sz;

	gfd = g_open(dev, 0);
	if (gfd < 0)
		err(1, "g_open: %s", dev);
	sz = g_mediasize(gfd);
	(void) close(gfd);
	return (sz);
#else
	(void) fprintf(stderr, "Not supported on this OS yet\n");
	exit(1);
	/* NOT REACHED */
#endif
}

int
main(int argc, char *argv [])
{
	unsigned char *block1, *block2, *block3, *block4, *rblock;
	int rfd, fd, error, j, ch, k, p;
	u_int64_t bc;
	u_char b;

	while ((ch = getopt(argc, argv, "b:vn:r")) != -1)
		switch (ch) {
		case 'b':
			blocksize = atoi(optarg);
			break;
		case 'n':
			npasses = atoi(optarg);
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			verbose++;
			break;
		}
	argc -= optind;
	argv += optind;
	dsize = get_disk_size(argv[0]);
	tbytes = npasses * (rflag ? 6 : 4) * dsize;
	if (argc != 1) {
		(void) fprintf(stderr, "Usage: diskwipe [-v] [-b <blocksize>k] </dev/disk>]\n");
		(void) fprintf(stderr, "       -v   Be verbose\n");
		(void) fprintf(stderr, "       -n   Specify the number of passes\n");
		(void) fprintf(stderr, "       -r   Include random writes in each pass\n");
		(void) fprintf(stderr, "       -b   Specify block size in 'k' kilobytes\n");
		exit(1);
	}
	if (rflag) {
		rfd = open("/dev/random", O_RDONLY);
		if (rfd < 0) {
			(void) fprintf(stderr, "/dev/random: %s\n", strerror(errno));
			exit (1);
		}
		rblock = malloc(blocksize);
		if (rblock == NULL) {
			(void) fprintf(stderr, "malloc: %s\n", strerror(errno));
			exit(1);
		}
	}
	block1 = malloc(blocksize);
	block2 = malloc(blocksize);
	block3 = malloc(blocksize);
	block4 = malloc(blocksize);
	if (block1 == NULL || block2 == NULL || block3 == NULL || block4 == NULL) {
		(void) fprintf(stderr, "malloc: %s\n", strerror(errno));
		exit(1);
	}
	memset(&block1[0], 0xAA, blocksize);
	memset(&block2[0], 0x55, blocksize);
	memset(&block3[0], 0x0A, blocksize);
	memset(&block4[0], 0xF5, blocksize);
	fd = open(argv[0], O_RDWR|O_CREAT|O_TRUNC);
	if (fd < 0) {
		(void) fprintf(stderr, "%s: %s\n", argv[0],
		    strerror(errno));
		exit(1);
	}
	cbytes = 0;
	for (j = 0; j < npasses; j++) {
		if (verbose)
			(void) fprintf(stdout, "Initializing pass number %d\n",
			    j + 1);
		if (lseek(fd, 0UL, SEEK_SET) < 0) {
			(void) fprintf(stderr, "lseek\n");
			exit(1);
		}
		if (verbose) {
			(void) fprintf(stdout, "  Writing pattern 0xAA...");
			fflush(stdout);
		}
		b = block1[0];
		for (p = 0, k = 0, bc = 0;;bc += blocksize) {
			error = write(fd, &block1[0], blocksize);
			if (error == 0)
				break;
			else if (error < 0 && errno == EIO)
				continue;
			else if (error < 0) {
				(void) fprintf(stderr, "Write: %s\n",
				    strerror(errno));
				exit(1);
			}
			P_PROGRESS();
		}
		if (verbose)
			(void) fprintf(stdout, "%ju bytes written\n", bc);
                if (lseek(fd, 0UL, SEEK_SET) < 0) {
                        (void) fprintf(stderr, "lseek\n");
                        exit(1);
                }
		if (verbose) {
			(void) fprintf(stdout, "  Writing pattern 0x55...");
			fflush(stdout);
		}
		b = block2[0];
		for (bc = 0;; bc += blocksize) {
                        error = write(fd, &block2[0], blocksize);
                        if (error == 0)
                                break; 
                        else if (error < 0 && errno == EIO)
                                continue;
                        else if (error < 0) {
                                (void) fprintf(stderr, "Write: %s\n",
                                    strerror(errno));
                                exit(1);
                        }
			P_PROGRESS();
		}
		if (verbose)
			(void) fprintf(stdout, "%ju bytes written\n", bc);
                if (lseek(fd, 0UL, SEEK_SET) < 0) {
                        (void) fprintf(stderr, "lseek\n");
                        exit(1);
                }
		if (rflag) {
			if (verbose)
				(void) fprintf(stdout,
				    "  Writing random data\n");
			for (bc = 0; ; bc += blocksize) {
				if (read(rfd, rblock, blocksize) < 0) {
					(void) fprintf(stderr, "could not read random bytes\n");
					exit(1);
				}
				b = rblock[0];
				error = write(fd, rblock, blocksize);
				if (error == 0)
					break;
				else if (error < 0 && errno == EIO)
					continue;
				else if (error < 0) {
					(void) fprintf(stderr, "Write: %s\n",
					    strerror(errno));
					exit(1);
				}
				P_PROGRESS();
			}
			if (verbose)
				(void) fprintf(stdout, "%ju bytes written\n", bc);
			if (lseek(fd, 0UL, SEEK_SET) < 0) {
				(void) fprintf(stderr, "lseek\n");
				exit(1);
			}
		}
		if (verbose) {
			(void) fprintf(stdout, "  Writing pattern 0x0A...");
			fflush(stdout);
		}
		b = block3[0];
		for (bc = 0;; bc += blocksize) {
                        error = write(fd, &block3[0], blocksize);
                        if (error == 0)
                                break; 
                        else if (error < 0 && errno == EIO)
                                continue;
                        else if (error < 0) {
                                (void) fprintf(stderr, "Write: %s\n",
                                    strerror(errno));
                                exit(1);
                        }
			P_PROGRESS();
		}
		if (verbose)
			(void) fprintf(stdout, "%ju bytes written\n", bc);
                if (lseek(fd, 0UL, SEEK_SET) < 0) {
                        (void) fprintf(stderr, "lseek\n");
                        exit(1);
                }
		if (verbose) {
			(void) fprintf(stdout, "  Writing pattern 0xF5...");
			fflush(stdout);
		}
		b = block4[0];
		for (bc = 0;; bc += blocksize) {
                        error = write(fd, &block4[0], blocksize);
                        if (error == 0)
                                break; 
                        else if (error < 0 && errno == EIO)
                                continue;
                        else if (error < 0) {
                                (void) fprintf(stderr, "Write: %s\n",
                                    strerror(errno));
                                exit(1);
                        }
			P_PROGRESS();
		}
		if (lseek(fd, 0UL, SEEK_SET) < 0) {
			(void) fprintf(stderr, "lseek\n");
			exit(1);
		}
		if (verbose)
			(void) fprintf(stdout, "%ju bytes written\n", bc);
		if (rflag) {
			if (verbose)
				(void) fprintf(stdout,
				    "  Writing random data\n");
			for (bc = 0; ; bc += blocksize) {
				if (read(rfd, rblock, blocksize) < 0) {
					(void) fprintf(stderr, "could not read random bytes\n");
					exit(1);
				}
				b = rblock[0];
				error = write(fd, rblock, blocksize);
				if (error == 0)
					break;
				else if (error < 0 && errno == EIO)
					continue;
				else if (error < 0) {
					(void) fprintf(stderr, "Write: %s\n",
					    strerror(errno));
					exit(1);
				}
				P_PROGRESS();
			}
			if (verbose)
				(void) fprintf(stdout, "%ju bytes written\n", bc);
			if (lseek(fd, 0UL, SEEK_SET) < 0) {
				(void) fprintf(stderr, "lseek\n");
				exit(1);
			}
		}
	}
	P_PROGRESS();
	putchar('\n');
	return (0);
}
