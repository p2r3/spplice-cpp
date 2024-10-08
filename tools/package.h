#ifndef TOOLS_PACKAGE_H
#define TOOLS_PACKAGE_H

#include <filesystem>
#include <QObject>
#include <QPixmap>
#include <QJsonObject>

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

      PackageData (QJsonObject package);

    };

};

class PackageItemWorker : public QObject {
  Q_OBJECT

  public slots:
    void getPackageIcon (const ToolsPackage::PackageData* package, const QSize iconSize);
    void installPackage (const ToolsPackage::PackageData* package);

  signals:
    void packageIconResult (QPixmap pixmap);
    void packageIconReady ();
    void installStateUpdate ();

};

#endif
