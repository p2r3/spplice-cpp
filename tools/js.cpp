#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../deps/shared/duktape/duktape.h"

#include "../globals.h" // Project globals

#ifndef TARGET_WINDOWS
  #include "../deps/linux/include/curl/curl.h"
#else
  #include "../deps/win32/include/curl/curl.h"
  #include <windows.h>
#endif

#include "netcon.h" // ToolsNetCon
#include "curl.h" // ToolsCURL

// Definitions for this source file
#include "js.h"

// Max amount of WebSockets the JS environment can allocate
#define MAX_WEBSOCKETS 32
// Contains all WebSockets created in the JS environment
CURL *webSockets[MAX_WEBSOCKETS];

// Implements custom functions for the JavaScript context
struct customJS {

  // Imitates the "console" object for printing to stdout
  struct console {
    static duk_ret_t log (duk_context *ctx) {

      int argc = duk_get_top(ctx);

      LOGFILE << "[JS I] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = duk_to_string(ctx, i);
        if (str) LOGFILE << str << " " << std::endl;
      }
      LOGFILE << std::endl;

      return 0;

    }
    static duk_ret_t error (duk_context *ctx) {

      int argc = duk_get_top(ctx);

      LOGFILE << "[JS E] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = duk_to_string(ctx, i);
        if (str) LOGFILE << str << " ";
      }
      LOGFILE << std::endl;

