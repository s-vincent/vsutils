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
 * \file util_net.c
 * \brief Some helper network functions.
 * \author Sebastien Vincent
 * \date 2013-2019
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#if !__BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <net/route.h>
#include <ifaddrs.h>

#ifdef __linux__
#include <linux/if_packet.h>
#include <linux/ipv6.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <net/if_dl.h>
#include <netinet6/in6_var.h>

#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

#include "util_net.h"

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

char* net_encode_http_string(const char* str)
{
  size_t len = strlen(str);
  char* p = NULL;
  size_t j = 0;
  size_t alloc_size = 3 * len + 1;

  if(!len)
  {
    return NULL;
  }

  /* in the worst case, it take 3x (%20) the size */
  p = malloc(sizeof(char) * alloc_size);

  if(!p)
  {
    return NULL;
  }

  for(size_t i = 0 ; i < len ; i++, j++)
  {
    unsigned int t = (unsigned int)str[i];

    if(t < 42 || (t >= 58 && t < 64) ||
       (t >= 91 && t < 95) || t == '`' ||
       t > 122 || t == '+' || t == '&' ||
       t == ',' || t == ';' || t == '/' ||
       t == '?' || t == '@' || t == '$' ||
       t == '=' || t == ':' )
    {
      /* replace */
      snprintf(p + j, alloc_size - j, "%%%02X", t);
      j += 2;
    }
    else
    {
      p[j] = (char)t;
    }
  }

  p[j] = 0x00;

  return p;
}

void net_iovec_free_data(struct iovec* iov, size_t nb)
{
  for(size_t i = 0 ; i < nb ; i++)
  {
    free(iov[i].iov_base);
    iov[i].iov_base = NULL;
  }
}

int net_socket_create(enum address_family af, enum protocol_type protocol,
    const char* addr, uint16_t port, int v6only, int reuse)
{
  int sock = -1;
  struct addrinfo hints;
  struct addrinfo* res = NULL;
  char service[8];

  snprintf(service, sizeof(service), "%u", port);
  service[sizeof(service)-1] = 0x00;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = af;
  hints.ai_socktype = (protocol == NET_TCP ? SOCK_STREAM : SOCK_DGRAM);
  hints.ai_protocol = (protocol == NET_TCP ? IPPROTO_TCP : IPPROTO_UDP);
  hints.ai_flags = AI_PASSIVE;

  if(getaddrinfo(addr, service, &hints, &res) != 0)
  {
    return -1;
  }

  for(struct addrinfo* p = res ; p ; p = p->ai_next)
  {
    int on = 1;

    sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if(sock == -1)
    {
      continue;
    }

    if(reuse)
    {
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
    }

    /* accept IPv6 and IPv4 on the same socket */
    on = v6only ? 1 : 0;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(int));

    if(bind(sock, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sock);
      sock = -1;
      continue;
    }

    /* socket bound, break the loop */
    break;
  }

  freeaddrinfo(res);

  return sock;
}

ssize_t net_sock_readv(int fd, const struct iovec *iov, size_t iovcnt,
    const struct sockaddr* addr, socklen_t* addr_size)
{
  struct msghdr msg;
  ssize_t len = -1;

  memset(&msg, 0x00, sizeof(struct msghdr));
  msg.msg_name = (struct sockaddr*)addr;
  msg.msg_namelen = *addr_size;
  msg.msg_iov = (struct iovec*)iov;
  msg.msg_iovlen = iovcnt;

  len = recvmsg(fd, &msg, 0);
  *addr_size = msg.msg_namelen;

  return len;
}

ssize_t net_sock_writev(int fd, const struct iovec *iov, size_t iovcnt,
    const struct sockaddr* addr, socklen_t addr_size)
{
  struct msghdr msg;

  memset(&msg, 0x00, sizeof(struct msghdr));
  msg.msg_name = (struct sockaddr*)addr;
  msg.msg_namelen = addr_size;
  msg.msg_iov = (struct iovec*)iov;
  msg.msg_iovlen = iovcnt;
  return sendmsg(fd, &msg, 0);
}

int net_sockaddr_make(int family, const char* address, uint16_t port,
    struct sockaddr_storage* addr, socklen_t* addr_size)
{
  struct addrinfo hints;
  struct addrinfo* res = NULL;
  char service[8];
  int ret = 0;

  snprintf(service, sizeof(service), "%u", port);
  service[sizeof(service)-1] = 0x00;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = family;
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;
  hints.ai_flags = 0;

  if(getaddrinfo(address, service, &hints, &res) != 0)
  {
    return -1;
  }

  if(res)
  {
    memcpy(addr, res->ai_addr, res->ai_addrlen);
    *addr_size = res->ai_addrlen;
    freeaddrinfo(res);
    ret = 0;
  }
  else
  {
    ret = -1;
  }

  return ret;
}

socklen_t net_sockaddr_len(const struct sockaddr_storage* addr)
{
  socklen_t ret = 0;

  switch(addr->ss_family)
  {
  case AF_INET:
    ret = sizeof(struct sockaddr_in);
    break;
  case AF_INET6:
    ret = sizeof(struct sockaddr_in6);
    break;
  default:
    ret = 0;
    break;
  }

  return ret;
}

