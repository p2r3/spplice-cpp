#ifndef TOOLS_PACKAGE_H
#define TOOLS_PACKAGE_H

#include <filesystem>
#include <QObject>
#include <QPixmap>
#include <QJsonObject>

class ToolsPackage {
  public:

    // The structure of a package's properties
    struct PackageData {

      std::string title;
      std::string author;
      std::string description;
      std::string version;
      std::vector<std::string> args;
      std::string file;
      std::string icon;
      std::string repository;

      PackageData (QJsonObject package, const std::string &url);

    };

    static QWidget* createPackageItem (const PackageData* package);

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
    void installWorkerDone ();

};

#endif
