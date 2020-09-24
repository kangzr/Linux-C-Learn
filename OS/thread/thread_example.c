#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define handle_error_en(en, msg) \
		do {errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

#define handle_error(msg) \
		do {perror(msg); exit(EXIT_FAILURE); } while(0)

struct thread_info{			/*Used as argument to thread_start()*/
	pthread_t thread_id;	/*ID returned by pthread_create()*/
	int thread_num;			/*App-defeind thead*/
	char *argv_string;		/*From command-line arg*/
}

static void *
thread_start(void *arg)
{
	struct thread_info *tinfo = arg;
	char *uargv, *p;
	printf("Thread %d: top of stack near %p: argv_string=%s\n",
			tinfo->thread_num, &p, tinfo->argv_string);

	uargv = strdup(tinfo->argv_string);
	if (uargv == NULL)
		handle_error("strdup");

	for (p = uargv; *p != '\0'; p++)
		*p = toupper(*p);
	return uargv;
}

int
main(int argc, char *argv[])
{
	int s, tnum, opt, num_threads;
	struct thread_info *tinfo;
	pthread_attr_t attr;
	int stack_size;
	void *res;

	stack_size = -1;
	while ((opt = getopt(argc, argv, "s: ")) != -1) {
		switch (opt) {
			case 's':
				stack_size = strtoul(optarg, NULL, 0);
				break;
			default:
				fprintf(stderr, "Usage: %s [-s stack-size] arg...\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	num_threads = argc - optind;
}
