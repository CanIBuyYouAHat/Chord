#ifndef CHORD_H
#define CHORD_H

#include <inttypes.h>
#include <arpa/inet.h>
#include <argp.h>

#include "../ht/ht.h"
#include "../protobuf/chord.pb-c.h"

#define KEY_LEN = 8;

typedef struct Message 
{
  uint64_t len;
  void *ChordMessage;
} Message;

struct Chord;

void printKey(uint64_t key);

typedef struct Chord
{
  uint8_t num_successors;
  uint16_t stabilize_period;
  uint16_t fix_fingers_period;
  uint16_t check_predecessor_period;
  struct sockaddr_in my_addr;
  struct sockaddr_in join_addr;
  
  int socket_fd;

  uint64_t id;

  struct Chord *pred;
  struct Chord *succ;

  struct Chord *fingers;
} Chord;

extern Chord *chord_node;

error_t chord_parser(int key, char *arg, struct argp_state *state);

Chord *chord_constructor(int argc, char *argv[]);

void get_id();

void cleanup_node(Chord *node);

void handle_signal(int signal);

void check_predecessor_response(int requester_fd);

void notification_request(int requester_fd, Node *peer);

void *fix_fingers();

void *stabilize();

void *check_predecessor();

void get_predecessor_response(int requester_fd);

void dispatch_chord_message(int messager_fd, uint8_t *raw_message, size_t message_size);

void send_msg(void *buf, size_t len, Chord *peer, int messager_fd);


#endif
