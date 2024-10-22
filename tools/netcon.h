#ifndef TOOLS_NETCON_H
#define TOOLS_NETCON_H

#include <string>

class ToolsNetCon {
  public:
    static int attemptConnection ();
    static void disconnect (int sockfd);
    static int findOpenPort (const int start, const int end);
    static bool sendCommand (int sockfd, std::string command);
    static std::string readConsole (int sockfd, size_t size);
};

#endif
