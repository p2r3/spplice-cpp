#ifndef TOOLS_PACKAGE_H
#define TOOLS_PACKAGE_H

#include "rapidjson/document.h"
#include <filesystem>
#include <QObject>
#include <QPixmap>

// The structure of a package's properties
class ToolsPackage {
  public:

    struct PackageData {

      std::string title;
      std::string author;
      std::string description;
      std::string version;
      std::vector<std::string> args;
      std::string file;
      std::string icon;

      PackageData (rapidjson::Value &package);

    };

};

class PackageItemWorker : public QObject {
  Q_OBJECT

  public slots:
    void getPackageIcon (const std::string &imageURL, const std::filesystem::path imagePath, const QSize iconSize);
    void installPackageURL (const std::string &fileURL, const std::string &version);

  signals:
    void packageIconResult (QPixmap pixmap);
    void packageIconReady ();
    void installStateUpdate ();

};

#endif
