CFLAGS = -std=c11 -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls -Wshadow -pedantic -pedantic-errors -fno-strict-aliasing -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -O2 -I./include/vsutils
LDFLAGS = -lpthread -lrt -lcrypto -lssl -lOpenCL
SOURCES = src/bitfield.c src/dbg.c src/ipc_mq.c src/ipc_mq_posix.c src/ipc_mq_sysv.c src/ipc_sem.c src/ipc_sem_posix.c src/ipc_sem_sysv.c src/ipc_shm.c src/ipc_shm_posix.c src/ipc_shm_sysv.c src/netevt.c src/netevt_epoll.c src/netevt_kqueue.c src/netevt_poll.c src/netevt_select.c src/thread_dispatcher.c src/thread_pool.c src/util_crypto.c src/util_net.c src/util_opencl.c src/util_sys.c
OBJ = $(SOURCES:.c=.o)
TESTS = test_bitfield test_list test_thread_pool test_thread_dispatcher test_netevt test_mq_posix test_mq_sysv test_shm_posix test_shm_sysv test_sem_posix test_sem_sysv

all: $(OBJ)
	
tests: $(TESTS)

.c.o:
	$(CC) -g -c $(CFLAGS) $< -o $@

test_bitfield: $(OBJ) tests/test_bitfield.o
	$(CC) -o $@ $? $(LDFLAGS)

test_list: $(OBJ) tests/test_list.o
	$(CC) -o $@ $? $(LDFLAGS)

test_thread_pool: $(OBJ) tests/test_thread_pool.o
	$(CC) -o $@ $? $(LDFLAGS)

test_thread_dispatcher: $(OBJ) tests/test_thread_dispatcher.o
	$(CC) -o $@ $? $(LDFLAGS)

test_netevt: $(OBJ) tests/test_netevt.o
	$(CC) -o $@ $? $(LDFLAGS)

test_mq_posix: $(OBJ) tests/test_mq_posix.o tests/test_mq_common.o
	$(CC) -o $@ $? $(LDFLAGS)

test_mq_sysv: $(OBJ) tests/test_mq_sysv.o tests/test_mq_common.o
	$(CC) -o $@ $? $(LDFLAGS)

test_shm_posix: $(OBJ) tests/test_shm_posix.o tests/test_shm_common.o
	$(CC) -o $@ $? $(LDFLAGS)

test_shm_sysv: $(OBJ) tests/test_shm_sysv.o tests/test_shm_common.o
	$(CC) -o $@ $? $(LDFLAGS)

test_sem_posix: $(OBJ) tests/test_sem_posix.o tests/test_sem_common.o
	$(CC) -o $@ $? $(LDFLAGS)

test_sem_sysv: $(OBJ) tests/test_sem_sysv.o tests/test_sem_common.o
	$(CC) -o $@ $? $(LDFLAGS)

doc:
	rm -rf doc/html
	doxygen doc/Doxyfile

clean:
	echo $(OBJ)
	rm -f src/*.o tests/*.o $(TESTS)
	rm -rf doc/html

.PHONY: doc

