#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <time.h>

#include "chord.h"
#include "../hash/hash.h"

Chord *chord_node = NULL;

error_t chord_parser(int key, char *arg, struct argp_state *state)
{
  Chord *node = state->input;
  error_t ret = 0; 
 
  switch(key)
  {
    case 'p':
      {
        uint16_t port = atoi(arg);
        if (0) {
          argp_error(state, "Invalid port number");
        }
        node->my_addr.sin_family = AF_INET;
        node->my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        node->my_addr.sin_port = htons(port);
        
        break;
      }

  case 300:
      {
        node->join_addr.sin_family = AF_INET;
        int success = inet_pton(AF_INET, arg, (void *)&(node->join_addr.sin_addr.s_addr));
        if (success != 1) {
          argp_error(state, "Invalid address");
        }
        break;
      }

  case 301:
      {
        uint16_t port = atoi(arg);
        if (0) {
          argp_error(state, "Invalid port number");
        }
        node->join_addr.sin_port = port;
        break;
      }

    case 400:
       {
        int ts_arg = atoi(arg);
        if (0) {
          argp_error(state, "Invalid stabilize period");
        } else {
          node->stabilize_period = (uint16_t)ts_arg;
        }
        break;
      }

    case 401:
      {
        int ts_arg = atoi(arg);
        if (0) {
          argp_error(state, "Invalid fix fingers period");
        } else {
          node->fix_fingers_period = (uint16_t)ts_arg;
        }
        break;
      }

    case 402:
      {
        int ts_arg = atoi(arg);
        if (0) {
          argp_error(state, "Invalid check predecessor period");
        } else {
          node->check_predecessor_period = (uint16_t)ts_arg;
        }
        break;
      }

    case 'r':
      {
        if (0) {
          argp_error(state, "Invalid option for successor count");
        } else {
          node->num_successors = (uint8_t)atoi(arg);
        }
        break;
      }

    default:
          ret = ARGP_ERR_UNKNOWN;
          break;
    } 
    return ret;
}

void set_id() {
  uint8_t checksum[20];
  char id[50];
  char ip_str[INET_ADDRSTRLEN];
  int port;
  uint64_t head = 0;

  if (inet_ntop(AF_INET, &(chord_node->my_addr.sin_addr), ip_str, sizeof(ip_str)) == NULL) {
    perror("inet_ntop error");
    exit(1);
  }

  port = ntohs(chord_node->my_addr.sin_port);

  snprintf(id, sizeof(id), "%s:%d", ip_str, port);

  struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
  if (!ctx) {
    fprintf(stderr, "Error creating checksum\n");
    exit(1);
  }

  int error = sha1sum_finish(ctx, (const uint8_t*)id, strlen(id), checksum);
  if (!error) {
    head = sha1sum_truncated_head(checksum);
    chord_node->id = head;
    printf("Node ID: %" PRIu64, head);
    printf("\n\n");
  }

  sha1sum_reset(ctx);
}
 
Chord *chord_constructor(int argc, char *argv[]) 
{
  struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port that is being used at the chord node", 0},
		{ "sp", 0, "stabilize_period", 0, "The time between invocations of 'stablize'", 0},
		{ "ffp", 401, "fix_fingers_period", 0, "The time between invocations of 'fix_fingers'", 0},
		{ "cpp", 402, "check_predecessor_period", 0, "The time between invocations of 'check_predecessor'", 0},
		{ "successors", 'r', "successors", 0, "The number of successors maintained", 0},
		{ "ja", 300, "join_addr", 0, "The IP address the chord node to join", 0},
		{ "jp", 301, "join_port", 0, "The port that chord node we're joining is listening on", 0},
		{0}
	};

  struct argp argp_settings = { options, chord_parser, 0, 0, 0, 0, 0 };
  
  chord_node = (Chord *)malloc(sizeof(Chord)); 
  if (!chord_node) {
    fprintf(stderr, "Memory allocation for node failed\n");
    exit(1);
  }

  memset(chord_node, 0, sizeof(Chord));
  
  if (argp_parse(&argp_settings, argc, argv, 0, NULL, chord_node) != 0) 
  {
    fprintf(stderr, "Error while parsing!\n");
    exit(1);
  } 

  chord_node->pred = NULL;
  chord_node->succ = NULL;
  chord_node->fingers = (Chord *)malloc(64 * sizeof(Chord));

  if (!chord_node->fingers) {
    fprintf(stderr, "Memory allocation for fingers table failed");
    cleanup_node(chord_node);
    exit(1);
  }

  chord_node->fingers[0] = *chord_node;
  set_id(chord_node);
  return chord_node;
}
 
