#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>

#include "chord/chord.h"

int main() {
  struct sockaddr_in addr;
  char buffer[80];
  int sock, rc;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);

  Chord *test = (Chord *)malloc(sizeof(Chord));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  assert(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) > 0);

  test->my_addr = addr;
  

  ChordMessage msg = CHORD_MESSAGE__INIT;
  CheckPredRequest req = CHECK_PRED_REQUEST__INIT;
  void *buf; 
  unsigned len;

  msg.version = 417;
  msg.check_pred_req = &req;

  len = chord_message__get_packed_size(&msg);

  buf = malloc(len);
  if (!buf) {
    perror("Check pred malloc failed\n");
    exit(1);
  }

  chord_message__pack(&msg, buf);

  send_msg(buf, len, test);

  free(buf);  
  
  rc = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
  assert(rc >= 0);
  
  const char *message = "test";
  rc = send(sock, message, strlen(message), 0);
  assert(rc > 0);
  printf("Message sent to server\n");
  

  ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
  assert(bytes_received > 0);

  buffer[bytes_received] = '\0';
  printf("Response: %s\n", buffer);

  close(sock);
  return 0;
}
