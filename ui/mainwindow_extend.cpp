// Headers for this source file
#include "mainwindow_extend.h"
// The UI class we're extending
#include "mainwindow.h"

#include <iostream>
#include <filesystem>

// Required for returning UI elements
#include <QVBoxLayout>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setAcceptDrops(true);
}

MainWindow::~MainWindow() {
  delete ui;
}

// Accepts drag-and-drop only for events with file URLs
void MainWindow::dragEnterEvent (QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
  else event->ignore();
}

// Handles a file being dropped into the main window
void MainWindow::dropEvent (QDropEvent *event) {

  // Check the MIME data of the event again
  const QMimeData *mimeData = event->mimeData();
  if (!mimeData->hasUrls()) return event->ignore();

  // Iterate over all URLs (files) dropped
  foreach (const QUrl &url, mimeData->urls()) {

#ifndef TARGET_WINDOWS
    std::filesystem::path filePath = url.toLocalFile().toStdString();
#else
    std::filesystem::path filePath = url.toLocalFile().toStdWString();
#endif

    // TODO: Process this further
    std::cout << "Received dropped file: " << filePath << std::endl;

  }

  event->acceptProposedAction();

}

QVBoxLayout *MainWindow::getPackageListLayout() const {
  return ui->PackageListLayout;
}
QPushButton *MainWindow::getSettingsButton () const {
  return ui->TitleButtonL;
}
QPushButton *MainWindow::getRepositoryButton () const {
  return ui->TitleButtonR;
}
