/**
 * \file netevt.c
 * \brief Network event manager.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "netevt.h"
#include "netevt_select.h"
#include "netevt_poll.h"
#include "netevt_epoll.h"
#include "netevt_kqueue.h"

/**
 * \struct netevt
 * \brief Network event manager.
 */
struct netevt
{
    enum netevt_method method; /**< Method to detect network event used. */
    struct list_head sockets; /**< List of sockets. */
    unsigned int nb_sockets; /**< Number of sockets. */
    struct netevt_impl impl; /**< Implementation specific (select, ...). */
    int (*impl_init)(struct netevt_impl*); /**< Initialize implementation. */
    int (*impl_destroy)(struct netevt_impl*); /**< Destroy implementation. */
};

int netevt_is_method_supported(enum netevt_method method)
{
#ifdef __linux__
    /* *BSD specific */
    if(method == NETEVT_KQUEUE)
    {
        return 0;
    }
    else
    {
        /* select, poll and epoll are supported on Linux */
        return 1;
    }
#elif defined(_WIN32) || defined(_WIN64)
    if(method != NETEVT_SELECT)
    {
        return 0;
    }
    else
    {
        return 1;
    }
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
    /* Linux specific */
    if(method == NETEVT_EPOLL)
    {
        return 0;
    }
    else
    {
        /* select, poll and kqueue are supported on *BSD and OS X */
        return 1;
    }
#else
    /* Linux and *BSD specific */
    if(method == NETEVT_EPOLL || method == NETEVT_KQUEUE)
    {
        return 0;
    }
    else
    {
        /* select and poll useally is supported on many platforms */
        return 1;
    }
#endif
}

struct netevt* netevt_new(enum netevt_method method)
{
    struct netevt* ret = NULL;
    enum netevt_method m = method;

    if(m == NETEVT_AUTO)
    {
        if(netevt_is_method_supported(NETEVT_EPOLL))
        {
            m = NETEVT_EPOLL;
        }
        else if(netevt_is_method_supported(NETEVT_KQUEUE))
        {
            m = NETEVT_KQUEUE;
        }
        else if(netevt_is_method_supported(NETEVT_SELECT))
        {
            m = NETEVT_SELECT;
        }
        else if(netevt_is_method_supported(NETEVT_POLL))
        {
            m = NETEVT_POLL;
        }
        else
        {
            /* no method available! */
            return NULL;
        }
    }
    else if(!netevt_is_method_supported(method))
    {
        return NULL;
    }
    
    ret = malloc(sizeof(struct netevt));

    if(!ret)
    {
        return NULL;
    }

    memset(ret, 0x00, sizeof(struct netevt));
    list_head_init(&ret->sockets);

    switch(m)
    {
        case NETEVT_SELECT:
            ret->impl_init = netevt_select_init;
            ret->impl_destroy = netevt_select_destroy;
            break;
        case NETEVT_POLL:
            ret->impl_init = netevt_poll_init;
            ret->impl_destroy = netevt_poll_destroy;
            break;
        case NETEVT_EPOLL:
            ret->impl_init = netevt_epoll_init;
            ret->impl_destroy = netevt_epoll_destroy;
            break;
        case NETEVT_KQUEUE:
            //ret->impl_init = netevt_kqueue_init;
            //ret->impl_destroy = netevt_kqueue_destroy;
            break;
        default:
            /* should not happen */
            break;
    }

    if(ret->impl_init(&ret->impl) == -1)
    {
        free(ret);
        return NULL;
    }

    ret->method = m;

    return ret;
}

void netevt_free(netevt* obj)
{
    netevt_remove_all_sockets(*obj);
    (*obj)->impl_destroy(&(*obj)->impl);
    free(*obj);
    *obj = NULL;
}

int netevt_add_socket(netevt obj, int sock, int event_mask, void* data)
{
    struct netevt_socket* p = NULL;
    socklen_t addr_size = sizeof(struct sockaddr_storage);

    p = malloc(sizeof(struct netevt_socket));

    if(!p)
    {
        return -1;
    }

    p->sock = sock;
    getsockname(p->sock, (struct sockaddr*)&p->local, &addr_size);
    p->data = data;
    
    if(obj->impl.add_socket(&obj->impl, obj, p, event_mask) != 0)
    {
        free(p);
        return -1;
    }

    list_head_add_tail(&obj->sockets, &p->list);
    obj->nb_sockets++;

    return 0;
}

int netevt_remove_socket(netevt obj, int sock)
{
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;
    struct netevt_socket* s = NULL;

    /* find the socket */
    list_head_iterate_safe(&obj->sockets, pos, tmp)
    {
        s = list_head_get(pos, struct netevt_socket, list);
        if(s->sock == sock)
        {
            break;
        }
        s = NULL;
    }

    if(s)
    {
        obj->impl.remove_socket(&obj->impl, obj, s);
        list_head_remove(&obj->sockets, &s->list);
        obj->nb_sockets--;
        free(s);
        return 0;
    }

    return -1;
}

int netevt_remove_all_sockets(netevt obj)
{
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;

    /* find the socket */
    list_head_iterate_safe(&obj->sockets, pos, tmp)
    {
        struct netevt_socket* s = list_head_get(pos, struct netevt_socket, list);
        obj->impl.remove_socket(&obj->impl, obj, s);
        obj->nb_sockets--;
        free(s);
    }

    return 0;
}

int netevt_wait(netevt obj, int timeout, struct netevt_event* events, size_t events_nb)
{
    return obj->impl.wait(&obj->impl, obj, timeout, events, events_nb);
}

int netevt_get_nb_sockets(netevt obj)
{
    return obj->nb_sockets;
}

struct netevt_socket* netevt_get_sockets(netevt obj, size_t* sockets_nb)
{
    struct netevt_socket* ret = NULL;
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;
    unsigned int i = 0;
    
    if(obj->nb_sockets)
    {
        ret = malloc(sizeof(struct netevt_socket) * obj->nb_sockets);
    }
    else
    {
        errno = 0;
    }

    if(!ret)
    {
        *sockets_nb = 0;
        return NULL;
    }

    list_head_iterate_safe(&obj->sockets, pos, tmp)
    {
        struct netevt_socket* s = list_head_get(pos, struct netevt_socket, list);
        ret[i].sock = s->sock;
        memcpy(&ret[i].local, &s->local, sizeof(struct sockaddr_storage));
        i++;
    }

    *sockets_nb = i;

    return ret;
}

struct list_head* netevt_get_sockets_list(netevt obj)
{
    return &obj->sockets;
}

void netevt_fprint_info(netevt obj, FILE* output)
{
    struct list_head* pos = NULL;
    struct list_head* tmp = NULL;
    char buf[INET6_ADDRSTRLEN];

    fprintf(output, "Information about netevt: %p\n", (void*)obj);
    fprintf(output, "\tNumber of sockets: %d\n", obj->nb_sockets);

    /* find the socket */
    list_head_iterate_safe(&obj->sockets, pos, tmp)
    {
        struct netevt_socket* s = list_head_get(pos, struct netevt_socket, list);
        inet_ntop(s->local.ss_family, &s->local, buf, INET6_ADDRSTRLEN);
        fprintf(output, "\tSocket: %d local address: %s\n", s->sock, buf);
    }
}

void netevt_print_info(netevt obj)
{
    netevt_fprint_info(obj, stdout);
}

