#include "rapidjson/document.h"
#include <QWidget>

// The structure of a package's properties
class ToolsPackage {
  public:
    class PackageData {
      public:

        std::string name;
        std::string title;
        std::string author;
        std::string file;
        std::string icon;
        std::string description;
        int weight;

        PackageData (rapidjson::Value &package);
        QWidget *createPackageItem ();

    };
};
