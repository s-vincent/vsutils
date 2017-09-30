/*
 * Copyright (C) 2008-2017 Sebastien Vincent.
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
 * \file util_crypto.h
 * \brief Some helper cryptographic functions.
 * \author Sebastien Vincent
 * \date 2008-2017
 */

#ifndef VSUTILS_UTIL_CRYPTO
#define VSUTILS_UTIL_CRYPTO

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Initialize the PRNG.
 * \return 0 if successfull, -1 if seed is cryptographically weak.
 */
int crypto_seed_prng_init(void);

/**
 * \brief Cleanup the PRNG.
 * \return 0 if successfull, -1 if seed is cryptographically weak.
 */
void crypto_seed_prng_cleanup(void);

/**
 * \brief Generate random bytes.
 * \param id buffer that will be filled with random value.
 * \param len length of id.
 * \return 0 if successfull, -1 if the random number is cryptographically weak.
 */
int crypto_random_bytes_generate(uint8_t* id, size_t len);

/**
 * \brief Generate a SHA1 hash.
 * \param hash buffer with at least 20 bytes length.
 * \param text text to hash.
 * \param len text length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_sha1_generate(unsigned char* hash, const unsigned char* text,
    size_t len);

/**
 * \brief Generate a SHA-256 hash.
 * \param hash buffer with at least 32 bytes length.
 * \param text text to hash.
 * \param len text length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_sha256_generate(unsigned char* hash, const unsigned char* text,
    size_t len);

/**
 * \brief Generate a SHA-384 hash.
 * \param hash buffer with at least 48 bytes length.
 * \param text text to hash.
 * \param len text length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_sha384_generate(unsigned char* hash, const unsigned char* text,
    size_t len);

/**
 * \brief Generate a SHA-512 hash.
 * \param hash buffer with at least 64 bytes length.
 * \param text text to hash.
 * \param len text length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_sha512_generate(unsigned char* hash, const unsigned char* text,
    size_t len);

/**
 * \brief Generate a MD5 hash.
 * \param hash buffer with at least 16 bytes length.
 * \param text text to hash.
 * \param len text length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_md5_generate(unsigned char* hash, const unsigned char* text,
    size_t len);

/**
 * \brief Generate a HMAC-SHA1 hash.
 * \param hash buffer with at least 20 bytes length.
 * \param text text to hash.
 * \param text_len text length.
 * \param key key used for HMAC.
 * \param key_len key length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_hmac_sha1_generate(unsigned char* hash, const unsigned char* text,
    size_t text_len, const unsigned char* key, size_t key_len);

/**
 * \brief Generate a HMAC-SHA-256 hash.
 * \param hash buffer with at least 32 bytes length.
 * \param text text to hash.
 * \param text_len text length.
 * \param key key used for HMAC.
 * \param key_len key length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_hmac_sha256_generate(unsigned char* hash, const unsigned char* text,
    size_t text_len, const unsigned char* key, size_t key_len);

/**
 * \brief Generate a HMAC-SHA-384 hash.
 * \param hash buffer with at least 48 bytes length.
 * \param text text to hash.
 * \param text_len text length.
 * \param key key used for HMAC.
 * \param key_len key length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_hmac_sha384_generate(unsigned char* hash, const unsigned char* text,
    size_t text_len, const unsigned char* key, size_t key_len);

/**
 * \brief Generate a HMAC-SHA-512 hash.
 * \param hash buffer with at least 64 bytes length.
 * \param text text to hash.
 * \param text_len text length.
 * \param key key used for HMAC.
 * \param key_len key length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_hmac_sha512_generate(unsigned char* hash, const unsigned char* text,
    size_t text_len, const unsigned char* key, size_t key_len);

/**
 * \brief Generate a HMAC-MD5 hash.
 * \param hash buffer with at least 16 bytes length.
 * \param text text to hash.
 * \param text_len text length.
 * \param key key used for HMAC.
 * \param key_len key length.
 * \return 0 if success, -1 otherwise.
 */
int crypto_hmac_md5_generate(unsigned char* hash, const unsigned char* text,
    size_t text_len, const unsigned char* key, size_t key_len);

/**
 * \brief Generate a CRC-32 (ISO 3309, ITU-T V.42 8.1.1.6.2, RFC 1952).
 * \param data data.
 * \param len length of data.
 * \param prev previous value.
 * \return CRC-32 of data.
 */
uint32_t crypto_crc32_generate(const uint8_t* data, size_t len, uint32_t prev);

/**
 * \brief Print a digest.
 * \param buf buffer.
 * \param len length of buffer.
 */
void crypto_digest_print(const unsigned char* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_UTIL_CRYPTO */