      return 0;

    }
  };

  // Implements simple high-level file system operations
  struct fs {
    static duk_ret_t mkdir (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      if (!path) return duk_type_error(ctx, "fs.mkdir: Invalid path argument");

      // Normalize base path and input path
      const std::filesystem::path basePath = std::filesystem::absolute(CACHE_DIR / "tempcontent");
      const std::filesystem::path fullPath = std::filesystem::absolute(basePath / path);
      // Check for path traversal
      if (fullPath.string().find(basePath.string()) != 0) {
        return duk_generic_error(ctx, "fs.mkdir: Path traversal detected");
      }

      if (std::filesystem::exists(fullPath)) return duk_generic_error(ctx, "fs.mkdir: Path already exists");

      std::filesystem::create_directories(fullPath);
      return 0;

    }
    static duk_ret_t unlink (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      if (!path) return duk_type_error(ctx, "fs.unlink: Invalid path argument");

      // Normalize base path and input path
      const std::filesystem::path basePath = std::filesystem::absolute(CACHE_DIR / "tempcontent");
      const std::filesystem::path fullPath = std::filesystem::absolute(basePath / path);
      // Check for path traversal
      if (fullPath.string().find(basePath.string()) != 0) {
        return duk_generic_error(ctx, "fs.unlink: Path traversal detected");
      }

      if (!std::filesystem::exists(fullPath)) return duk_generic_error(ctx, "fs.unlink: Path does not exist");

      if (std::filesystem::is_directory(fullPath)) {
        std::filesystem::remove_all(fullPath);
      } else {
        std::filesystem::remove(fullPath);
      }
      return 0;

    }
    static duk_ret_t read (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      if (!path) return duk_type_error(ctx, "fs.read: Invalid path argument");

      // Normalize base path and input path
      // For fs.read, the base path is GAME_DIR to allow for reading other game files
      const std::filesystem::path basePath = std::filesystem::absolute(GAME_DIR);
      const std::filesystem::path fullPath = std::filesystem::absolute((basePath / (SPPLICE_STEAMAPP_DIRS[SPPLICE_STEAMAPP_INDEX] + "_tempcontent")) / path);
      // Check for path traversal
      if (fullPath.string().find(basePath.string()) != 0) {
        return duk_generic_error(ctx, "fs.read: Path traversal detected");
      }

      if (!std::filesystem::exists(fullPath)) return duk_generic_error(ctx, "fs.read: File does not exist");
      if (!std::filesystem::is_regular_file(fullPath)) return duk_generic_error(ctx, "fs.read: Path is not a file");

      std::ifstream fileStream(fullPath);
      std::stringstream fileBuffer;
      fileBuffer << fileStream.rdbuf();
      fileStream.close();

      std::string fileString = fileBuffer.str();
      duk_push_lstring(ctx, fileString.c_str(), fileString.length());
      return 1;

    }
    static duk_ret_t write (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      const char *contents = duk_to_string(ctx, 1);
      if (!path) return duk_type_error(ctx, "fs.write: Invalid path argument");
      if (!contents) return duk_type_error(ctx, "fs.write: Invalid contents argument");

      // Normalize base path and input path
      const std::filesystem::path basePath = std::filesystem::absolute(CACHE_DIR / "tempcontent");
      const std::filesystem::path fullPath = std::filesystem::absolute(basePath / path);
      // Check for path traversal
      if (fullPath.string().find(basePath.string()) != 0) {
        return duk_generic_error(ctx, "fs.write: Path traversal detected");
      }

      if (std::filesystem::exists(fullPath) && !std::filesystem::is_regular_file(fullPath)) {
        return duk_generic_error(ctx, "fs.write: Path already exists and is not a file");
      }

      std::ofstream fileStream(fullPath);
      fileStream << contents;
      fileStream.close();

      return 0;

    }
    static duk_ret_t rename (duk_context *ctx) {

      const char *oldPath = duk_to_string(ctx, 0);
      const char *newPath = duk_to_string(ctx, 1);
      if (!oldPath) return duk_type_error(ctx, "fs.rename: Invalid oldPath argument");
      if (!newPath) return duk_type_error(ctx, "fs.rename: Invalid newPath argument");

      // Normalize base path and input paths
      const std::filesystem::path basePath = std::filesystem::absolute(CACHE_DIR / "tempcontent");
      const std::filesystem::path fullPathOld = std::filesystem::absolute(basePath / oldPath);
      const std::filesystem::path fullPathNew = std::filesystem::absolute(basePath / newPath);
      // Check for path traversal
      if (
        fullPathOld.string().find(basePath.string()) != 0 ||
        fullPathNew.string().find(basePath.string()) != 0
      ) {
        return duk_generic_error(ctx, "fs.rename: Path traversal detected");
      }

      if (!std::filesystem::exists(fullPathOld)) return duk_generic_error(ctx, "fs.rename: Path does not exist");
      if (std::filesystem::exists(fullPathNew)) return duk_generic_error(ctx, "fs.rename: New path already occupied");

      std::filesystem::rename(fullPathOld, fullPathNew);
      return 0;

    }
  };

  // Implements an interface for connecting to the Portal 2 TCP console
  struct game {
    static duk_ret_t connect (duk_context *ctx) {

      int sockfd = ToolsNetCon::attemptConnection();
      duk_push_number(ctx, sockfd);
      return 1;

    }
    static duk_ret_t disconnect (duk_context *ctx) {

      int sockfd = duk_to_int(ctx, 0);
      if (sockfd < 0) return duk_type_error(ctx, "game.disconnect: Invalid socket provided");

      ToolsNetCon::disconnect(sockfd);
      return 0;

    }
    static duk_ret_t send (duk_context *ctx) {

      int sockfd = duk_to_int(ctx, 0);
      if (sockfd < 0) return duk_type_error(ctx, "game.send: Invalid socket provided");

      const char *command = duk_to_string(ctx, 1);

      if (!ToolsNetCon::sendCommand(sockfd, command)) {
        return duk_generic_error(ctx, "game.send: Failed to send command");
      }
      return 0;

    }
    static duk_ret_t read (duk_context *ctx) {

      int sockfd = duk_to_int(ctx, 0);
      if (sockfd < 0) return duk_type_error(ctx, "game.read: Invalid socket provided");

      size_t size = duk_to_uint(ctx, 1);
      if (!size) size = 1024;

      const std::string output = ToolsNetCon::readConsole(sockfd, size);

      if (output == "\x04") {
        return duk_generic_error(ctx, "game.read: Failed to read from socket");
      }

      duk_push_lstring(ctx, output.c_str(), output.length());
      return 1;

    }
  };

  // Implements a WebSocket client interface
  struct ws {
    static duk_ret_t connect (duk_context *ctx) {

      const char *url = duk_to_string(ctx, 0);
      if (!url) return duk_type_error(ctx, "ws.connect: Invalid URL provided");

      int sockid = 0;
      while (sockid < MAX_WEBSOCKETS) {
        if (!webSockets[sockid]) break;
        sockid ++;
      }
      if (sockid == MAX_WEBSOCKETS) {
        return duk_type_error(ctx, "ws.connect: Too many WebSocket connections");
      }

      // Attempt connection
      CURL *newSocket = ToolsCURL::wsConnect(url);

      // If the connection failed, return null
      if (!newSocket) {
        duk_push_null(ctx);
        return 1;
      }

      // Add the newly created socket to the array
      webSockets[sockid] = newSocket;

      // Add 1 to sockid so that a good socket wouldn't be falsy
      duk_push_number(ctx, sockid + 1);
      return 1;

    }
    static duk_ret_t disconnect (duk_context *ctx) {

      int sockid = duk_to_int(ctx, 0) - 1;
      if (sockid < 0 || sockid >= MAX_WEBSOCKETS || !webSockets[sockid]) {
        return duk_type_error(ctx, "ws.disconnect: Invalid WebSocket provided");
      }

      // Close the socket
      ToolsCURL::wsDisconnect(webSockets[sockid]);
      // Free the socket ID
      webSockets[sockid] = nullptr;

      return 0;

    }
    static duk_ret_t send (duk_context *ctx) {

      int sockid = duk_to_int(ctx, 0) - 1;
      if (sockid < 0 || sockid >= MAX_WEBSOCKETS || !webSockets[sockid]) {
        return duk_type_error(ctx, "ws.send: Invalid WebSocket provided");
      }

      const char *data = duk_to_string(ctx, 1);
      if (!data) return duk_type_error(ctx, "ws.send: Invalid data provided");

      duk_push_boolean(ctx, ToolsCURL::wsSend(webSockets[sockid], data));
      return 1;

    }
    static duk_ret_t read (duk_context *ctx) {

      int sockid = duk_to_int(ctx, 0) - 1;
      if (sockid < 0 || sockid >= MAX_WEBSOCKETS || !webSockets[sockid]) {
        return duk_type_error(ctx, "ws.read: Invalid WebSocket provided");
      }

      size_t size = duk_to_uint(ctx, 1);
      if (!size) size = 1024;

      const std::string data = ToolsCURL::wsReceive(webSockets[sockid], size);

      duk_push_lstring(ctx, data.c_str(), data.length());
      return 1;

    }
  };

  struct download {
    static duk_ret_t file (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      const char *url = duk_to_string(ctx, 1);
      if (!path) return duk_type_error(ctx, "download.file: Invalid path argument");
      if (!url) return duk_type_error(ctx, "download.file: Invalid URL argument");

      const std::filesystem::path fullPath = CACHE_DIR / "tempcontent" / path;
      if (std::filesystem::exists(path)) return duk_generic_error(ctx, "download.file: Path already occupied");

      if (!ToolsCURL::downloadFile(url, fullPath)) {
        return duk_generic_error(ctx, "download.file: Download failed");
      }
      return 0;

    }
    static duk_ret_t string (duk_context *ctx) {

      const char *url = duk_to_string(ctx, 0);
      if (!url) return duk_type_error(ctx, "download.string: Invalid URL argument");

      std::string output = ToolsCURL::downloadString(url);
      duk_push_lstring(ctx, output.c_str(), output.length());
      return 1;

    }
  };

  static duk_ret_t sleep (duk_context *ctx) {

    size_t time = duk_to_uint(ctx, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(time));
    return 0;

  }

};

