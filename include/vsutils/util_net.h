/*
 * Copyright (C) 2013-2017 Sebastien Vincent.
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
 */

/**
 * \file util_net.h
 * \brief Some helper network functions.
 * \author Sebastien Vincent
 * \date 2013-2019
 */

#ifndef VSUTILS_UTIL_NET
#define VSUTILS_UTIL_NET

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>

#include <sys/uio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \enum address_family
 * \brief Address family.
 */
enum address_family
{
  NET_UNSPEC = AF_UNSPEC, /**< Any family. */
  NET_IPV4 = AF_INET, /**< IPv4 family. */
  NET_IPV6 = AF_INET6 /**< IPv6 family. */
};

/**
 * \enum protocol_type
 * \brief Transport protocol.
 */
enum protocol_type
{
  NET_UDP = IPPROTO_UDP, /**< UDP protocol. */
  NET_TCP = IPPROTO_TCP, /**< TCP protocol. */
};

/**
 * \brief Network interface description.
 */
struct net_iface
{
  /**
   * \brief Interface index.
   */
  unsigned int ifindex;

  /**
   * \brief Interface name.
   */
  char ifname[IF_NAMESIZE];

  /**
   * \brief Interface link-layer address.
   */
  char ifaddr[16];
};

/**
 * \brief Specific cast for FD_* macro.
 * \def NET_SFD_CAST
 */
#ifdef __MACH__
#define NET_SFD_CAST (fd_set*)
#else
#define NET_SFD_CAST
#endif

/* to specify a user-defined FD_SETSIZE */
#ifndef NET_SFD_SETSIZE
/**
 * \def NET_SFD_SETSIZE
 * \brief User defined FD_SETSIZE.
 */
#define NET_SFD_SETSIZE FD_SETSIZE
#endif

/**
 * \struct net_sfd_set
 * \brief An fd_set-like structure.
 *
 * Replacement for the classic fd_set.
 * Ensure that select() can manage the maximum open files
 * on a system.
 */
struct net_sfd_set
{
  long int fds_bits[NET_SFD_SETSIZE / (8 * sizeof(long int))]; /**< Bitmask. */

  /**
   * \def __fds_bits
   * \brief Definition of __fds_bits for *BSD.
   */
#define __fds_bits fds_bits
};

/**
 * \typedef sfd_set
 * \brief Short name for struct net_sfd_set.
 */
typedef struct net_sfd_set sfd_set;

/**
 * \def NET_SFD_ZERO
 * \brief FD_ZERO wrapper.
 */
#define NET_SFD_ZERO(set) memset((set), 0x00, sizeof(sfd_set))

/**
 * \def NET_SFD_SET
 * \brief FD_SET wrapper.
 */
#define NET_SFD_SET(fd, set) FD_SET((fd), NET_SFD_CAST(set))

/**
 * \def NET_SFD_ISSET
 * \brief FD_ISSET wrapper.
 */
#define NET_SFD_ISSET(fd, set) FD_ISSET((fd), NET_SFD_CAST(set))

/**
 * \def NET_SFD_CLR
 * \brief FD_CLR wrapper.
 */
#define NET_SFD_CLR(fd, set) FD_CLR((fd), NET_SFD_CAST(set))

/**
 * \brief Create and bind socket.
 * \param af address family.
 * \param protocol transport protocol used.
 * \param addr address or FQDN name.
 * \param port to bind.
 * \param v6only accept socket to accept both IPv4 and IPv6.
 * \param reuse allow socket to reuse transport address (SO_REUSE).
 * \return socket descriptor, -1 otherwise (check errno to know the reason).
 */
int net_socket_create(enum address_family af, enum protocol_type protocol,
    const char* addr, uint16_t port, int v6only, int reuse);

/**
 * \brief Free elements of an iovec array.
 * It does not freed the array (if allocated).
 * \param iov the iovec array.
 * \param nb number of elements.
 */
void net_iovec_free_data(struct iovec* iov, size_t nb);

