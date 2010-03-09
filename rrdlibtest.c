#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dlfcn.h>
#include "rrd.h"

/*
 * this proggy shows the corruption under rh9 using shared libs...with
 * rrdtool-1.0.42 (http://www.rrdtool.org)
 * 
 * You can compile this program 3 ways: static, dynamically linked to librrd.so
 * and dynamically loaded via dlopen(). Errors occur with either of the
 * dynamicly linked versions.
 * 
 * Specifically...
 * 
 * when setting MALLOC_CHECK_=1 and runnng dynamic/dlopen you will see 'free():
 * invalid pointer' warnings
 * 
 * when UNsetting MALLOC_CHECK_ and running valgrind (http://valgrind.kde.org/),
 * all versions work
 * 
 * when UNsetting MALLOC_CHECK_ and runnng dynamic/dlopen you will see these
 * hang, straces report hanging on a futex:
 * 
 * open("txbytes.rrd", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3 fstat64(3,
 * {st_mode=S_IFREG|0664, st_size=0, ...}) = 0 mmap2(NULL, 4096,
 * PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x40017000
 * write(3, "RRD\0000001\0\0\0\0/%\300\307C+\37[\1\0\0\0\2\0\0\0\n\0"...,
 * 1260) = 1260 close(3) = 0 munmap(0x40017000, 4096) = 0 time(NULL) =
 * 1068744414 open("txbytes.rrd", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
 * fstat64(3, {st_mode=S_IFREG|0664, st_size=0, ...}) = 0 mmap2(NULL, 4096,
 * PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x40017000
 * futex(0x8049cc0, FUTEX_WAIT, -127, NULL <unfinished ...>
 * 
 * running a DIFFERENT program (the one that started me on this bug hunt, aware
 * (http://www.elegant-software.com/software/aware),which uses pthreads)
 * under valgrind I see:
 * 
 * ==24437== pthread_mutex_lock/trylock: mutex is invalid ==24437== at
 * 0x40227B10: __pthread_mutex_lock (vg_libpthread.c:951) ==24437== by
 * 0x4022B287: _IO_flockfile (vg_libpthread.c:3130) ==24437== by 0x402D0F92:
 * _IO_un_link_internal (in /lib/libc-2.3.2.so) ==24437== by 0x402CD8E8:
 * _IO_fclose@xxxxxxxxx (in /lib/libc-2.3.2.so) ==24437==
 * 
 * running the above DIFFERENT program WITHOUT valgrind gets a segfault when
 * executing rrd_create() via dlopen() linking.
 * 
 * 
 */

/*
 * first convert librrd.a to a .so:
 * 
 * ar x librrd.a ld -shared -soname=librrd.so -o ./librrd.so *.o
 * 
 */

/*
 * static compile: gcc -static -O2 -g -I/usr/local/rrdtool-1.0.42/include
 * debugtest.c -o debugtest -L/usr/local/rrdtool-1.0.42/lib -lrrd
 */
// NO CORRUPTION !

/*
 * dynamic compile: gcc -O2 -g -I/usr/local/rrdtool-1.0.42/include
 * debugtest.c -o debugtest -L/usr/local/rrdtool-1.0.42/lib -lrrd -lm
 */
//CORRUPTION !

/*
 * ldopen compile: gcc -DLDOPEN -O2 -g -rdynamic
 * -I/usr/local/rrdtool-1.0.42/include debugtest.c -o debugtest -ldl
 * -L/usr/local/rrdtool-1.0.42/lib -lrrd -lm
 */
//CORRUPTION !

/* before running, export MALLOC_CHECK_=1 */

/*
 * before running, export
 * LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/rrdtool-1.0.42/lib
 */

/* to run, do gdb debugtest */

void 
main()
{
	char           *argv[] = {
		"create",
		"txbytes.rrd",
		"-s",
		"10",
		"DS:bytes:GAUGE:30:U:U",
		"RRA:AVERAGE:0.5:1:6",
		"RRA:AVERAGE:0.5:6:60"
	};
	int
	                counter = 100, argc = 7;

#ifndef LDOPEN
	while (counter--)
		rrd_create(argc, argv);
#else
	{
		void
		               *sohandle = dlopen("librrd.so", RTLD_NOW | RTLD_GLOBAL);
		int
		                (*rrdcreate_function) (int, char **);

		if (sohandle == NULL) {
			perror("rrd.so");
			exit(-1);
		}
		/* lookup constructor */
		rrdcreate_function = dlsym(sohandle, "rrd_create");
		if (rrdcreate_function == NULL) {
			fprintf(stderr, "Unable to find rrd_create\n");
			exit(-1);
		}		/* end if */
		while (counter--)
			(*rrdcreate_function) (argc, argv);

	}

#endif


}
// gcc -o tmp tmp.c -L/opt/local/lib/ -lrrd
