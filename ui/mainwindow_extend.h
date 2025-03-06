#ifndef MAINWINDOW_EXTEND_H
#define MAINWINDOW_EXTEND_H

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow (QWidget *parent = nullptr);
  QVBoxLayout *getPackageListLayout () const;
  QPushButton *getSettingsButton () const;
  QPushButton *getRepositoryButton () const;
  ~MainWindow () override;

protected:
  void dragEnterEvent (QDragEnterEvent *event) override;
  void dropEvent (QDropEvent *event) override;

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_EXTEND_H
