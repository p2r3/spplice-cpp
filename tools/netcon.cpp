#include <iostream>
#include <cstring>

#include "../globals.h" // Project globals

#ifndef TARGET_WINDOWS
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <poll.h>
#else
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

// Definitions for this source file
#include "netcon.h"

// Counts the amount of open netcon sockets for setup and cleanup
// I have absolutely no idea why you'd ever need more than one, but oh well...
int openSockets = 0;

// Closes the given socket
void ToolsNetCon::disconnect (int sockfd)
#ifndef TARGET_WINDOWS
{
  close(sockfd);
}
#else
{
  closesocket(sockfd);
  // Clean up Winsock if all sockets have closed
  if (--openSockets == 0) WSACleanup();
}
#endif

// Attempts to connect to the game's telnet console on SPPLICE_NETCON_PORT
int ToolsNetCon::attemptConnection () {

  // If this is the first socket we've made, set up Winsock
  if (++openSockets == 1) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
      std::cerr << "WSAStartup failed." << std::endl;
      return -1;
    }
  }

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

// Close our eyes and pretend Windows poll is the same as Linux
#ifdef TARGET_WINDOWS
  #define pollfd(a) WSAPOLLFD(a)
  #define poll(a, b, c) WSAPoll(a, b, c)
#else
  #define SOCKET_ERROR -1
#endif

// Reads the given amount of bytes from the telnet console on the given socket
std::string ToolsNetCon::readConsole (int sockfd, size_t size) {

  // Check if data is available for reading
  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLIN;

  // Timeout of 0, don't wait for data to become available
  int result = poll(&pfd, 1, 0);

  // Handle the poll result
  if (result == SOCKET_ERROR) {
    std::cerr << "Failed to receive data from telnet server." << std::endl;
    return "";
  } else if (result == 0 || !(pfd.revents & POLLIN)) {
    return "";
  }

  // Initialize an empty buffer
  char buffer[size];
  memset(buffer, 0, size);

  // Attempt to receive data
  int received = recv(sockfd, buffer, size, 0);
  if (received <= 0) {
    std::cerr << "Failed to receive data from telnet server." << std::endl;
    return "";
  }

  // Convert the relevant part of the buffer to a string
  return std::string(buffer, received);

}