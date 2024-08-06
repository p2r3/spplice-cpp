/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *CentralWidget;
    QGridLayout *gridLayout;
    QSpacerItem *TitleSpacerT;
    QLabel *MenuTitle;
    QSpacerItem *TitleSpacerL;
    QSpacerItem *TitleSpacerR;
    QSpacerItem *TitlteSpacerB;
    QScrollArea *PackageList;
    QWidget *PackageListContents;
    QVBoxLayout *verticalLayout;
    QSpacerItem *ListSpacerB;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1000, 650);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setMinimumSize(QSize(800, 600));
        MainWindow->setBaseSize(QSize(1000, 650));
        MainWindow->setStyleSheet(QString::fromUtf8("QWidget {\n"
"	background-color: #000000;\n"
"}\n"
"#MenuTitle {\n"
"	color: #17C0E9;\n"
"}"));
        CentralWidget = new QWidget(MainWindow);
        CentralWidget->setObjectName(QString::fromUtf8("CentralWidget"));
        gridLayout = new QGridLayout(CentralWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        TitleSpacerT = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(TitleSpacerT, 0, 2, 1, 1);

        MenuTitle = new QLabel(CentralWidget);
        MenuTitle->setObjectName(QString::fromUtf8("MenuTitle"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(MenuTitle->sizePolicy().hasHeightForWidth());
        MenuTitle->setSizePolicy(sizePolicy1);
        QFont font;
        font.setFamily(QString::fromUtf8("Quicksand Medium"));
        font.setPointSize(30);
        font.setBold(false);
        font.setWeight(50);
        MenuTitle->setFont(font);
        MenuTitle->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(MenuTitle, 1, 1, 1, 3);

        TitleSpacerL = new QSpacerItem(309, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(TitleSpacerL, 1, 0, 1, 1);

        TitleSpacerR = new QSpacerItem(309, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(TitleSpacerR, 1, 4, 1, 1);

        TitlteSpacerB = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(TitlteSpacerB, 2, 2, 1, 1);

        PackageList = new QScrollArea(CentralWidget);
        PackageList->setObjectName(QString::fromUtf8("PackageList"));
        PackageList->setWidgetResizable(true);
        PackageListContents = new QWidget();
        PackageListContents->setObjectName(QString::fromUtf8("PackageListContents"));
        PackageListContents->setGeometry(QRect(0, 0, 980, 396));
        verticalLayout = new QVBoxLayout(PackageListContents);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        PackageList->setWidget(PackageListContents);

        gridLayout->addWidget(PackageList, 3, 0, 1, 5);

        ListSpacerB = new QSpacerItem(20, 100, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(ListSpacerB, 4, 2, 1, 1);

        MainWindow->setCentralWidget(CentralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        MenuTitle->setText(QCoreApplication::translate("MainWindow", "Spplice packages", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // MAINWINDOW_H
