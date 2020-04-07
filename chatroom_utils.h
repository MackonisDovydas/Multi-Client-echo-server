#ifndef CHATROOM_UTILS_H_
#define CHATROOM_UTILS_H_

#ifdef __unix__
#define OS_Windows 0
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define OS_Windows 1
#include <Winsock2.h>
#endif

#include <stdio.h>

//Enum of different messages possible.
typedef enum
{
  CONNECT,
  DISCONNECT,
  SET_USERNAME,
  PUBLIC_MESSAGE,
  PRIVATE_MESSAGE,
  TOO_FULL,
  USERNAME_ERROR,
  SUCCESS,
  ERRORS //check if works on linux else ERROR

} message_type;


//message structure
typedef struct
{
  message_type type;
  char username[21];
  char data[256];

} message;

//structure to hold client connection information
typedef struct connection_info
{
  int socket;
  struct sockaddr_in address;
  char username[20];
} connection_info;


// Removes the trailing newline character from a string.
void trim_newline(char *text);

// discard any remaining data on stdin buffer.
void clear_stdin_buffer();

#endif
