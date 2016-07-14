CC=gcc
CFLAGS=-std=c99 -c -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls -Wshadow -pedantic -fno-strict-aliasing -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -O2 -I./include/vsutils
LDFLAGS=-lpthread -lrt -lcrypto -lssl -lOpenCL
SOURCES=$(wildcard src/*c)
OBJ=$(SOURCES:.c=.o)
TESTS= test_bitfield test_list test_thread_dispatcher test_netevt test_mq_posix test_mq_sysv test_shm_posix test_shm_sysv test_sem_posix test_sem_sysv

all: $(OBJ)
	
tests: $(TESTS) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

test_bitfield: $(OBJ) tests/test_bitfield.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_list: $(OBJ) tests/test_list.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_thread_dispatcher: $(OBJ) tests/test_thread_dispatcher.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_netevt: $(OBJ) tests/test_netevt.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_mq_posix: $(OBJ) tests/test_mq_posix.o tests/test_mq_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_mq_sysv: $(OBJ) tests/test_mq_sysv.o tests/test_mq_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_shm_posix: $(OBJ) tests/test_shm_posix.o tests/test_shm_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_shm_sysv: $(OBJ) tests/test_shm_sysv.o tests/test_shm_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_sem_posix: $(OBJ) tests/test_sem_posix.o tests/test_sem_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

test_sem_sysv: $(OBJ) tests/test_sem_sysv.o tests/test_sem_common.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o tests/*.o $(TESTS)
	rm -rf doc/html

