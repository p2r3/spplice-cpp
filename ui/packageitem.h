/********************************************************************************
** Form generated from reading UI file 'PackageItem.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PACKAGEITEM_H
#define PACKAGEITEM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PackageItem
{
public:
    QGridLayout *gridLayout;
    QFrame *PackageFrame;
    QGridLayout *gridLayout_2;
    QLabel *PackageIcon;
    QVBoxLayout *PackageText;
    QLabel *PackageTitle;
    QLabel *PackageDescription;
    QPushButton *PackageButton;
    QSpacerItem *SpacerL;
    QSpacerItem *SpacerR;

    void setupUi(QWidget *PackageItem)
    {
        if (PackageItem->objectName().isEmpty())
            PackageItem->setObjectName(QString::fromUtf8("PackageItem"));
        PackageItem->resize(770, 427);
        QFont font;
        font.setPointSize(11);
        PackageItem->setFont(font);
        PackageItem->setStyleSheet(QString::fromUtf8("#PackageFrame {\n"
"	background-color: #000;\n"
"	border: 2px solid white;\n"
"	border-radius: 10px;\n"
"}\n"
"QLabel, QPushButton {\n"
"	color: #fff;\n"
"}"));
        gridLayout = new QGridLayout(PackageItem);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        PackageFrame = new QFrame(PackageItem);
        PackageFrame->setObjectName(QString::fromUtf8("PackageFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PackageFrame->sizePolicy().hasHeightForWidth());
        PackageFrame->setSizePolicy(sizePolicy);
        PackageFrame->setMinimumSize(QSize(675, 100));
        PackageFrame->setMaximumSize(QSize(675, 100));
        PackageFrame->setBaseSize(QSize(675, 100));
        PackageFrame->setFrameShape(QFrame::StyledPanel);
        PackageFrame->setFrameShadow(QFrame::Raised);
        gridLayout_2 = new QGridLayout(PackageFrame);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(7, 7, 7, 7);
        PackageIcon = new QLabel(PackageFrame);
        PackageIcon->setObjectName(QString::fromUtf8("PackageIcon"));
        PackageIcon->setMinimumSize(QSize(153, 86));
        PackageIcon->setMaximumSize(QSize(153, 86));
        PackageIcon->setBaseSize(QSize(100, 100));

        gridLayout_2->addWidget(PackageIcon, 2, 0, 1, 1);

        PackageText = new QVBoxLayout();
        PackageText->setObjectName(QString::fromUtf8("PackageText"));
        PackageTitle = new QLabel(PackageFrame);
        PackageTitle->setObjectName(QString::fromUtf8("PackageTitle"));
        QFont font1;
        font1.setFamily(QString::fromUtf8("Quicksand"));
        font1.setPointSize(16);
        font1.setBold(true);
        font1.setWeight(75);
        PackageTitle->setFont(font1);

        PackageText->addWidget(PackageTitle);

        PackageDescription = new QLabel(PackageFrame);
        PackageDescription->setObjectName(QString::fromUtf8("PackageDescription"));
        QFont font2;
        font2.setFamily(QString::fromUtf8("Quicksand Light"));
        font2.setPointSize(11);
        PackageDescription->setFont(font2);
        PackageDescription->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        PackageText->addWidget(PackageDescription);


        gridLayout_2->addLayout(PackageText, 2, 1, 1, 1);

        PackageButton = new QPushButton(PackageFrame);
        PackageButton->setObjectName(QString::fromUtf8("PackageButton"));
        PackageButton->setMinimumSize(QSize(100, 40));
        PackageButton->setMaximumSize(QSize(100, 40));
        QFont font3;
        font3.setFamily(QString::fromUtf8("Quicksand Medium"));
        font3.setPointSize(14);
        PackageButton->setFont(font3);
        PackageButton->setFlat(true);

        gridLayout_2->addWidget(PackageButton, 2, 2, 1, 1);


        gridLayout->addWidget(PackageFrame, 0, 1, 1, 1);

        SpacerL = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(SpacerL, 0, 0, 1, 1);

        SpacerR = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(SpacerR, 0, 2, 1, 1);


        retranslateUi(PackageItem);

        QMetaObject::connectSlotsByName(PackageItem);
    } // setupUi

    void retranslateUi(QWidget *PackageItem)
    {
        PackageItem->setWindowTitle(QCoreApplication::translate("PackageItem", "Form", nullptr));
        PackageIcon->setText(QString());
        PackageTitle->setText(QCoreApplication::translate("PackageItem", "Mid-Air Portals", nullptr));
        PackageDescription->setText(QCoreApplication::translate("PackageItem", "Lets you place portals mid-air.", nullptr));
        PackageButton->setText(QCoreApplication::translate("PackageItem", "Install", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PackageItem: public Ui_PackageItem {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PACKAGEITEM_H
