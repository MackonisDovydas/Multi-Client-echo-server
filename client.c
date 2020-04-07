#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#ifdef __unix__
#define OS_Windows 0
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define OS_Windows 1
#include <Winsock2.h>
#endif

#include "chatroom_utils.h"

// get a username from the user.
void get_username(char *username)
{
  while(true)
  {
    printf("Enter a username: ");
    fflush(stdout);
    memset(username, 0, 1000);
    fgets(username, 22, stdin);
    trim_newline(username);

    if(strlen(username) > 20)
    {
      puts("Username must be 20 characters or less.");
    }
	else {
      break;
    }
  }
}

//send local username to the server.
void set_username(connection_info *connection)
{
  message msg;
  msg.type = SET_USERNAME;
  strncpy(msg.username, connection->username, 20);

  if(send(connection->socket, (void*)&msg, sizeof(msg), 0) == -1)
  {
    perror("Send failed");
    exit(1);
  }
}

void stop_client(connection_info *connection)
{
  close(connection->socket);
  exit(0);
}

//initialize connection to the server.
void connect_to_server(connection_info *connection, char *address, char *port)
{
  while(true)
  {
    get_username(connection->username);

    //Create socket
	//int socket(int domain, int type, int protocol);
    if ((connection->socket = socket(PF_INET, SOCK_STREAM ,0)) == -1)
    {
        perror("Could not create socket");
    }

    connection->address.sin_addr.s_addr = inet_addr(address);
    connection->address.sin_family = AF_INET;
    connection->address.sin_port = htons(atoi(port));

    //Connect to remote server
	//int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
    if (connect(connection->socket, (struct sockaddr *)&connection->address , sizeof(connection->address)) == -1)
    {
        perror("Connect failed.");
        exit(1);
    }

    set_username(connection);

    message msg;
	ssize_t recv_val = recv(connection->socket, (void*)&msg, sizeof(message), 0);
    //ssize_t recv_val = recv(connection->socket, &msg, sizeof(message), 0);
    if(recv_val < 0)
    {
        perror("recv failed");
        exit(1);

    }
    else if(recv_val == 0)
    {
      close(connection->socket);
      printf("The username \"%s\" is taken, please try another name.\n", connection->username);
      continue;
    }
    break;
  }

  puts("Connected to server.");
  puts("/quit or /q: Exit the program.");
  puts("/m <username> <message> Send a private message to given username.");
}


void handle_user_input(connection_info *connection)
{
  char input[255];
  fgets(input, 255, stdin);
  trim_newline(input);

  if(strcmp(input, "/q") == 0 || strcmp(input, "/quit") == 0)
  {
    stop_client(connection);
  }
  else if(strncmp(input, "/m", 2) == 0)
  {
    message msg;
    msg.type = PRIVATE_MESSAGE;

    char *toUsername, *chatMsg;

    toUsername = strtok(input+3, " ");

    if(toUsername == NULL)
    {
      puts("The format for private messages is: /m <username> <message>");
      return;
    }

    if(strlen(toUsername) == 0)
    {
      puts("You must enter a username for a private message." );
      return;
    }

    if(strlen(toUsername) > 20)
    {
      puts("The username must be between 1 and 20 characters.");
      return;
    }

    chatMsg = strtok(NULL, "");

    if(chatMsg == NULL)
    {
      puts("You must enter a message to send to the specified user.");
      return;
    }

    strncpy(msg.username, toUsername, 20);
    strncpy(msg.data, chatMsg, 255);

    if(send(connection->socket, (void*)&msg, sizeof(message), 0) == -1)
    {
        perror("Send failed");
        exit(1);
    }

  }
  else //regular public message
  {
    message msg;
    msg.type = PUBLIC_MESSAGE;
    strncpy(msg.username, connection->username, 20);

    if(strlen(input) == 0) {
        return;
    }

    strncpy(msg.data, input, 255);

    //Send some data
    if(send(connection->socket, (void*)&msg, sizeof(message), 0) == -1)
    {
        perror("Send failed");
        exit(1);
    }
  }
}

void handle_server_message(connection_info *connection)
{
  message msg;

  //Receive a reply from the server
  ssize_t recv_val = recv(connection->socket, (void*)&msg, sizeof(message), 0);
  if(recv_val == -1)
  {
      perror("recv failed");
      exit(1);

  }
  else if(recv_val == 0)
  {
    close(connection->socket);
    puts("Server disconnected.");
    exit(0);
  }

  switch(msg.type)
  {
    case CONNECT:
      printf("%s has connected.\n", msg.username);
    break;

    case DISCONNECT:
      printf("%s has disconnected.\n" , msg.username);
    break;

    case PUBLIC_MESSAGE:
      printf("%s: %s\n", msg.username, msg.data);
    break;

    case PRIVATE_MESSAGE:
      printf("From %s: %s\n", msg.username, msg.data);
    break;

    case TOO_FULL:
      fprintf(stderr, "Server chatroom is too full to accept new clients.\n");
      exit(0);
    break;

    default:
      fprintf(stderr, "Unknown message type received.\n");
    break;
  }
}

int main(int argc, char *argv[])
{
  connection_info connection;
  fd_set file_descriptors;

  if (argc != 3) {
    fprintf(stderr,"Usage: %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  connect_to_server(&connection, argv[1], argv[2]);

  //keep communicating with server
  while(true)
  {
    FD_ZERO(&file_descriptors); //Clear all entries from the set
    FD_SET(STDIN_FILENO, &file_descriptors); // Add fd to the set.
    FD_SET(connection.socket, &file_descriptors);
    fflush(stdin);

    if(select(connection.socket+1, &file_descriptors, NULL, NULL, NULL) == -1)
    {
      perror("Select failed.");
      exit(1);
    }

	// Return true if fd is in the set.
    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      handle_user_input(&connection);
    }

    if(FD_ISSET(connection.socket, &file_descriptors))
    {
      handle_server_message(&connection);
    }
  }

  close(connection.socket);
  return 0;
}