void cleanup_node(Chord *node){
  if (!node) {
    return;
  }
  if (node != chord_node) {
    if (node->pred) {
      cleanup_node(node->pred);
      node->pred = NULL;
    }

    if (node->succ) {
      cleanup_node(node->succ);
      node->succ = NULL;
    }

    if (node->socket_fd >= 0) {
      close(node->socket_fd);
      node->socket_fd = -1;
    }

    if (node->fingers) {
      for (int i = 1; i < 64; i++) {
        if (&node->fingers[i] != node) {
          cleanup_node(&node->fingers[i]);
        }
      }
      free(node->fingers);
      node->fingers = NULL;
    }

    free(node);
    node = NULL;
  }
}

void check_predecessor_response(int requester_fd) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  CheckPredResponse resp = CHECK_PRED_RESPONSE__INIT;
  void *buf;
  unsigned len;

  msg.version = 417;
  msg.msg_case = CHORD_MESSAGE__MSG_CHECK_PRED_RESP;
  msg.check_pred_resp = &resp;
  
  len = chord_message__get_packed_size(&msg);
  buf = malloc(len);
  if (!buf) {
    perror("Check pred malloc failed\n");
    exit(1);
  }

  chord_message__pack(&msg, buf);

  send_msg(buf, len, chord_node->pred, requester_fd);

  free(buf);
}

int element_of(uint64_t key) {
  int ret = 0;
  printf("Comparing keys:\n");
  printf("Current node's key: %" PRIu64, chord_node->id);
  printf("\nPeer node's key: %" PRIu64, key);
  printf("\n\n");
  if (key <= chord_node->id && key > chord_node->pred->id) {
    ret = 1;
  }
  return ret;
}

void notification_request(int requester_fd, Node *peer) {
  if (!chord_node->pred || element_of(peer->key)){
    Chord *new_pred = (Chord *)malloc(sizeof(Chord));
    struct sockaddr_in peer_addr;

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer->port);
    peer_addr.sin_addr.s_addr = htonl(peer->address); 
    new_pred->id = peer->key;
    new_pred->my_addr = peer_addr;

    chord_node->pred = new_pred;

  // Send notification response??

  } else {
    printf("Notification failed\n");
  }
}

void *fix_fingers() {
  struct timespec ts;
  ts.tv_sec = chord_node->stabilize_period;
  ts.tv_nsec = 0;

  while (1) {
    nanosleep(&ts, NULL);
    printf("Fixing Fingers\n");
  }

  return NULL;
}

void *stabilize() {
  struct timespec ts;
  ts.tv_sec = chord_node->stabilize_period;
  ts.tv_nsec = 0;

  while (1) {
    nanosleep(&ts, NULL);
    printf("Stabilizing\n");
  }

  return NULL;
}