int net_sockaddr_str(const struct sockaddr_storage* addr, char* str,
    socklen_t str_size, uint16_t* port)
{
  int ret = -1;
  struct sockaddr_in* in = NULL;
  struct sockaddr_in6* in6 = NULL;

  switch(addr->ss_family)
  {
  case AF_INET:
    if(str && str_size < INET_ADDRSTRLEN)
    {
      errno = EINVAL;
      return -1;
    }

    in = (struct sockaddr_in*)addr;
    if(str)
    {
      inet_ntop(AF_INET, &in->sin_addr, str, str_size);
    }

    if(port)
    {
      *port = ntohs(in->sin_port);
    }
    break;
  case AF_INET6:
    if(str && str_size < INET6_ADDRSTRLEN)
    {
      errno = EINVAL;
      return -1;
    }

    in6 = (struct sockaddr_in6*)addr;
    if(str)
    {
      inet_ntop(AF_INET6, &in6->sin6_addr, str, str_size);
    }

    if(port)
    {
      *port = ntohs(in6->sin6_port);
    }
    break;
  default:
    ret = -1;
    break;
  }

  return ret;
}

int net_ipv4_address_is_valid(const char* address)
{
  struct in_addr dst;
  return inet_pton(AF_INET, address, &dst);
}

int net_ipv6_address_is_valid(const char* address)
{
  struct in6_addr dst;
  return inet_pton(AF_INET6, address, &dst);
}

int net_address_is_valid(const char* address)
{
  return net_ipv4_address_is_valid(address) ||
    net_ipv6_address_is_valid(address);
}

int net_ipv6_address_is_tunneled(const struct in6_addr* addr)
{
  static const uint8_t addr_6to4[2] = {0x20, 0x02};
  static const uint8_t addr_teredo[4] = {0x20, 0x01, 0x00, 0x00};

  /* 6to4 or teredo address ? */
  if(!memcmp(addr->s6_addr, addr_6to4, 2) ||
      !memcmp(addr->s6_addr, addr_teredo, 4))
  {
    return 1;
  }
  return 0;
}

int net_ipv6_netmask_get_prefix(unsigned int prefix, struct in6_addr* addr)
{
  if(prefix > 128)
  {
    errno = EINVAL;
    return -1;
  }

  for(size_t i = 0 ; i < 16 ; i++)
  {
    if(prefix >= 8)
    {
      prefix -= 8;
      addr->s6_addr[i] = 0xFF;
      continue;
    }
    else
    {
      addr->s6_addr[i] = 0xFF << (8 - prefix);
      prefix = 0;
    }
  }

  return 0;
}

unsigned int net_ipv6_netmask_get_prefix_length(struct in6_addr* addr)
{
  unsigned int ret = 0;

  for(size_t i = 0 ; i < 16 ; i++)
  {
    switch(addr->s6_addr[i])
    {
      case 0xFF:
        ret += 8;
        break;
      case 0xFE:
        ret += 7;
        break;
      case 0xFC:
        ret += 6;
        break;
      case 0xF8:
        ret += 5;
        break;
      case 0xF0:
        ret += 4;
        break;
      case 0xE0:
        ret += 3;
        break;
      case 0xC0:
        ret += 2;
        break;
      case 0x80:
        ret += 1;
        break;
      default:
        break;
    }
  }
  return ret;
}

int net_ipv4_get_addresses(int ifindex, struct in_addr** addrs)
{
  struct ifaddrs* ifaddr = NULL;
  struct in_addr* ptr = NULL;
  size_t i = 0;
  size_t max = 16;

  if(getifaddrs(&ifaddr) == -1)
  {
    return -1;
  }

  ptr = malloc(sizeof(struct in_addr) * max);

  if(ptr == NULL)
  {
    freeifaddrs(ifaddr);
    return -1;
  }

  for(struct ifaddrs* ifa = ifaddr ; ifa != NULL ; ifa = ifa->ifa_next)
  {
    /* only considers IPv4 network interface */
    if(ifa->ifa_addr->sa_family != AF_INET ||
        if_nametoindex(ifa->ifa_name) != (unsigned int)ifindex)
    {
      continue;
    }

    memcpy(&ptr[i], &((struct sockaddr_in*)&ifa->ifa_addr)->sin_addr,
        sizeof(struct in_addr));
    i++;

    /* in case of overflow */
    if(i > max)
    {
      struct in_addr* tmp = NULL;

      max += 16;
      tmp = realloc(ptr, max * sizeof(struct in_addr));

      if(!tmp)
      {
        free(ptr);
        freeifaddrs(ifaddr);
        errno = ENOMEM;
        return -1;
      }
      ptr = tmp;
    }
  }

  freeifaddrs(ifaddr);
  *addrs = ptr;
  return i;
}

