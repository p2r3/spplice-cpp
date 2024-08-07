#include <iostream>
#include <filesystem>
#include <archive.h>
#include <archive_entry.h>
#include "../globals.h" // Project globals
#include "curl.h" // ToolsCURL

// Definitions for this source file
#include "install.h"

bool ToolsInstall::extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest) {

  struct archive* archive;
  struct archive* extracted;
  struct archive_entry* entry;
  int r;

  archive = archive_read_new();
  archive_read_support_format_tar(archive);
  archive_read_support_filter_xz(archive);
  extracted = archive_write_disk_new();
  archive_write_disk_set_options(extracted, ARCHIVE_EXTRACT_TIME);

  if ((r = archive_read_open_filename(archive, path.c_str(), 10240))) {
    std::cerr << "Could not open file: " << archive_error_string(archive) << std::endl;
    return false;
  }

  bool output = true;
  while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {

    std::filesystem::path full_path = dest / archive_entry_pathname(entry);
    archive_entry_set_pathname(entry, full_path.c_str());

    std::cout << "Extracting: " << archive_entry_pathname(entry) << std::endl;
    archive_write_header(extracted, entry);

    const void* buff;
    size_t size;
    la_int64_t offset;

    while (true) {
      r = archive_read_data_block(archive, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) break;
      if (r != ARCHIVE_OK) {
        std::cerr << "Archive read error: " << archive_error_string(archive) << std::endl;
        output = false;
        break;
      }
      r = archive_write_data_block(extracted, buff, size, offset);
      if (r != ARCHIVE_OK) {
        std::cerr << "Archive write error: " << archive_error_string(extracted) << std::endl;
        output = false;
        break;
      }
    }
    archive_write_finish_entry(extracted);

  }

  archive_read_close(archive);
  archive_read_free(archive);
  archive_write_close(extracted);
  archive_write_free(extracted);

  return output;

}

bool ToolsInstall::installRemoteFile (const std::string &fileURL) {

  std::cout << "downloading file from " << fileURL << " to " << TEMP_DIR / "package" << std::endl;
  ToolsCURL::downloadFile(fileURL, TEMP_DIR / "package");

  std::filesystem::path gamePath = "/home/p2r3/.local/share/Steam/steamapps/common/Portal 2";
  std::filesystem::path tempcontentPath = gamePath / "portal2_tempcontent";
  std::filesystem::create_directories(tempcontentPath);

  return extractLocalFile(TEMP_DIR / "package", tempcontentPath);

}
