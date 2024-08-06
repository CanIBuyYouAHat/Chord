#include "chord/chord.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0

static int end_node = FALSE;
static struct pollfd fds[200];
static int nfds = 1;

pthread_t polling_background, stabilize_background, fix_fingers_background, check_predecessor_background;

void handle_signal(int signal) {
  if (signal == SIGINT) {
    printf("\nShutting down...\n");
    end_node = TRUE;
  }
}

void cleanup() {
  pthread_join(polling_background, NULL);
  pthread_join(stabilize_background, NULL);
  pthread_join(fix_fingers_background, NULL);
  pthread_join(check_predecessor_background, NULL);

  printf("Cleaning fds...");
  for (int i = 0; i < nfds; i++) {
    if (fds[i].fd >= 0) {
       close(fds[i].fd);
    }
  }
  printf("cleaned\n");

  printf("Cleaning nodes...");
  cleanup_node(chord_node);
  printf("cleaned\n");
}

void *polling() {
  int socket_fd, new_fd;
  int close_conn;
  int compress_array = FALSE;
  uint8_t buffer[1024];
  int rc, on = 1, i, j, len;
  int curr_size = 0;

  // Domain (IPV4), Type (?), Protocol (0)
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket() failed");
    exit(1);
  } else {
    printf("Socket created...\n");
  }
  
  // Set socket reusability, if it fails then close the socket
  rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, 
                  (char *)&on, sizeof(int)); 
  if (rc < 0) {
    perror("setsockopt() failed");
    close(socket_fd);
    exit(1);
  } else {
    printf("Set reusability...\n");
  }

  
  // Set non-blocking
  rc = ioctl(socket_fd, FIONBIO, (char *)&on);
  if (rc < 0) {
    perror("ioctl() failed");
    close(socket_fd);
    exit(1);
  } else {
    printf("Set non-blocking...\n");
  }
  
  chord_node->socket_fd = socket_fd;

  signal(SIGINT, handle_signal);

  rc = bind(chord_node->socket_fd, (struct sockaddr *)&(chord_node->my_addr), sizeof(chord_node->my_addr));
  if (rc < 0) {
    perror("bind() failed");
    close(chord_node->socket_fd);
    exit(-1);
  } else {
    printf("Socket bound...\n");
  }
 
  rc = listen(chord_node->socket_fd, 32);
  if (rc < 0) {
    perror("listen() failed");
    close(chord_node->socket_fd);
    exit(-1);
  } else {
    printf("Listening...\n");
  }

  memset(fds, 0, sizeof(fds));

  fds[0].fd = chord_node->socket_fd;
  fds[0].events = POLLIN;

  do {
    rc = poll(fds, nfds, -1);
    if (rc < 0) {
      perror("poll() failed");
      break;
    } else {
      printf("Polling...\n");
    }

    if (rc == 0) {
      printf("poll() timed out. Should not reach.");
      break;
    }

    curr_size = nfds;
    for (i = 0; i < curr_size; i++) {
      if (fds[i].revents == 0)
        continue;
    
      if (fds[i].revents != POLLIN) {
         printf("Error, revents = %d\n", fds[i].revents);
         end_node = TRUE;
         break;
      }
      
      if (fds[i].fd == chord_node->socket_fd) {
         printf("Listening socket is readable\n");

         do {
          new_fd = accept(chord_node->socket_fd, NULL, NULL);
          if (new_fd < 0) {
            if (errno != EWOULDBLOCK) {
              perror("accept() failed\n");
              end_node = TRUE;
            }
            break;
          }

          printf("New incoming connection - %d\n", new_fd);

          fds[nfds].fd = new_fd;
          fds[nfds].events = POLLIN;
          nfds++;

         } while (new_fd != -1);
    } else {
        printf("Descriptor %d is readable\n", fds[i].fd);
        close_conn = FALSE;

        do {
          ssize_t bytes_received = recv(fds[i].fd, buffer, sizeof(buffer), 0);
          if (rc < 0) {
            if (errno != EWOULDBLOCK) {
              perror("recv() failed");
              close_conn = TRUE;
            }
            break;
          }

          if (rc == 0) {
            printf("\nConnection closed\n");
            close_conn = TRUE;
            break;
          }

          len = rc;
          printf("%d bytes received\n", len);

          dispatch_chord_message(fds[i].fd, buffer, bytes_received);
          close_conn = TRUE;
          break;

           /* rc = send(fds[i].fd, buffer, len, 0);
           if (rc < 0) {
            perror("send() failed");
            close_conn = TRUE;
            break;
           } */

         } while (TRUE);

         if (close_conn) {
          close(fds[i].fd);
          fds[i].fd = -1;
          compress_array = TRUE;
         }

    } // End of existing connections
  } // End of loop through pollable descriptors
  
  if (compress_array) {
    compress_array = FALSE;
    for (i = 0; i < nfds; i++) {
      if (fds[i].fd == -1) {
         for (j = i; j < nfds; j++) {
            fds[j].fd = fds[j + 1].fd;
         }
         i--;
         nfds--;
  
      }
    }
  }

  } while (end_node == FALSE);

  cleanup();

}

int main(int argc, char *argv[]){
  chord_node = chord_constructor(argc, argv);
  
  if (chord_node->join_addr) {
    join();
  } else {
    create();
  }

  if (pthread_create(&polling_background, NULL, polling, NULL) != 0) {
    perror("Failed to create polling thread\n");
    exit(1);
  }

  if (pthread_create(&stabilize_background, NULL, stabilize, NULL) != 0) {
    perror("Failed to create stabilization thread\n");
    exit(1);
  }

  if (pthread_create(&fix_fingers_background, NULL, fix_fingers, NULL) != 0) {
    printf("Failed to create fix fingers thread\n");
    exit(1);
  }

  if (pthread_create(&check_predecessor_background, NULL, check_predecessor, NULL) != 0) {
    printf("Failed to create check predecessor thread\n");
    exit(1);
  }

  return 0;
}