void ToolsJS::runFile (const std::filesystem::path &filePath) {

  // Check if the input file exists
  if (!std::filesystem::exists(filePath)) {
    LOGFILE << "[E] JavaScript file " << filePath << " not found." << std::endl;
    return;
  }

  // Read the input file into a string buffer
  std::ifstream fileStream(filePath);
  std::stringstream fileBuffer;
  fileBuffer << fileStream.rdbuf();
  fileStream.close();
  // Create a new Duktape context
  duk_context *ctx = duk_create_heap_default();
  // Register custom functions
  duk_idx_t obj_idx;

  obj_idx = duk_push_object(ctx);
  duk_push_c_function(ctx, customJS::console::log, DUK_VARARGS);
  duk_put_prop_string(ctx, obj_idx, "log");
  duk_push_c_function(ctx, customJS::console::error, DUK_VARARGS);
  duk_put_prop_string(ctx, obj_idx, "error");
  duk_put_global_string(ctx, "console");

  obj_idx = duk_push_object(ctx);
  duk_push_c_function(ctx, customJS::fs::mkdir, 1);
  duk_put_prop_string(ctx, obj_idx, "mkdir");
  duk_push_c_function(ctx, customJS::fs::unlink, 1);
  duk_put_prop_string(ctx, obj_idx, "unlink");
  duk_push_c_function(ctx, customJS::fs::read, 1);
  duk_put_prop_string(ctx, obj_idx, "read");
  duk_push_c_function(ctx, customJS::fs::write, 2);
  duk_put_prop_string(ctx, obj_idx, "write");
  duk_push_c_function(ctx, customJS::fs::rename, 2);
  duk_put_prop_string(ctx, obj_idx, "rename");
  duk_put_global_string(ctx, "fs");

  obj_idx = duk_push_object(ctx);
  duk_push_c_function(ctx, customJS::game::connect, 0);
  duk_put_prop_string(ctx, obj_idx, "connect");
  duk_push_c_function(ctx, customJS::game::disconnect, 1);
  duk_put_prop_string(ctx, obj_idx, "disconnect");
  duk_push_c_function(ctx, customJS::game::send, 2);
  duk_put_prop_string(ctx, obj_idx, "send");
  duk_push_c_function(ctx, customJS::game::read, 2);
  duk_put_prop_string(ctx, obj_idx, "read");
  duk_put_global_string(ctx, "game");

  obj_idx = duk_push_object(ctx);
  duk_push_c_function(ctx, customJS::download::file, 2);
  duk_put_prop_string(ctx, obj_idx, "file");
  duk_push_c_function(ctx, customJS::download::string, 1);
  duk_put_prop_string(ctx, obj_idx, "string");
  duk_put_global_string(ctx, "download");

  obj_idx = duk_push_object(ctx);
  duk_push_c_function(ctx, customJS::ws::connect, 1);
  duk_put_prop_string(ctx, obj_idx, "connect");
  duk_push_c_function(ctx, customJS::ws::disconnect, 1);
  duk_put_prop_string(ctx, obj_idx, "disconnect");
  duk_push_c_function(ctx, customJS::ws::send, 2);
  duk_put_prop_string(ctx, obj_idx, "send");
  duk_push_c_function(ctx, customJS::ws::read, 2);
  duk_put_prop_string(ctx, obj_idx, "read");
  duk_put_global_string(ctx, "ws");

  duk_push_c_function(ctx, customJS::sleep, 1);
  duk_put_global_string(ctx, "sleep");

  // Evaluate the code in the file buffer
  const std::string code = fileBuffer.str();
  duk_push_lstring(ctx, code.c_str(), code.length());
  if (duk_peval(ctx) != 0) {
    LOGFILE << "[E] [" << filePath << "] " << duk_safe_to_string(ctx, -1) << std::endl;
  }

  // Clean up the context
  duk_pop(ctx);
  duk_destroy_heap(ctx);

}