/**
 * \brief Encode string for HTTP request.
 * \param str string to encode.
 * \return encoding string or NULL if problem.
 * \warning The caller must free the return value after use.
 */
char* net_encode_http_string(const char* str);

/**
 * \brief The writev() socket helper function.
 * \param fd the socket descriptor to write the data.
 * \param iov the iovector which contains the data.
 * \param iovcnt number of element that should be written.
 * \param addr source address to send with UDP, set to NULL for TCP.
 * \param addr_size sizeof addr.
 * \return number of bytes written or -1 if error (check errno to know the
 * reason).
 * \warning this function work only with socket!
 */
ssize_t net_sock_writev(int fd, const struct iovec *iov, size_t iovcnt,
    const struct sockaddr* addr, socklen_t addr_size);

/**
 * \brief The readv() socket helper function.
 * \param fd the socket descriptor to read the data.
 * \param iov the iovector to store the data.
 * \param iovcnt number of element that should be filled.
 * \param addr destination address for UDP socket. Use NULL for TCP socket.
 * \param addr_size pointer on address size, will be filled by this function.
 * \return number of bytes read or -1 if error (check errno to know the reason).
 * \warning this function work only with socket!
 */
ssize_t net_sock_readv(int fd, const struct iovec *iov, size_t iovcnt,
    const struct sockaddr* addr, socklen_t* addr_size);

/**
 * \brief Returns whether or not socket has been triggered for an event.
 *
 * It is a convenient function to test if socket is ready for read or
 * write operation (depending of fds parameter).
 * \param sock socket to check.
 * \param nsock parameter of (p)select() function.
 * \param fds set of descriptor (see select()) to check.
 * \return 1 if socket has data, 0 otherwise.
 */
static inline int net_sfd_is_ready(int sock, int nsock, sfd_set* fds)
{
  return (sock > 0 && sock < nsock && NET_SFD_ISSET(sock, fds)) ? 1 : 0;
}

/**
 * \brief Constructs a sockaddr from FQDN or address string and port.
 * \param family AF_INET for IPv4 or AF_INET6 for IPv6.
 * \param address FQDN or address string.
 * \param port port.
 * \param addr sockaddr_storage pointer which will be filled with result.
 * \param addr_size size of the sockaddr_storage.
 * \return 0 if success, -1 otherwise.
 */
int net_sockaddr_make(int family, const char* address, uint16_t port,
    struct sockaddr_storage* addr, socklen_t* addr_size);

/**
 * \brief Returns socket address length.
 * \param addr socket address.
 * \return socket address length or 0 if failure.
 */
socklen_t net_sockaddr_len(const struct sockaddr_storage* addr);

/**
 * \brief Converts address in string format and port in host order.
 * \param addr sockaddr storage
 * \param str if not NULL, converted-string address will be filled in.
 * \param str_size size of addr parameter.
 * \param port if not NULL, port will be filled in.
 * \return 0 if success, -1 otherwise.
 */
int net_sockaddr_str(const struct sockaddr_storage* addr, char* str,
    socklen_t str_size, uint16_t* port);

/**
 * \brief Returns whether or not the address is a valid address (IPv4 or IPv6).
 * \param address address to test.
 * \return 1 if address is a valid address, 0 otherwise.
 */
int net_address_is_valid(const char* address);

/**
 * \brief Returns whether or not the address is a valid IPv4 address.
 * \param address address to test.
 * \return 1 if address is a valid IPv4 address, 0 otherwise.
 */
int net_ipv4_address_is_valid(const char* address);

/**
 * \brief Returns whether or not the address is a valid IPv6 address.
 * \param address address to test.
 * \return 1 if address is a valid IPv6 address, 0 otherwise.
 */
int net_ipv6_address_is_valid(const char* address);

/**
 * \brief Returns whether or not the address is an IPv6 tunneled ones (6to4 or
 * teredo).
 * \param addr address to test.
 * \return 1 if the address is an IPv6 tunneled ones, 0 otherwise.
 */
int net_ipv6_address_is_tunneled(const struct in6_addr* addr);