void *check_predecessor() {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  ChordMessage *resp;
  CheckPredRequest req = CHECK_PRED_REQUEST__INIT;
  void *buf; 
  unsigned len;
  int rc;
  uint8_t buffer[1024];
 
  struct sockaddr_in pred_addr;

  struct pollfd fds[1];
  fds[0].events = POLLIN;

  struct timespec ts;
  ts.tv_sec = chord_node->check_predecessor_period;
  ts.tv_nsec = 0;

  msg.version = 417;
  msg.msg_case = CHORD_MESSAGE__MSG_CHECK_PRED_REQ;
  msg.check_pred_req = &req;

  len = chord_message__get_packed_size(&msg);

  buf = malloc(len);
  if (!buf) {
    perror("Check pred malloc failed\n");
    exit(1);
  }

  chord_message__pack(&msg, buf);
   
  while (1) {
    nanosleep(&ts, NULL);
    
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
      perror("socket() failed\n");
      exit(1);
    }

    pred_addr.sin_family = AF_INET;
    pred_addr.sin_port = chord_node->pred->my_addr.sin_port;
    pred_addr.sin_addr = chord_node->pred->my_addr.sin_addr;

    if (connect(socket_fd, (struct sockaddr *)&pred_addr, sizeof(pred_addr)) < 0) {
      perror("connect() failed\n");
      close(socket_fd);
      chord_node->pred = NULL;
    } else {
   
      fds[0].fd = socket_fd;

      send_msg(buf, len, chord_node->pred, socket_fd);

      rc = poll(fds, 1, 5000);
      if (rc == 0) {
        printf("Predecessor check failed, setting predecessor to NULL\n");
        chord_node->pred = NULL;
      } else if (rc < 0) {
        perror("poll() failed\n");
      } else {
        if (fds[0].revents & POLLIN) {
          ssize_t bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
          if (bytes_received > 0) {
            resp = chord_message__unpack(NULL, bytes_received, buffer);
            if (resp == NULL)
            {
              fprintf(stderr, "Error unpacking ChordMessage");
              exit(1);
            }

            if (resp->version == 417) {
              printf("Version confirmed\n");

              if (resp->msg_case == CHORD_MESSAGE__MSG_CHECK_PRED_RESP) {
                printf("Check Pred Response\n");
              } else {
                printf("Incorrect response from predecessor\n");
                chord_node->pred = NULL;
              }
            } else {
              perror("recv() failed\n");
            }

            chord_message__free_unpacked(resp, NULL);
          }
        }

      }
    }
  }

  free(buf); 
  return NULL;
}

void get_predecessor_response(int requester_fd) {
  ChordMessage msg = CHORD_MESSAGE__INIT;
  GetPredResponse resp = GET_PRED_RESPONSE__INIT; 
  Node node = NODE__INIT;
  void *buf;
  unsigned len;

  msg.version = 417;
  msg.msg_case = CHORD_MESSAGE__MSG_GET_PRED_RESP;
  msg.get_pred_resp = &resp;
  
  node.key = chord_node->pred->id;
  node.address = chord_node->pred->my_addr.sin_addr.s_addr;
  node.port = chord_node->pred->my_addr.sin_port;

  msg.get_pred_resp->node = &node;

  len = chord_message__get_packed_size(&msg);
  buf = malloc(len);
  if (!buf) {
    printf("Get pred malloc() failed\n");
    return;
  }

  chord_message__pack(&msg, buf);

  send_msg(buf, len, chord_node->pred, requester_fd);

  free(buf); 
}

void dispatch_chord_message(int messager_fd, uint8_t *raw_message, size_t message_size) {
  ChordMessage *msg;

  printf("Raw message receieved (size %zu):\n", message_size);
  for (size_t i = 0; i < message_size; i++) {
    printf("%02x", raw_message[i]);
  }
  printf("\n");

  msg = chord_message__unpack(NULL, message_size, raw_message);
  if (msg == NULL)
  {
    fprintf(stderr, "Error unpacking ChordMessage");
    exit(1);
  }

  if (msg->version == 417) {
    printf("Version confirmed\n");
    switch (msg->msg_case) {
      case CHORD_MESSAGE__MSG_CHECK_PRED_REQ:
          printf("Check Pred request\n");
          if (msg->check_pred_req) {
            printf("Check Pred Request Confirmed\n");
          }
          check_predecessor_response(messager_fd);
          break;

      case CHORD_MESSAGE__MSG_NOTIFY_REQ:  
          if (msg->notify_req){
            printf("Notification from Peer\n");
            notification_request(messager_fd, msg->notify_req->node);
          } else {
            printf("Missing Notify Node\n");
          }
          break;

      case CHORD_MESSAGE__MSG_GET_PRED_REQ:
          printf("Get Pred Request\n");
          get_predecessor_response(messager_fd);
          break;
        
      case CHORD_MESSAGE__MSG_FIND_SUCC_REQ:
          printf("Find Succ Request\n");
          break;

      default:
          printf("Unknown ChordMessage\n"); 
  
    }
  }

  chord_message__free_unpacked(msg, NULL);

}

void send_msg(void *buf, size_t len, Chord *peer, int messager_fd) {
    ssize_t sent_bytes = send(messager_fd, buf, len, 0);

  if (sent_bytes <= 0) {
    perror("send_msg send error\n");
  } 

}
