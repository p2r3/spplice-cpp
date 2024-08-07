// The structure of a package's properties
class PackageData {
  public:

    std::string name;
    std::string title;
    std::string author;
    std::string file;
    std::string icon;
    std::string description;
    int weight;

    // Constructs the PackageData instance from a JSON object
    PackageData (rapidjson::Value &package) {
      this->name = package["name"].GetString();
      this->title = package["title"].GetString();
      this->author = package["author"].GetString();
      this->file = package["file"].GetString();
      this->icon = package["icon"].GetString();
      this->description = package["description"].GetString();
      this->weight = package["weight"].GetInt();
    }

    // Generates a PackageItem instance of the given package for rendering
    QWidget *createPackageItem (CURL *curl) {

      QWidget *item = new QWidget;
      Ui::PackageItem itemUI;
      itemUI.setupUi(item);

      // Set values of text-based elements
      itemUI.PackageTitle->setText(QString::fromStdString( this->title ));
      itemUI.PackageDescription->setText(QString::fromStdString( this->description ));

      // Download the package icon
      std::string imageFileName = this->name + "_icon";
      QString imagePath = QString::fromStdString(downloadTempFile(curl, this->icon, imageFileName));

      // Create a pixmap for the icon
      QSize labelSize = itemUI.PackageIcon->size();
      QPixmap iconPixmap = QPixmap(imagePath).scaled(labelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      QPixmap iconRoundedPixmap = getRoundedPixmap(iconPixmap, 10);

      // Set the package icon
      itemUI.PackageIcon->setPixmap(iconRoundedPixmap);

      // Connect the install button
      std::string fileURL = this->file;
      QWidget::connect(itemUI.PackageInstallButton, &QPushButton::clicked, [fileURL, curl]() {
        std::cout << "requested install " << fileURL << std::endl;
      });

      return item;

    }

};