/**
 * \brief Converts an IPv6 prefix length struct in6_addr.
 * \param prefix prefix length.
 * \param addr address that will be filled.
 * \return 0 if success, -1 otherwise.
 */
int net_ipv6_netmask_get_prefix(unsigned int prefix, struct in6_addr* addr);

/**
 * \brief Converts an IPv6 netmask into prefix length.
 * \param addr IPv6 netmask.
 * \return prefix prefix length.
 */
unsigned int net_ipv6_netmask_get_prefix_length(struct in6_addr* addr);

/**
 * \brief Returns list of IPv4 addresses for an interface.
 * \param ifindex interface index.
 * \param addrs pointer that will be allocated to hold IPv4 addresses.
 * \return number of IPv4 addresses returned or -1 if error.
 */
int net_ipv4_get_addresses(int ifindex, struct in_addr** addrs);

/**
 * \brief Adds an IPv4 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv4 address.
 * \param prefix IPv4 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_add_address(int ifindex, const struct in_addr* addr,
    unsigned int prefix);

/**
 * \brief Adds an IPv4 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv4 address.
 * \param prefix IPv4 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_add_address_str(int ifindex, const char* addr,
    unsigned int prefix);

/**
 * \brief Removes an IPv4 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv4 address.
 * \param prefix IPv4 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_del_address(int ifindex, const struct in_addr* addr,
    unsigned int prefix);

/**
 * \brief Removes an IPv4 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv4 address.
 * \param prefix IPv4 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_del_address_str(int ifindex, const char* addr,
    unsigned int prefix);

/**
 * \brief Adds an IPv4 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv4 route.
 * \param prefix IPv4 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_add_route(int ifindex, const struct in_addr* dst,
    unsigned int prefix, const struct in_addr* gw);

/**
 * \brief Adds an IPv4 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv4 route.
 * \param prefix IPv4 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_add_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw);

/**
 * \brief Removes an IPv4 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv4 route.
 * \param prefix IPv4 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_del_route(int ifindex, const struct in_addr* dst,
    unsigned int prefix, const struct in_addr* gw);

/**
 * \brief Removes an IPv4 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv4 route.
 * \param prefix IPv4 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv4_del_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw);

/**
 * \brief Returns list of IPv6 addresses for an interface.
 * \param ifindex interface index.
 * \param addrs pointer that will be allocated to hold IPv6 addresses.
 * \return number of IPv6 addresses returned or -1 if error.
 */
int net_ipv6_get_addresses(int ifindex, struct in6_addr** addrs);

/**
 * \brief Adds an IPv6 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv6 address.
 * \param prefix IPv6 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_add_address(int ifindex, const struct in6_addr* addr,
    unsigned int prefix);

/**
 * \brief Adds an IPv6 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv6 address.
 * \param prefix IPv6 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_add_address_str(int ifindex, const char* addr,
    unsigned int prefix);

/**
 * \brief Removes an IPv6 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv6 address.
 * \param prefix IPv6 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_del_address(int ifindex, const struct in6_addr* addr,
    unsigned int prefix);

/**
 * \brief Removes an IPv6 address to an interface.
 * \param ifindex interface index.
 * \param addr IPv6 address.
 * \param prefix IPv6 prefix.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_del_address_str(int ifindex, const char* addr,
    unsigned int prefix);

/**
 * \brief Adds an IPv6 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv6 route.
 * \param prefix IPv6 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_add_route(int ifindex, const struct in6_addr* dst,
    unsigned int prefix, const struct in6_addr* gw);

/**
 * \brief Adds an IPv6 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv6 route.
 * \param prefix IPv6 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_add_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw);

/**
 * \brief Removes an IPv6 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv6 route.
 * \param prefix IPv6 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_del_route(int ifindex, const struct in6_addr* dst,
    unsigned int prefix, const struct in6_addr* gw);

/**
 * \brief Removes an IPv6 route to an interface.
 * \param ifindex interface index.
 * \param dst IPv6 route.
 * \param prefix IPv6 prefix.
 * \param gw gateway if any.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_ipv6_del_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw);

/**
 * \brief Joins an IPv4 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv4 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_join_mcast(int fd, int ifindex, const struct in_addr* group);

/**
 * \brief Joins an IPv4 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv4 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_join_mcast_str(int fd, int ifindex, const char* group);

/**
 * \brief Leaves an IPv4 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv4 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_leave_mcast(int fd, int ifindex, const struct in_addr* group);

/**
 * \brief Leaves an IPv4 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv4 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_leave_mcast_str(int fd, int ifindex, const char* group);

/**
 * \brief Joins an IPv6 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv6 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_join_mcast6(int fd, int ifindex, const struct in6_addr* group);

/**
 * \brief Joins an IPv6 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv6 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_join_mcast6_str(int fd, int ifindex, const char* group);

/**
 * \brief Leaves an IPv6 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv6 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_leave_mcast6(int fd, int ifindex, const struct in6_addr* group);

/**
 * \brief Leaves an IPv6 multicast group.
 * \param fd socket descriptor.
 * \param ifindex interface index.
 * \param group IPv6 multicast group.
 * \return 0 if success, -1 otherwise (check errno for reason).
 */
