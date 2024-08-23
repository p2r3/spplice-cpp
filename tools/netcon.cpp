#include <iostream>
#include <cstring>

#include "../globals.h" // Project globals

#ifndef TARGET_WINDOWS
  #include <unistd.h>
  #include <arpa/inet.h>
#else
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

// Definitions for this source file
#include "netcon.h"

// Closes the given socket
void ToolsNetCon::disconnect (int sockfd)
#ifndef TARGET_WINDOWS
{
  close(sockfd);
}
#else
{
  closesocket(sockfd);
  WSACleanup();
}
#endif

// Attempts to connect to the game's telnet console on SPPLICE_NETCON_PORT
int ToolsNetCon::attemptConnection () {

  // Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "Failed to create a network socket for telnet." << std::endl;
    return -1;
  }

  // Point the socket to the game's telnet server
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SPPLICE_NETCON_PORT);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  // Attempt connection to the server
  if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    ToolsNetCon::disconnect(sockfd);
    return -1;
  }

  return sockfd;

}

// Sends a command to the telnet server on the given socket
bool ToolsNetCon::sendCommand (int sockfd, std::string command) {

  // Commands are only processed upon line termination
  command += "\n";

  // Attempt to send data
  if (send(sockfd, command.c_str(), command.length(), 0) < 0) {
    std::cerr << "Failed to send command to telnet server." << std::endl;
    return false;
  }
  return true;

}

// Reads the given amount of bytes from the telnet console on the given socket
std::string ToolsNetCon::readConsole (int sockfd, size_t size) {

  // Initialize an empty buffer
  char buffer[size];
  memset(buffer, 0, size);

  // Attempt to receive data
  int received = recv(sockfd, buffer, size, 0);
  if (received < 0) {
    std::cerr << "Failed to receive data from telnet server." << std::endl;
    return "";
  }

  // Convert the relevant part of the buffer to a string
  return std::string(buffer, received);

}
