VSUtils
=======

VSUtils is a collection of helper functions related to system and network
written in C language.

It includes:
- Network event-driven helper (select, poll, ...);
- IPC helper for POSIX/System V (ipc_mq_*.[ch], ipc_shm_*.[ch], ipc_sem_*.[ch]);
- Socket and IP address helper (util_net.[ch]);
- Endianess tester, daemon/service, converter, ... (util_sys.[ch]);
- TLS and DTLS socket wrapper based on OpenSSL implementation (tls_peer.[ch]);
- Some crypto related functions based on OpenSSL (util_crypto.[ch]);
- Bitfield implementation (bitfield.[ch]);
- Debug related functions (dbg.[ch]);
- Doubly linked list (list.h);
- Some #define to determine the OS (os.h).

