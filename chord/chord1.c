#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>


#include "chord_arg_parser.c"

#define SERVER_PORT 12345

#define TRUE 1
#define FAlSE 0

main (int argc, char *argv[])
{
  struct chord_arguments chord = chord_constructor(argc, argv);

    
    

}

struct chord_agruments lookup(char *value) 
{
  // Hash value, get key
  // Lookup
}

