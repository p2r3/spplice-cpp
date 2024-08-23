#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../quickjs/quickjs.h"

#include "../globals.h" // Project globals
#include "netcon.h" // ToolsNetCon

// Definitions for this source file
#include "js.h"

// Implements custom functions for the JavaScript context
struct customJS {

  // Imitates the "console" object for printing to stdout
  struct console {
    static JSValue log (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      std::cout << "[JS] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (!str) continue;

        std::cout << str << " ";
        JS_FreeCString(ctx, str);
      }
      std::cout << std::endl;

      return JS_UNDEFINED;

    }
    static JSValue error (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      std::cerr << "[JS] ";
      for (int i = 0; i < argc; i ++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (!str) continue;

        std::cerr << str << " ";
        JS_FreeCString(ctx, str);
      }
      std::cerr << std::endl;

      return JS_UNDEFINED;

    }
  };

  // Implements simple high-level file system operations
  struct fs {
    static JSValue mkdir (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      const char *path = JS_ToCString(ctx, argv[0]);
      if (!path) return JS_ThrowTypeError(ctx, "fs.mkdir: Invalid path argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
      if (std::filesystem::exists(fullPath)) return JS_ThrowInternalError(ctx, "fs.mkdir: Path already exists");

      std::filesystem::create_directories(fullPath);
      return JS_UNDEFINED;

    }
    static JSValue unlink (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      const char *path = JS_ToCString(ctx, argv[0]);
      if (!path) return JS_ThrowTypeError(ctx, "fs.unlink: Invalid path argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
      if (!std::filesystem::exists(fullPath)) return JS_ThrowInternalError(ctx, "fs.unlink: Path does not exist");

      if (std::filesystem::is_directory(fullPath)) {
        std::filesystem::remove_all(fullPath);
      } else {
        std::filesystem::remove(fullPath);
      }
      return JS_UNDEFINED;

    }
    static JSValue read (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      const char *path = JS_ToCString(ctx, argv[0]);
      if (!path) return JS_ThrowTypeError(ctx, "fs.read: Invalid path argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
      if (!std::filesystem::exists(fullPath)) return JS_ThrowInternalError(ctx, "fs.read: File does not exist");
      if (!std::filesystem::is_regular_file(fullPath)) return JS_ThrowInternalError(ctx, "fs.read: Path is not a file");

      std::ifstream fileStream(fullPath);
      std::stringstream fileBuffer;
      fileBuffer << fileStream.rdbuf();
      fileStream.close();

      return JS_NewString(ctx, fileBuffer.str().c_str());

    }
    static JSValue write (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      const char *path = JS_ToCString(ctx, argv[0]);
      const char *contents = JS_ToCString(ctx, argv[1]);
      if (!path) return JS_ThrowTypeError(ctx, "fs.write: Invalid path argument");
      if (!contents) return JS_ThrowTypeError(ctx, "fs.write: Invalid contents argument");

      const std::filesystem::path fullPath = TEMP_DIR / "tempcontent" / path;
      if (std::filesystem::exists(fullPath) && !std::filesystem::is_regular_file(fullPath)) {
        return JS_ThrowInternalError(ctx, "fs.write: Path already exists and is not a file");
      }

      std::ofstream fileStream(fullPath);
      fileStream << contents;
      fileStream.close();

      return JS_UNDEFINED;

    }
    static JSValue rename (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      const char *oldPath = JS_ToCString(ctx, argv[0]);
      const char *newPath = JS_ToCString(ctx, argv[1]);
      if (!oldPath) return JS_ThrowTypeError(ctx, "fs.rename: Invalid oldPath argument");
      if (!newPath) return JS_ThrowTypeError(ctx, "fs.rename: Invalid newPath argument");

      const std::filesystem::path fullPathOld = TEMP_DIR / "tempcontent" / oldPath;
      const std::filesystem::path fullPathNew = TEMP_DIR / "tempcontent" / newPath;

      if (!std::filesystem::exists(oldPath)) return JS_ThrowInternalError(ctx, "fs.rename: Path does not exist");
      if (std::filesystem::exists(newPath)) return JS_ThrowInternalError(ctx, "fs.rename: New path already occupied");

      std::filesystem::rename(oldPath, newPath);
      return JS_UNDEFINED;

    }
  };

  // Implements an interface for connecting to the Portal 2 telnet console
  struct game {
    static JSValue connect (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      int sockfd = ToolsNetCon::attemptConnection();
      return JS_NewInt32(ctx, sockfd);

    }
    static JSValue disconnect (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      int sockfd;
      JS_ToInt32(ctx, &sockfd, argv[0]);
      ToolsNetCon::disconnect(sockfd);
      return JS_UNDEFINED;

    }
    static JSValue send (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      int sockfd;
      JS_ToInt32(ctx, &sockfd, argv[0]);
      if (sockfd == -1) JS_ThrowTypeError(ctx, "game.send: Invalid socket provided");

      const char *command = JS_ToCString(ctx, argv[1]);
      if (!ToolsNetCon::sendCommand(sockfd, command)) {
        return JS_ThrowInternalError(ctx, "game.send: Failed to send command");
      }
      return JS_UNDEFINED;

    }
    static JSValue read (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

      int sockfd;
      JS_ToInt32(ctx, &sockfd, argv[0]);
      if (sockfd == -1) JS_ThrowTypeError(ctx, "game.read: Invalid socket provided");

      int64_t size;
      JS_ToInt64(ctx, &size, argv[1]);
      if (!size) size = 1024;

      const std::string output = ToolsNetCon::readConsole(sockfd, size);
      return JS_NewStringLen(ctx, output.c_str(), output.length());

    }
  };

  static JSValue sleep (JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

    int64_t time;
    JS_ToInt64(ctx, &time, argv[0]);
    if (time < 0) return JS_ThrowTypeError(ctx, "sleep: Time cannot be negative");

    std::this_thread::sleep_for(std::chrono::milliseconds(time));
    return JS_UNDEFINED;

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

  // Create a new QuickJS runtime
  JSRuntime *rt = JS_NewRuntime();
  JSContext *ctx = JS_NewContext(rt);
  JSValue global = JS_GetGlobalObject(ctx);

  // Register custom functions
  JSValue console = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, console, "log", JS_NewCFunction(ctx, customJS::console::log, "log", 3));
  JS_SetPropertyStr(ctx, console, "error", JS_NewCFunction(ctx, customJS::console::error, "error", 5));
  JS_SetPropertyStr(ctx, global, "console", console);

  JSValue fs = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, fs, "mkdir", JS_NewCFunction(ctx, customJS::fs::mkdir, "mkdir", 5));
  JS_SetPropertyStr(ctx, fs, "unlink", JS_NewCFunction(ctx, customJS::fs::unlink, "unlink", 6));
  JS_SetPropertyStr(ctx, fs, "read", JS_NewCFunction(ctx, customJS::fs::read, "read", 4));
  JS_SetPropertyStr(ctx, fs, "write", JS_NewCFunction(ctx, customJS::fs::write, "write", 5));
  JS_SetPropertyStr(ctx, fs, "rename", JS_NewCFunction(ctx, customJS::fs::rename, "rename", 6));
  JS_SetPropertyStr(ctx, global, "fs", fs);

  JSValue game = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, game, "connect", JS_NewCFunction(ctx, customJS::game::connect, "connect", 7));
  JS_SetPropertyStr(ctx, game, "disconnect", JS_NewCFunction(ctx, customJS::game::disconnect, "disconnect", 10));
  JS_SetPropertyStr(ctx, game, "send", JS_NewCFunction(ctx, customJS::game::send, "send", 4));
  JS_SetPropertyStr(ctx, game, "read", JS_NewCFunction(ctx, customJS::game::read, "read", 4));
  JS_SetPropertyStr(ctx, global, "game", game);

  JS_SetPropertyStr(ctx, global, "sleep", JS_NewCFunction(ctx, customJS::sleep, "sleep", 5));

  // Evaluate the code in the file buffer
  const std::string code = fileBuffer.str();
  JSValue result = JS_Eval(ctx, code.c_str(), code.length(), filePath.filename().string().c_str(), JS_EVAL_TYPE_GLOBAL);

  // Handle exceptions
  if (JS_IsException(result)) {
    JSValue exception = JS_GetException(ctx);
    const char *exception_str = JS_ToCString(ctx, exception);
    std::cerr << "Exception: " << exception_str << std::endl;
    JS_FreeCString(ctx, exception_str);
    JS_FreeValue(ctx, exception);
  }

  // Clean up the runtime
  JS_FreeValue(ctx, global);
  JS_FreeValue(ctx, result);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

}