int net_sock_leave_mcast6_str(int fd, int ifindex, const char* group);

/**
 * \brief Returns list of network interfaces.
 * \param ifaces pointer that will be allocated.
 * \return number of interfaces returned if success, -1 otherwise.
 * \note Caller is responsible to free() ifaces parameter.
 */
int net_iface_list(struct net_iface** ifaces);

/**
 * \brief Returns whether network interface has flag enable.
 * \param ifindex interface index.
 * \param flag flag to test.
 * \return 1 if flag is set, 0 if not set, -1 if error.
 */
int net_iface_get_flag(int ifindex, int flag);

/**
 * \brief Returns whether network interface has flag enable.
 * \param ifindex interface index.
 * \param flag flag to test.
 * \param enable 1 to enable, 0 to disable.
 * \return 0 if success, -1 otherwise.
 */
int net_iface_set_flag(int ifindex, int flag, int enable);

/**
 * \brief Returns whether or not network interface is UP.
 * \param ifindex interface index.
 * \return 1 if network interface is UP, 0 if not, -1 if error.
 */
int net_iface_is_up(int ifindex);

/**
 * \brief Sets the network interface UP or DOWN.
 * \param ifindex interface index.
 * \param up 1 to set interface UP, 0 to set interface DOWN.
 * \return 0 if success, -1 otherwise.
 */
int net_iface_set_up(int ifindex, int up);

/**
 * \brief Returns MTU of network interface.
 * \param ifindex interface index.
 * \return MTU of network interface is UP, -1 if error.
 */
int net_iface_get_mtu(int ifindex);

/**
 * \brief Sets the network interface MTU.
 * \param ifindex interface index.
 * \param mtu mtu to set.
 * \return 0 if success, -1 otherwise.
 */
int net_iface_set_mtu(int ifindex, unsigned int mtu);

/**
 * \brief Sets a link-layer address on a network interface.
 * \param ifindex interface index.
 * \param addr link-layer address (in binary).
 * \param size address size in bytes (6 for ethernet).
 * \return 0 if success, -1 otherwise.
 */
int net_iface_set_addr(int ifindex, const char* addr, size_t size);

/**
 * \brief Converts a human-readable ethernet address into link-layer address.
 * \param src human-readable ethernet address.
 * \param dst address.
 * \return 0 if success, -1 otherwise.
 * \note Format of the ethernet address can be :
 * - separated by hyphens (00-00-00-00-00-00);
 * - separeted by colons (00:00:00:00:00:00).
 */
int net_eth_pton(const char* src, void* dst);

/**
 * \brief Converts a link-layer address into human-readable form.
 * \param src link-layer address.
 * \param dst buffer to hold the human-readable form.
 * \param size size of buffer.
 * \return dst if success, NULL otherwise.
 */
char* net_eth_ntop(const void* src, char* dst, socklen_t size);

#ifdef __cplusplus
}
#endif

#endif /* VSUTILS_UTIL_NET */

