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

      std::string name;
      std::string title;
      std::string author;
      std::string file;
      std::string icon;
      std::string description;
      int weight;

      PackageData (rapidjson::Value &package);

    };

};

class PackageItemWorker : public QObject {
  Q_OBJECT

  public slots:
    void getPackageIcon (const std::string &imageURL, const std::string &imagePath, const QSize iconSize);

  signals:
    void packageIconResult (QPixmap pixmap);
    void packageIconReady ();

};

#endif
