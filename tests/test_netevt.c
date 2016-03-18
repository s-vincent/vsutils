/**
 * \file test_netevt.c
 * \brief Tests for network event.
 * \author Sebastien Vincent
 * \date 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#else
#define WINDOWS_LEAN_AND_MEAN
#include <Winsock.h>
#endif

#include "util_net.h"
#include "netevt.h"

/**
 * \var g_run
 * \brief Running state of the program.
 */
static volatile sig_atomic_t g_run = 0;

/**
 * \brief Signal management.
 * \param code signal code
 */
static void signal_handler(int code)
{
  switch(code)
  {
    case SIGINT:
    case SIGTERM:
      /* stop the program */
      g_run = 0;
      break;
    default:
      break;
  }
}

/**
 * \brief Entry point of the program.
 * \param argc number of arguments.
 * \param argv array of arguments.
 * \return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char** argv)
{
  netevt nevt = netevt_new(NETEVT_AUTO);
  int sock = -1;

  (void)argc;
  (void)argv;

  if(signal(SIGINT, signal_handler) == SIG_ERR)
  {
    fprintf(stderr, "Signal SIGINT will not be catched\n");
  }

  if(signal(SIGTERM, signal_handler) == SIG_ERR)
  {
    fprintf(stderr, "Signal SIGTERM will not be catched\n");
  }

  fprintf(stdout, "Begin\n");
  if(!nevt)
  {
    perror("netevt_new");
    exit(EXIT_FAILURE);
  }

  sock = net_socket_create(IPPROTO_TCP, "127.0.0.1", 8022, 1, 1);

  if(sock == -1)
  {
    perror("sock");
    netevt_free(&nevt);
    exit(EXIT_FAILURE);
  }

  if(listen(sock, 5) == -1)
  {
    perror("listen");
    netevt_free(&nevt);
    close(sock);
    exit(EXIT_FAILURE);
  }

  if(netevt_add_socket(nevt, sock, NETEVT_STATE_READ, "server") == -1)
  {
    perror("netevt_add_socket");
    close(sock);
    netevt_free(&nevt);
    exit(EXIT_FAILURE);
  }

  g_run = 1;
  
  while(g_run)
  {
    size_t nb_evt = 32; 
    struct netevt_event evts[nb_evt];
    int ret = -1;
    ssize_t nb = 0;
    char buf[1500];
    unsigned int timeout = 2;

    ret = netevt_wait(nevt, timeout, evts, nb_evt);

    if(ret == 0)
    {
      fprintf(stdout, "Timeout\n");
    }
    else if(ret == -1)
    {
      perror("Error");
    }
    else
    {
      for(int i = 0 ; i < ret ; i++)
      {
        if(evts[i].state & NETEVT_STATE_READ)
        {
          if(evts[i].socket.sock == sock)
          {
            struct sockaddr_storage ss;
            socklen_t ss_len = sizeof(struct sockaddr_storage);
            int clt = 0;

            fprintf(stdout, "Accept operation\n");

            clt = accept(sock, (struct sockaddr*)&ss, &ss_len);

            if(clt == -1)
            {
              perror("accept");
              continue;
            }

            if(netevt_add_socket(nevt, clt, NETEVT_STATE_READ, "client") == -1)
            {
              perror("netevt_add_socket client");
            }
          }
          else
          {
            fprintf(stdout, "Read operation\n");
            fprintf(stdout, "Socket data: %s\n",
                (char*)evts[i].socket.data);

            nb = recv(evts[i].socket.sock, buf, 1500 - 1, 0);

            if(nb == -1)
            {
              fprintf(stderr, "Error read\n");
              netevt_remove_socket(nevt, evts[i].socket.sock);
            }
            else if(nb == 0)
            {
              fprintf(stderr, "Disconnected\n");
              netevt_remove_socket(nevt, evts[i].socket.sock);
            }
            else
            {
              buf[nb] = 0x00;
              fprintf(stdout, "Buf: %s\n", buf);
              if(send(evts[i].socket.sock, buf, nb, 0) == -1)
              {
                fprintf(stderr, "Error send\n");
              }
              else
              {
                fprintf(stderr, "Send OK\n");
              }
            }
          }
        }
      }
    }
  }

  netevt_free(&nevt);

  fprintf(stdout, "End\n");
  return EXIT_SUCCESS;
}