int net_ipv4_add_address(int ifindex, const struct in_addr* addr,
    unsigned int prefix)
{
  int sock = -1;
  struct ifreq ifr;
  int ret = -1;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  memcpy(&ifr.ifr_addr, addr, sizeof(struct in_addr));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  if(sock ==-1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCSIFADDR, &ifr);

  if(ret != -1)
  {
    struct in_addr netmask;

    memset(&netmask, 0x00, sizeof(struct in_addr));
    netmask.s_addr = htonl(0xFFFFFFFFu << (32 - prefix));
    memcpy(&ifr.ifr_addr, &netmask, sizeof(struct in_addr));

    ret = ioctl(sock, SIOCSIFADDR, &ifr);
  }

  close(sock);
  return ret;
}

int net_ipv4_add_address_str(int ifindex, const char* addr,
    unsigned int prefix)
{
  struct in_addr addrv4;

  if(inet_pton(AF_INET, addr, (void*)&addrv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv4_add_address(ifindex, &addrv4, prefix);
}

int net_ipv4_del_address(int ifindex, const struct in_addr* addr,
    unsigned int prefix)
{
  int sock = -1;
  struct ifreq ifr;
  int ret = -1;

  (void)prefix;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  memcpy(&ifr.ifr_addr, addr, sizeof(struct in_addr));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  if(sock == -1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCDIFADDR, &ifr);
  close(sock);
  return ret;
}

int net_ipv4_del_address_str(int ifindex, const char* addr,
    unsigned int prefix)
{
  struct in_addr addrv4;

  if(inet_pton(AF_INET, addr, (void*)&addrv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv4_del_address(ifindex, &addrv4, prefix);
}

int net_ipv4_add_route(int ifindex, const struct in_addr* dst,
    unsigned int prefix, const struct in_addr* gw)
{
  int sock = -1;
  int ret = -1;
  struct sockaddr_in* inaddr = NULL;

#ifdef __linux__
  struct rtentry rte;
  char dev[IF_NAMESIZE];

  memset(&rte, 0x00, sizeof(struct rtentry));
  rte.rt_flags = RTF_UP;

  if(if_indextoname(ifindex, dev) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1)
  {
    return -1;
  }

  /* destination */
  inaddr = (struct sockaddr_in*)&rte.rt_dst;
  inaddr->sin_family = AF_INET;
  memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
  inaddr->sin_port = 0;

  /* gateway */
  if(gw)
  {
    inaddr = (struct sockaddr_in*)&rte.rt_gateway;
    inaddr->sin_family = AF_INET;
    memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
    inaddr->sin_port = 0;

    rte.rt_flags |= RTF_GATEWAY;
  }

  /* netmask */
  inaddr = (struct sockaddr_in*)&rte.rt_genmask;
  inaddr->sin_family = AF_INET;
  inaddr->sin_addr.s_addr = htonl(0xFFFFFFFFu << (32 - prefix));
  inaddr->sin_port = 0;

  ret = ioctl(sock, SIOCADDRT, &rte);
  close(sock);
#else
  char buffer[1024];
  struct rt_msghdr* rtm = (struct rt_msghdr*)buffer;

  sock = socket(AF_ROUTE, SOCK_RAW, AF_INET);
  if(sock == -1)
  {
    return -1;
  }

  memset(rtm, 0x00, sizeof(struct rt_msghdr));
  rtm->rtm_msglen = sizeof(struct rt_msghdr) + (2 * sizeof(struct sockaddr_in));
  rtm->rtm_type = RTM_ADD;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_addrs = RTA_DST | RTA_NETMASK;
  rtm->rtm_errno = 0;
  rtm->rtm_pid = getpid();
  rtm->rtm_seq = 1;

  /* destination */
  inaddr = (struct sockaddr_in*)(buffer + sizeof(struct rt_msghdr));
  inaddr->sin_family = AF_INET;
  memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
  inaddr->sin_port = 0;

  /* gateway */
  if(gw)
  {
    inaddr++;
    inaddr->sin_family = AF_INET;
    memcpy(&inaddr->sin_addr, gw, sizeof(struct in_addr));
    inaddr->sin_port = 0;
    rtm->rtm_addrs |= RTA_GATEWAY;
    rtm->rtm_msglen += sizeof(struct sockaddr_in);
  }

  /* netmask */
  inaddr++;
  inaddr->sin_family = AF_INET;
  inaddr->sin_addr.s_addr = htonl(0xFFFFFFFFu << (32 - prefix));
  inaddr->sin_port = 0;

  rtm->rtm_index = ifindex;

  ret = (write(sock, (caddr_t)rtm, rtm->rtm_msglen) >= 0 ? 0 : -1);
  close(sock);
#endif
  return ret;
}

int net_ipv4_add_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw)
{
  struct in_addr dstv4;
  struct in_addr gwv4;

  if(inet_pton(AF_INET, dst, (void*)&dstv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }

  if(gw && inet_pton(AF_INET, gw, (void*)&gwv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv4_add_route(ifindex, &dstv4, prefix, gw ? &gwv4 : NULL);
}

int net_ipv4_del_route(int ifindex, const struct in_addr* dst,
    unsigned int prefix, const struct in_addr* gw)
{
  int sock = -1;
  int ret = -1;
  struct sockaddr_in* inaddr = NULL;

#ifdef __linux__
  struct rtentry rte;
  char dev[IF_NAMESIZE];

  memset(&rte, 0x00, sizeof(struct rtentry));
  rte.rt_flags = RTF_UP;

  if(if_indextoname(ifindex, dev) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1)
  {
    return -1;
  }

  /* destination */
  inaddr = (struct sockaddr_in*)&rte.rt_dst;
  inaddr->sin_family = AF_INET;
  memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
  inaddr->sin_port = 0;

  /* gateway */
  if(gw)
  {
    inaddr = (struct sockaddr_in*)&rte.rt_gateway;
    inaddr->sin_family = AF_INET;
    memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
    inaddr->sin_port = 0;

    rte.rt_flags |= RTF_GATEWAY;
  }

  /* netmask */
  inaddr = (struct sockaddr_in*)&rte.rt_genmask;
  inaddr->sin_family = AF_INET;
  inaddr->sin_addr.s_addr = htonl(0xFFFFFFFFu << (32 - prefix));
  inaddr->sin_port = 0;

  ret = ioctl(sock, SIOCDELRT, &rte);
  close(sock);
#else
  char buffer[1024];
  struct rt_msghdr* rtm = (struct rt_msghdr*)buffer;

  sock = socket(AF_ROUTE, SOCK_RAW, AF_INET);
  if(sock == -1)
  {
    return -1;
  }

  memset(rtm, 0x00, sizeof(struct rt_msghdr));
  rtm->rtm_msglen = sizeof(struct rt_msghdr) + (2 * sizeof(struct sockaddr_in));
  rtm->rtm_type = RTM_DELETE;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_addrs = RTA_DST | RTA_NETMASK;
  rtm->rtm_errno = 0;
  rtm->rtm_pid = getpid();
  rtm->rtm_seq = 1;

  /* destination */
  inaddr = (struct sockaddr_in*)(buffer + sizeof(struct rt_msghdr));
  inaddr->sin_family = AF_INET;
  memcpy(&inaddr->sin_addr, dst, sizeof(struct in_addr));
  inaddr->sin_port = 0;

  /* gateway */
  if(gw)
  {
    inaddr++;
    inaddr->sin_family = AF_INET;
    memcpy(&inaddr->sin_addr, gw, sizeof(struct in_addr));
    inaddr->sin_port = 0;
    rtm->rtm_addrs |= RTA_GATEWAY;
    rtm->rtm_msglen += sizeof(struct sockaddr_in);
  }

  /* netmask */
  inaddr++;
  inaddr->sin_family = AF_INET;
  inaddr->sin_addr.s_addr = htonl(0xFFFFFFFFu << (32 - prefix));
  inaddr->sin_port = 0;

  rtm->rtm_index = ifindex;

  ret = (write(sock, (caddr_t)rtm, rtm->rtm_msglen) >= 0 ? 0 : -1);
  close(sock);
#endif
  return ret;
}

int net_ipv4_del_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw)
{
  struct in_addr dstv4;
  struct in_addr gwv4;

  if(inet_pton(AF_INET, dst, (void*)&dstv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }

  if(gw && inet_pton(AF_INET, gw, (void*)&gwv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }

  return net_ipv4_del_route(ifindex, &dstv4, prefix, gw ? &gwv4 : NULL);
}

int net_ipv6_get_addresses(int ifindex, struct in6_addr** addrs)
{
  struct ifaddrs* ifaddr = NULL;
  struct in6_addr* ptr = NULL;
  size_t i = 0;
  size_t max = 16;

  if(getifaddrs(&ifaddr) == -1)
  {
    return -1;
  }

  ptr = malloc(sizeof(struct in6_addr) * max);

  if(ptr == NULL)
  {
    freeifaddrs(ifaddr);
    return -1;
  }

  for(struct ifaddrs* ifa = ifaddr ; ifa != NULL ; ifa = ifa->ifa_next)
  {
    /* only considers IPv4 network interface */
    if(ifa->ifa_addr->sa_family != AF_INET6 ||
        if_nametoindex(ifa->ifa_name) != (unsigned int)ifindex)
    {
      continue;
    }

    memcpy(&ptr[i], &((struct sockaddr_in6*)&ifa->ifa_addr)->sin6_addr,
        sizeof(struct in_addr));
    i++;

    /* in case of overflow */
    if(i > max)
    {
      struct in6_addr* tmp = NULL;

      max += 16;
      tmp = realloc(ptr, max * sizeof(struct in6_addr));

      if(!tmp)
      {
        free(ptr);
        freeifaddrs(ifaddr);
        errno = ENOMEM;
        return -1;
      }
      ptr = tmp;
    }
  }

  freeifaddrs(ifaddr);
  *addrs = ptr;
  return i;
}

int net_ipv6_add_address(int ifindex, const struct in6_addr* addr,
    unsigned int prefix)
{
  int ret = -1;
  int sock = -1;

#ifdef __linux__
  struct in6_ifreq ifr6;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);

  if(sock == -1)
  {
    return -1;
  }

  memset(&ifr6, 0x00, sizeof(struct in6_ifreq));
  memcpy(&ifr6.ifr6_addr, addr, sizeof(struct in6_addr));
  ifr6.ifr6_prefixlen = prefix;
  ifr6.ifr6_ifindex = ifindex;

  ret = ioctl(sock, SIOCSIFADDR, &ifr6);
  close(sock);
#else
  struct in6_aliasreq ifreq;
  struct sockaddr_in6* inaddr = NULL;

  memset(&ifreq, 0x00, sizeof(struct in6_aliasreq));
  if(if_indextoname(ifindex, ifreq.ifra_name) == NULL)
  {
    return -1;
  }

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_addr;
  inaddr->sin6_port = 0;
  inaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
  memcpy(&inaddr->sin6_addr, addr, sizeof(struct in6_addr));

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_prefixmask;
  inaddr->sin6_port = 0;
  inaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
  net_ipv6_netmask_get_prefix(prefix, &inaddr->sin6_addr);

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_dstaddr;
  inaddr->sin6_family = AF_UNSPEC;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);

  if(sock == -1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCAIFADDR_IN6, &ifreq);
  close(sock);
#endif

  return ret;
}

int net_ipv6_add_address_str(int ifindex, const char* addr,
    unsigned int prefix)
{
  struct in6_addr addrv6;

  if(inet_pton(AF_INET6, addr, (void*)&addrv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv6_add_address(ifindex, &addrv6, prefix);
}

int net_ipv6_del_address(int ifindex, const struct in6_addr* addr,
    unsigned int prefix)
{
  int ret = -1;
  int sock = -1;

#ifdef __linux__
  struct in6_ifreq ifr6;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);

  if(sock == -1)
  {
    return -1;
  }

  memset(&ifr6, 0x00, sizeof(struct in6_ifreq));
  memcpy(&ifr6.ifr6_addr, addr, sizeof(struct in6_addr));
  ifr6.ifr6_prefixlen = prefix;
  ifr6.ifr6_ifindex = ifindex;

  ret = ioctl(sock, SIOCDIFADDR, &ifr6);
  close(sock);
#else
  struct in6_aliasreq ifreq;
  struct sockaddr_in6* inaddr = NULL;

  memset(&ifreq, 0x00, sizeof(struct in6_aliasreq));
  if(if_indextoname(ifindex, ifreq.ifra_name) == NULL)
  {
    return -1;
  }

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_addr;
  inaddr->sin6_port = 0;
  inaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
  memcpy(&inaddr->sin6_addr, addr, sizeof(struct in6_addr));

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_prefixmask;
  inaddr->sin6_port = 0;
  inaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
  net_ipv6_netmask_get_prefix(prefix, &inaddr->sin6_addr);

  inaddr = (struct sockaddr_in6*)&ifreq.ifra_dstaddr;
  inaddr->sin6_family = AF_UNSPEC;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);

  if(sock == -1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCDIFADDR_IN6, &ifreq);
  close(sock);
#endif
  return ret;
}

int net_ipv6_del_address_str(int ifindex, const char* addr,
    unsigned int prefix)
{
  struct in6_addr addrv6;

  if(inet_pton(AF_INET6, addr, (void*)&addrv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv6_del_address(ifindex, &addrv6, prefix);
}

int net_ipv6_add_route(int ifindex, const struct in6_addr* dst,
    unsigned int prefix, const struct in6_addr* gw)
{
  int ret = -1;
  int sock = -1;

#ifdef __linux__
  struct in6_rtmsg rte;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
  if(sock == -1)
  {
    return -1;
  }

  memset(&rte, 0x00, sizeof(struct in6_rtmsg));
  rte.rtmsg_type = 0;
  rte.rtmsg_metric = 1;
  rte.rtmsg_dst_len = prefix;
  rte.rtmsg_src_len = 0;
  rte.rtmsg_ifindex = ifindex;
  rte.rtmsg_flags = RTF_UP;

  /* destination */
  memcpy(&rte.rtmsg_dst, dst, sizeof(struct in6_addr));

  /* gateway */
  if(gw)
  {
    rte.rtmsg_flags |= RTF_GATEWAY;
    memcpy(&rte.rtmsg_gateway, gw, sizeof(struct in6_addr));
  }

  ret = ioctl(sock, SIOCADDRT, &rte);
  close(sock);
#else
  struct sockaddr_in6* inaddr = NULL;
  char buffer[1024];
  struct rt_msghdr* rtm = (struct rt_msghdr*)buffer;

  sock = socket(AF_ROUTE, SOCK_RAW, AF_INET6);
  if(sock == -1)
  {
    return -1;
  }

  memset(rtm, 0x00, sizeof(struct rt_msghdr));
  rtm->rtm_msglen = sizeof(struct rt_msghdr) + (2 * sizeof(struct sockaddr_in6));
  rtm->rtm_type = RTM_ADD;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_addrs = RTA_DST | RTA_NETMASK;
  rtm->rtm_errno = 0;
  rtm->rtm_pid = getpid();
  rtm->rtm_seq = 1;

  /* destination */
  inaddr = (struct sockaddr_in6*)(buffer + sizeof(struct rt_msghdr));
  inaddr->sin6_family = AF_INET6;
  memcpy(&inaddr->sin6_addr, dst, sizeof(struct in6_addr));
  inaddr->sin6_port = 0;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

  /* gateway */
  if(gw)
  {
    inaddr++;
    inaddr->sin6_family = AF_INET6;
    memcpy(&inaddr->sin6_addr, gw, sizeof(struct in_addr));
    inaddr->sin6_port = 0;
#ifdef SIN6_LEN
    inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

    rtm->rtm_addrs |= RTA_GATEWAY;
    rtm->rtm_msglen += sizeof(struct sockaddr_in6);
  }

  /* netmask */
  inaddr++;
  inaddr->sin6_family = AF_INET6;
  net_ipv6_netmask_get_prefix(prefix, &inaddr->sin6_addr);
  inaddr->sin6_port = 0;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

  rtm->rtm_index = ifindex;

  ret = (write(sock, (caddr_t)rtm, rtm->rtm_msglen) >= 0 ? 0 : -1);
  close(sock);
#endif
  return ret;
}

int net_ipv6_add_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw)
{
  struct in6_addr dstv6;
  struct in6_addr gwv6;

  if(inet_pton(AF_INET6, dst, (void*)&dstv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }

  if(gw && inet_pton(AF_INET6, gw, (void*)&gwv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv6_add_route(ifindex, &dstv6, prefix, gw ? &gwv6 : NULL);
}

int net_ipv6_del_route(int ifindex, const struct in6_addr* dst,
    unsigned int prefix, const struct in6_addr* gw)
{
  int ret = -1;
  int sock = -1;

#ifdef __linux__
  struct in6_rtmsg rte;

  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IPV6);
  if(sock == -1)
  {
    return -1;
  }

  memset(&rte, 0x00, sizeof(struct in6_rtmsg));
  rte.rtmsg_type = 0;
  rte.rtmsg_metric = 1;
  rte.rtmsg_dst_len = prefix;
  rte.rtmsg_src_len = 0;
  rte.rtmsg_ifindex = ifindex;
  rte.rtmsg_flags = RTF_UP;

  /* destination */
  memcpy(&rte.rtmsg_dst, dst, sizeof(struct in6_addr));

  /* gateway */
  if(gw)
  {
    rte.rtmsg_flags |= RTF_GATEWAY;
    memcpy(&rte.rtmsg_gateway, gw, sizeof(struct in6_addr));
  }

  ret = ioctl(sock, SIOCDELRT, &rte);
  close(sock);
#else
  struct sockaddr_in6* inaddr = NULL;
  char buffer[1024];
  struct rt_msghdr* rtm = (struct rt_msghdr*)buffer;

  sock = socket(AF_ROUTE, SOCK_RAW, AF_INET6);
  if(sock == -1)
  {
    return -1;
  }

  memset(rtm, 0x00, sizeof(struct rt_msghdr));
  rtm->rtm_msglen = sizeof(struct rt_msghdr) + (2 * sizeof(struct sockaddr_in6));
  rtm->rtm_type = RTM_DELETE;
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_addrs = RTA_DST | RTA_NETMASK;
  rtm->rtm_errno = 0;
  rtm->rtm_pid = getpid();
  rtm->rtm_seq = 1;

  /* destination */
  inaddr = (struct sockaddr_in6*)(buffer + sizeof(struct rt_msghdr));
  inaddr->sin6_family = AF_INET6;
  memcpy(&inaddr->sin6_addr, dst, sizeof(struct in6_addr));
  inaddr->sin6_port = 0;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

  /* gateway */
  if(gw)
  {
    inaddr++;
    inaddr->sin6_family = AF_INET6;
    memcpy(&inaddr->sin6_addr, gw, sizeof(struct in_addr));
    inaddr->sin6_port = 0;
#ifdef SIN6_LEN
    inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

    rtm->rtm_addrs |= RTA_GATEWAY;
    rtm->rtm_msglen += sizeof(struct sockaddr_in6);
  }

  /* netmask */
  inaddr++;
  inaddr->sin6_family = AF_INET6;
  net_ipv6_netmask_get_prefix(prefix, &inaddr->sin6_addr);
  inaddr->sin6_port = 0;
#ifdef SIN6_LEN
  inaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif

  rtm->rtm_index = ifindex;

  ret = (write(sock, (caddr_t)rtm, rtm->rtm_msglen) >= 0 ? 0 : -1);
  close(sock);
#endif
  return ret;
}

int net_ipv6_del_route_str(int ifindex, const char* dst, unsigned int prefix,
    const char* gw)
{
  struct in6_addr dstv6;
  struct in6_addr gwv6;

  if(inet_pton(AF_INET6, dst, (void*)&dstv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }

  if(gw && inet_pton(AF_INET6, gw, (void*)&gwv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_ipv6_del_route(ifindex, &dstv6, prefix, gw ? &gwv6 : NULL);
}

int net_sock_join_mcast(int fd, int ifindex, const struct in_addr* group)
{
  struct ip_mreq mcast;

  (void)ifindex;

  if(!IN_CLASSD(ntohl(group->s_addr)))
  {
    errno = EINVAL;
    return -1;
  }

  memset(&mcast, 0x00, sizeof(struct ip_mreq));
  memcpy(&mcast.imr_multiaddr, group, sizeof(struct ip_mreq));
  mcast.imr_interface.s_addr = htonl(INADDR_ANY);

  return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mcast,
        sizeof(struct ip_mreq));
}

int net_sock_join_mcast_str(int fd, int ifindex, const char* group)
{
  struct in_addr groupv4;

  if(inet_pton(AF_INET, group, (void*)&groupv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_sock_join_mcast(fd, ifindex, &groupv4);
}

int net_sock_leave_mcast(int fd, int ifindex, const struct in_addr* group)
{
  struct ip_mreq mcast;

  (void)ifindex;

  if(!IN_CLASSD(ntohl(group->s_addr)))
  {
    errno = EINVAL;
    return -1;
  }

  memset(&mcast, 0x00, sizeof(struct ip_mreq));
  memcpy(&mcast.imr_multiaddr, group, sizeof(struct ip_mreq));
  mcast.imr_interface.s_addr = htonl(INADDR_ANY);

  return setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&mcast,
        sizeof(struct ip_mreq));
}

int net_sock_leave_mcast_str(int fd, int ifindex, const char* group)
{
  struct in_addr groupv4;

  if(inet_pton(AF_INET, group, (void*)&groupv4) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_sock_leave_mcast(fd, ifindex, &groupv4);
}

int net_sock_join_mcast6(int fd, int ifindex, const struct in6_addr* group)
{
  struct ipv6_mreq mcast;

  if(!IN6_IS_ADDR_MULTICAST(group))
  {
    errno = EINVAL;
    return -1;
  }

  mcast.ipv6mr_interface = ifindex;
  memcpy(&mcast.ipv6mr_multiaddr, group, sizeof(struct in6_addr));
  return setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mcast,
      sizeof(struct ipv6_mreq));
}

int net_sock_join_mcast6_str(int fd, int ifindex, const char* group)
{
  struct in6_addr groupv6;

  if(inet_pton(AF_INET6, group, (void*)&groupv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_sock_leave_mcast6(fd, ifindex, &groupv6);
}

int net_sock_leave_mcast6(int fd, int ifindex, const struct in6_addr* group)
{
  struct ipv6_mreq mcast;

  if(!IN6_IS_ADDR_MULTICAST(group))
  {
    errno = EINVAL;
    return -1;
  }

  mcast.ipv6mr_interface = ifindex;
  memcpy(&mcast.ipv6mr_multiaddr, group, sizeof(struct in6_addr));
  return setsockopt(fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mcast,
      sizeof(struct ipv6_mreq));
}

int net_sock_leave_mcast6_str(int fd, int ifindex, const char* group)
{
  struct in6_addr groupv6;

  if(inet_pton(AF_INET6, group, (void*)&groupv6) == -1)
  {
    errno = EINVAL;
    return -1;
  }
  return net_sock_leave_mcast6(fd, ifindex, &groupv6);
}

int net_iface_list(struct net_iface** ifaces)
{
  struct ifaddrs* ifaddr = NULL;
  struct net_iface* ptr = NULL;
  size_t i = 0;
  size_t max = 128;

  if(getifaddrs(&ifaddr) == -1)
  {
    return -1;
  }

  ptr = malloc(sizeof(struct net_iface) * max);

  if(ptr == NULL)
  {
    freeifaddrs(ifaddr);
    return -1;
  }

  for(struct ifaddrs* ifa = ifaddr ; ifa != NULL ; ifa = ifa->ifa_next)
  {
    /* only considers link network interface because we do not want
     * duplicates and we want link-layer address.
     */
#ifdef __linux__
    if(ifa->ifa_addr->sa_family != AF_PACKET)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
    if(ifa->ifa_addr->sa_family != AF_LINK)
#endif
    {
      continue;
    }

    strncpy(ptr[i].ifname, ifa->ifa_name, IF_NAMESIZE - 1);
    ptr[i].ifname[IF_NAMESIZE - 1] = 0x00;

    ptr[i].ifindex = if_nametoindex(ifa->ifa_name);
    {
#ifdef __linux__
      struct sockaddr_ll* ll = (struct sockaddr_ll*)ifa->ifa_addr;

      memcpy(ptr[i].ifaddr, ll->sll_addr, ll->sll_halen);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
      struct sockaddr_dl* ll = (struct sockaddr_dl*)ifa->ifa_addr;
      char* lla = LLADDR(ll);

      memcpy(ptr[i].ifaddr, lla, ll->sdl_alen);
#endif
    }
    i++;

    /* in case of overflow */
    if(i > max)
    {
      struct net_iface* tmp = NULL;

      max += 128;
      tmp = realloc(ptr, max * sizeof(struct net_iface));

      if(!tmp)
      {
        free(ptr);
        errno = ENOMEM;
        return -1;
      }
      ptr = tmp;
    }
  }

  freeifaddrs(ifaddr);
  *ifaces = ptr;
  return i;
}

int net_iface_get_flag(int ifindex, int flag)
{
  int ret = -1;
  int sock = -1;
  struct ifreq ifr;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  if(sock == -1)
  {
    return -1;
  }

  if(ioctl(sock, SIOCGIFFLAGS, &ifr) == -1)
  {
    ret = -1;
  }
  else
  {
    ret = ifr.ifr_flags & flag;
  }

  close(sock);
  return ret;
}

int net_iface_set_flag(int ifindex, int flag, int enable)
{
  int sock = -1;
  int ret = -1;
  struct ifreq ifr;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

  if(sock == -1)
  {
    return -1;
  }

  if(ioctl(sock, SIOCGIFFLAGS, &ifr) == -1)
  {
    close(sock);
    return -1;
  }

  if(enable)
  {
    ifr.ifr_flags |= flag;
  }
  else
  {
    ifr.ifr_flags &= ~flag;
  }

  ret = ioctl(sock, SIOCSIFFLAGS, &ifr);
  close(sock);

  return ret;
}

int net_iface_is_up(int ifindex)
{
  return net_iface_get_flag(ifindex, IFF_UP);
}

int net_iface_set_up(int ifindex, int up)
{
  return net_iface_set_flag(ifindex, IFF_UP, up);
}

int net_iface_get_mtu(int ifindex)
{
  struct ifreq ifr;
  int sock = -1;
  int ret = -1;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCGIFMTU, &ifr);

  if(ret == 0)
  {
    ret = ifr.ifr_mtu;
  }

  close(sock);
  return ret;
}

int net_iface_set_mtu(int ifindex, unsigned int mtu)
{
  struct ifreq ifr;
  int sock = -1;
  int ret = -1;

  memset(&ifr, 0x00, sizeof(struct ifreq));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }
  ifr.ifr_mtu = mtu;

  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1)
  {
    return -1;
  }

  ret = ioctl(sock, SIOCSIFMTU, &ifr);

  close(sock);
  return ret;
}

int net_iface_set_addr(int ifindex, const char* addr, size_t size)
{
  int ret = -1;
  int sock = -1;
  struct ifreq ifr;

  memset(&ifr, 0x00, sizeof(ifr));
  if(if_indextoname(ifindex, ifr.ifr_name) == NULL)
  {
    return -1;
  }
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock == -1)
  {
    return -1;
  }

#ifdef __linux__
  // ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER
  memcpy(ifr.ifr_hwaddr.sa_data, addr, size);

  ret = ioctl(sock, SIOCSIFHWADDR, &ifr);
  close(sock);
#else
  ifr.ifr_addr.sa_len = size;
  ifr.ifr_addr.sa_family = AF_LINK;
  memcpy(ifr.ifr_addr.sa_data, addr, size);

  ret = ioctl(sock, SIOCSIFLLADDR, (caddr_t)&ifr);
  close(sock);
#endif
  return ret;
}

int net_eth_pton(const char* src, void* dst)
{
  size_t i = 0;
  char* str = NULL;
  char* tmp = NULL;
  char tmp2[64];
  char addr[8];
  char types[2] = ":;";
  char type = 0x00;

  /* find if we have colons or hyphens link-layer address */
  for(size_t c = 0 ; c < 2 ; c++)
  {
    if(strstr(src, &types[c]))
    {
      if(type != 0x00)
      {
        /* string with both hyphen and colon, invalid string */
        errno = EINVAL;
        return -1;
      }
      type = types[c];
    }
  }

  /* string with no colon or hypen, invalid string */
  if(type == 0x00)
  {
    errno = EINVAL;
    return -1;
  }

  strncpy(tmp2, src, sizeof(tmp2));
  tmp2[sizeof(tmp2) - 1] = 0x00;
  str = strtok_r(tmp2, &type, &tmp);

  do
  {
    long int res = strtol(str, NULL, 16);

    if(res == LONG_MIN || res == LONG_MAX)
    {
      return -1;
    }

    addr[i] = (char)res;
    i++;

    /* 6 is the size of ethernet address in bytes */
    if(i >= 6)
    {
      return -1;
    }
    str = strtok_r(NULL, &type, &tmp);
  }while(str);

  memcpy(dst, addr, i);
  return 0;
}

char* net_eth_ntop(const void* src, char* dst, socklen_t size)
{
  const char* addr = src;

  if(!src)
  {
    return NULL;
  }

  /* ethernet address is 6 bytes so 18 bytes in human-readable form, including
   * NULL character.
   */
  if(18 > size)
  {
    return NULL;
  }

  for(size_t i = 0 ; i < 6 ; i++)
  {
    snprintf(dst + (i * 3), size - (i * 3), "%02X:", addr[i]);
  }
  dst[17] = 0x00;

  return dst;
}

#ifdef __cplusplus
}
#endif

