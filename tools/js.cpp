#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../duktape/duktape.h"

#include "../globals.h" // Project globals
#include "netcon.h" // ToolsNetCon
#include "curl.h" // ToolsCURL

// Definitions for this source file
#include "js.h"

// Implements custom functions for the JavaScript context
struct customJS {

  // Imitates the "console" object for printing to stdout
  struct console {
    static duk_ret_t log (duk_context *ctx) {

      int argc = duk_get_top(ctx);

      std::cout << "[JS] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = duk_to_string(ctx, i);
        if (str) std::cout << str << " ";
      }
      std::cout << std::endl;

      return 0;

    }
    static duk_ret_t error (duk_context *ctx) {

      int argc = duk_get_top(ctx);

      std::cerr << "[JS] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = duk_to_string(ctx, i);
        if (str) std::cerr << str << " ";
      }
      std::cerr << std::endl;

      return 0;

    }
  };

  // Implements simple high-level file system operations
  struct fs {
    static duk_ret_t mkdir (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      if (!path) return duk_type_error(ctx, "fs.mkdir: Invalid path argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
      if (std::filesystem::exists(fullPath)) return duk_generic_error(ctx, "fs.mkdir: Path already exists");

      std::filesystem::create_directories(fullPath);
      return 0;

    }
    static duk_ret_t unlink (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      if (!path) return duk_type_error(ctx, "fs.unlink: Invalid path argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
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

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
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

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
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

      const std::filesystem::path fullPathOld = TEMP_DIR / "tempcontent" / oldPath;
      const std::filesystem::path fullPathNew = TEMP_DIR / "tempcontent" / newPath;

      if (!std::filesystem::exists(oldPath)) return duk_generic_error(ctx, "fs.rename: Path does not exist");
      if (std::filesystem::exists(newPath)) return duk_generic_error(ctx, "fs.rename: New path already occupied");

      std::filesystem::rename(oldPath, newPath);
      return 0;

    }
  };

  // Implements an interface for connecting to the Portal 2 telnet console
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
      duk_push_lstring(ctx, output.c_str(), output.length());
      return 1;

    }
  };

  struct download {
    static duk_ret_t file (duk_context *ctx) {

      const char *path = duk_to_string(ctx, 0);
      const char *url = duk_to_string(ctx, 1);
      if (!path) return duk_type_error(ctx, "download.file: Invalid path argument");
      if (!url) return duk_type_error(ctx, "download.file: Invalid URL argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
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
    std::cerr << "JavaScript file " << filePath << " not found." << std::endl;
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

  duk_push_c_function(ctx, customJS::sleep, 1);
  duk_put_global_string(ctx, "sleep");

  // Evaluate the code in the file buffer
  const std::string code = fileBuffer.str();
  duk_push_lstring(ctx, code.c_str(), code.length());
  if (duk_peval(ctx) != 0) {
    std::cerr << "[" << filePath.string() << "] " << duk_safe_to_string(ctx, -1) << std::endl;
  }

  // Clean up the context
  duk_pop(ctx);
  duk_destroy_heap(ctx);

}
