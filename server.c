#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
//VISKAS OK, TIK REIKIA PRITAIKYTI IR WINDOWS SISTEMAI
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include "chatroom_utils.h"

#define MAX_CLIENTS 4

void initialize_server(connection_info *server_info, int port)
{
   /*int socket(int domain, int type, int protocol); 
   *The most correct thing to do is to use 
   *AF_INET in your struct sockaddr_in and PF_INET in your call to socket(). - BJ */
  if((server_info->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Failed to create socket");
    exit(1);
  }

  server_info->address.sin_family = AF_INET;
  server_info->address.sin_addr.s_addr = INADDR_ANY;
  server_info->address.sin_port = htons(port);

  //int bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
  if(bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) == -1)
  {
    perror("Binding failed");
    exit(1);
  }

  const int optVal = 1; //true
  const socklen_t optLen = sizeof(optVal);
  // SO_REUSEADDR - Allows other sockets to bind() to this port,unless there is an 
  // active listening socket bound to the portal ready. This enables you to get around 
  // those “Address already in use” error messages when you try to restart your server after acrash.
  // int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
  if(setsockopt(server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) == -1)
  {
    perror("Set socket option failed");
    exit(1);
  }

  //int listen(int s, int backlog);
  if(listen(server_info->socket, MAX_CLIENTS) == -1) {
    perror("Listen failed");
    exit(1);
  }

  printf("Waiting for incoming connections...\n");
}

void send_public_message(connection_info clients[], int sender, char *message_text)
{
  message msg;
  msg.type = PUBLIC_MESSAGE;
  strncpy(msg.username, clients[sender].username, 20);
  strncpy(msg.data, message_text, 256);
  
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(i != sender && clients[i].socket != 0)
    {
	  //ssize_t send(int s, const void *buf, size_t len, int flags); 0-"normal" data
	  //sendto for SOCK_DGRAM
      if(send(clients[i].socket, &msg, sizeof(msg), 0) == -1)
      {
          perror("Send failed");
          exit(1);
      }
    }
  }
}

void send_private_message(connection_info clients[], int sender, char *username, char *message_text)
{
  message msg;
  msg.type = PRIVATE_MESSAGE;
  strncpy(msg.username, clients[sender].username, 20);
  strncpy(msg.data, message_text, 256);

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(i != sender && clients[i].socket != 0 && strcmp(clients[i].username, username) == 0)
    {
      if(send(clients[i].socket, &msg, sizeof(msg), 0) == -1)
      {
          perror("Send failed");
          exit(1);
      }
      return;
    }
  }

  msg.type = USERNAME_ERROR;
  sprintf(msg.data, "Username \"%s\" does not exist or is not logged in.", username);

  if(send(clients[sender].socket, &msg, sizeof(msg), 0) == -1)
  {
      perror("Send failed");
      exit(1);
  }
}

void send_connect_message(connection_info *clients, int sender)
{
  message msg;
  msg.type = CONNECT;
  strncpy(msg.username, clients[sender].username, 21);

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket != 0)
    {
      if(i == sender)
      {
        msg.type = SUCCESS;
        if(send(clients[i].socket, &msg, sizeof(msg), 0) == -1)
        {
            perror("Send failed");
            exit(1);
        }
      }
	  else 
	  {
        if(send(clients[i].socket, &msg, sizeof(msg), 0) == -1)
        {
            perror("Send failed");
            exit(1);
        }
      }
    }
  }
}

void send_disconnect_message(connection_info *clients, char *username)
{
  message msg;
  msg.type = DISCONNECT;
  strncpy(msg.username, username, 21);

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket != 0)
    {
      if(send(clients[i].socket, &msg, sizeof(msg), 0) == -1)
      {
          perror("Send failed");
          exit(1);
      }
    }
  }
}

void send_too_full_message(int socket)
{
  message too_full_message;
  too_full_message.type = TOO_FULL;

  if(send(socket, &too_full_message, sizeof(too_full_message), 0) == -1)
  {
      perror("Send failed");
      exit(1);
  }

  close(socket);
}

//close all the sockets before exiting
void stop_server(connection_info connection[])
{
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    close(connection[i].socket);
  }
  exit(0);
}

void handle_client_message(connection_info clients[], int sender)
{
  int read_size;
  message msg;

  //ssize_t recv(int s, void *buf, size_t len, int flags); 
  if((read_size = recv(clients[sender].socket, &msg, sizeof(message), 0)) == 0)
  {
    printf("User disconnected: %s.\n", clients[sender].username);
    close(clients[sender].socket);
    clients[sender].socket = 0;
    send_disconnect_message(clients, clients[sender].username);

  } 
  else {

    switch(msg.type)
    {
      case SET_USERNAME:
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
          if(clients[i].socket != 0 && strcmp(clients[i].username, msg.username) == 0)
          {
            close(clients[sender].socket);
            clients[sender].socket = 0;
            return;
          }
        }

        strcpy(clients[sender].username, msg.username);
        printf("User connected: %s\n", clients[sender].username);
        send_connect_message(clients, sender);
      break;

      case PUBLIC_MESSAGE:
        send_public_message(clients, sender, msg.data);
      break;

      case PRIVATE_MESSAGE:
        send_private_message(clients, sender, msg.username, msg.data);
      break;

      default:
        fprintf(stderr, "Unknown message type received.\n");
      break;
    }
  }
}

int construct_fd_set(fd_set *set, connection_info *server_info,
                      connection_info clients[])
{
  FD_ZERO(set); //Clear all entries from the set.
  FD_SET(STDIN_FILENO, set); 
  FD_SET(server_info->socket, set); //Add fd to the set. 

  int max_fd = server_info->socket;
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket > 0)
    {
      FD_SET(clients[i].socket, set);
      if(clients[i].socket > max_fd)
      {
        max_fd = clients[i].socket;
      }
    }
  }
  return max_fd;
}

void handle_new_connection(connection_info *server_info, connection_info clients[])
{
  int address_len;
  int new_socket = accept(server_info->socket, (struct sockaddr*)&server_info->address, (socklen_t*)&address_len);

  if (new_socket == -1)
  {
    perror("Accept Failed");
    exit(1);
  }

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket == 0) {
      clients[i].socket = new_socket;
      break;

    } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
    {
      send_too_full_message(new_socket);
    }
  }
}

void handle_user_input(connection_info clients[])
{
  char input[255];
  fgets(input, sizeof(input), stdin);
  trim_newline(input);

  if(input[0] == 'q') {
    stop_server(clients);
  }
}

int main(int argc, char *argv[])
{
  puts("Starting server.");

  fd_set file_descriptors;

  connection_info server_info;
  connection_info clients[MAX_CLIENTS];

  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].socket = 0;
  }

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  initialize_server(&server_info, atoi(argv[1]));

  while(true)
  {
    int max_fd = construct_fd_set(&file_descriptors, &server_info, clients);

	//int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	
    if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) == -1)
    {
      perror("Select Failed");
      stop_server(clients);
    }

	// Return true if fd is in the set.
    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      handle_user_input(clients);
    }

    if(FD_ISSET(server_info.socket, &file_descriptors))
    {
      handle_new_connection(&server_info, clients);
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
      if(clients[i].socket > 0 && FD_ISSET(clients[i].socket, &file_descriptors))
      {
        handle_client_message(clients, i);
      }
    }
  }

  return 0;
}
