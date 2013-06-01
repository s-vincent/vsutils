/*
 * Copyright (C) 2008-2013 Sebastien Vincent.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * This product includes software developed by the OpenSSL Project
 * for use in the OpenSSL Toolkit (http://www.openssl.org/).
 */

/**
 * \file tls_peer.h
 * \brief TLS and DTLS peer implementation.
 * \author Sebastien Vincent
 * \date 2008-2013
 */

#ifndef VSUTILS_TLS_PEER_H
#define VSUTILS_TLS_PEER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>

#ifdef _MSC_VER
typedef int ssize_t;
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "list.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \def LIBSSL_INIT
 * \brief Initialize libssl.
 * \note You have to call it before using tls_peer functions.
 */
#define LIBSSL_INIT {SSL_library_init(); OpenSSL_add_all_algorithms(); SSL_load_error_strings(); ERR_load_crypto_strings(); }while(0)

/**
 * \def LIBSSL_CLEANUP
 * \brief Cleanup libssl.
 * \note You have to call it when your program exit.
 * \note It is normal if your program still leaks 48 bytes on x86
 * or 88 bytes on x86_64 due to libssl.
 */
#define LIBSSL_CLEANUP {EVP_cleanup(); ERR_remove_state(0); ERR_free_strings(); CRYPTO_cleanup_all_ex_data(); }while(0)

/**
 * \struct tls_peer
 * \brief Describes a TLS/DTLS peer.
 */
struct tls_peer
{
  enum protocol_type type; /**< Transport protocol used (TCP or UDP). */
  int sock; /**< Server socket descriptor. */
  SSL_CTX* ctx_client; /**< SSL context for client side. */
  SSL_CTX* ctx_server; /**< SSL context for server side. */
  struct list_head remote_peers; /**< Remote peers. */
  BIO* bio_fake; /**< Fake BIO for read operations. */
  int (*verify_callback)(int, X509_STORE_CTX *); /**< Verification callback. */
};

/**
 * \brief Create a new TLS/DTLS peer.
 * \param type transport protocol (TCP or UDP).
 * \param addr Network address or FQDN.
 * \param port listen port.
 * \param ca_file Certification Authority file.
 * \param cert_file certificate file.
 * \param key_file private key file.
 * \param verify_callback callback to verify certificate.
 * \return valid pointer on tls_peer or NULL if failure.
 */
struct tls_peer* tls_peer_new(enum protocol_type type, const char* addr,
    uint16_t port, const char* ca_file, const char* cert_file,
    const char* key_file, int (*verify_callback)(int, X509_STORE_CTX *));

/**
 * \brief Free a TLS/DTLS peer.
 * \param peer pointer on tls_peer instance (create by tls_peer_new).
 */
void tls_peer_free(struct tls_peer** peer);

/**
 * \brief Write a message using TLS/DTLS.
 * \param peer TLS/DTLS peer instance.
 * \param buf buffer to send.
 * \param buflen buffer length.
 * \param addr destination address.
 * \param addrlen sizeof address.
 * \return bytes sent or -1 if error(s).
 */
ssize_t tls_peer_write(struct tls_peer* peer, const char* buf, ssize_t buflen,
    const struct sockaddr* addr, socklen_t addrlen);

/**
 * \brief Read a message using TLS for TCP use only.
 * \param peer TLS/DTLS peer instance.
 * \param buf buffer that contains the data from recv/recvfrom.
 * \param buflen buffer length.
 * \param bufout out buffer that will receive the data.
 * \param bufoutlen out buffer length.
 * \param addr source address.
 * \param addrlen sizeof address.
 * \param sock the freshly accept()ed socket descriptor.
 * \return bytes sent or -1 if error(s).
 * \note Before calling this function, the caller must have recv() data.
 * \warning TCP use only!
 */
ssize_t tls_peer_tcp_read(struct tls_peer* peer, char* buf, ssize_t buflen,
    char* bufout, ssize_t bufoutlen, const struct sockaddr* addr,
    socklen_t addrlen, int sock);

/**
 * \brief Read a message using TLS for UDP use only.
 * \param peer TLS/DTLS peer instance.
 * \param buf buffer that contains the data from recv/recvfrom.
 * \param buflen buffer length.
 * \param bufout out buffer that will receive the data.
 * \param bufoutlen out buffer length.
 * \param addr source address.
 * \param addrlen sizeof address.
 * \return bytes sent or -1 if error(s).
 * \note Before calling this function, the caller must have recvfrom() data.
 * \warning UDP use only!
 */
ssize_t tls_peer_udp_read(struct tls_peer* peer, char* buf, ssize_t buflen,
    char* bufout, ssize_t bufoutlen, const struct sockaddr* addr,
    socklen_t addrlen);

/**
 * \brief Do the TLS/DTLS handshake.
 * \param peer TLS/DTLS peer instance.
 * \param daddr destination sockaddr.
 * \param daddr_size sockaddr size.
 * \return 0 if success, -1 otherwise.
 */
int tls_peer_do_handshake(struct tls_peer* peer, const struct sockaddr* daddr,
    socklen_t daddr_size);

/**
 * \brief Print the connection informations.
 * \param peer TLS/DTLS peer.
 */
void tls_peer_print_connection(struct tls_peer* peer);

/**
 * \brief If the frame is TLS/DTLS encrypted or not.
 * \param buf transport payload.
 * \param len payload length.
 * \return 1 if the payload is TLS/DTLS, 0 otherwise.
 */
int tls_peer_is_encrypted(const char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_TLS_PEER_H */

