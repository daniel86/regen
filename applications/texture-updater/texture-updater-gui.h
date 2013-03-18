/********************************************************************************
** Form generated from reading UI file 'texture-updater-guiEV9029.ui'
**
** Created: Mon Mar 18 20:03:04 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TEXTURE_2D_UPDATER_2D_GUIEV9029_H
#define TEXTURE_2D_UPDATER_2D_GUIEV9029_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_textureUpdater
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QFrame *buttonBarFrame;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *buttonSpacer;
    QPushButton *openButton;
    QPushButton *exitButton;
    QWidget *blackBackground;
    QGridLayout *gridLayout;
    QWidget *glWidget;
    QGridLayout *gridLayout_6;
    QGridLayout *glWidgetLayout;

    void setupUi(QMainWindow *textureUpdater)
    {
        if (textureUpdater->objectName().isEmpty())
            textureUpdater->setObjectName(QString::fromUtf8("textureUpdater"));
        textureUpdater->resize(519, 430);
        textureUpdater->setStyleSheet(QString::fromUtf8("#buttonBarFrame {\n"
"border: none;\n"
"background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"stop: 0 #a6a6a6, stop: 0.08 #7f7f7f,\n"
"stop: 0.39999 #717171, stop: 0.4 #626262,\n"
"stop: 0.9 #4c4c4c, stop: 1 #333333);\n"
"}\n"
"\n"
"#buttonBarFrame QPushButton {\n"
"color: #333;\n"
"border: 2px solid #555;\n"
"border-radius: 11px;\n"
"padding: 5px;\n"
"background: qradialgradient(cx: 0.3, cy: -0.4,\n"
"fx: 0.3, fy: -0.4,\n"
"radius: 1.35, stop: 0 #fff, stop: 1 #888);\n"
"min-width: 80px;\n"
"}\n"
"\n"
"#buttonBarFrame QPushButton:hover {\n"
"background: qradialgradient(cx: 0.3, cy: -0.4,\n"
"fx: 0.3, fy: -0.4,\n"
"radius: 1.35, stop: 0 #fff, stop: 1 #bbb);\n"
"}\n"
"\n"
"#buttonBarFrame QPushButton:pressed {\n"
"background: qradialgradient(cx: 0.4, cy: -0.1,\n"
"fx: 0.4, fy: -0.1,\n"
"radius: 1.35, stop: 0 #fff, stop: 1 #ddd);\n"
"}\n"
"\n"
"#bottomFrame {\n"
"border: none;\n"
"background: white;\n"
"}\n"
""));
        centralwidget = new QWidget(textureUpdater);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        buttonBarFrame = new QFrame(centralwidget);
        buttonBarFrame->setObjectName(QString::fromUtf8("buttonBarFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(buttonBarFrame->sizePolicy().hasHeightForWidth());
        buttonBarFrame->setSizePolicy(sizePolicy);
        buttonBarFrame->setMaximumSize(QSize(16777215, 50));
        buttonBarFrame->setFrameShape(QFrame::StyledPanel);
        buttonBarFrame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(buttonBarFrame);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        buttonSpacer = new QSpacerItem(298, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(buttonSpacer);

        openButton = new QPushButton(buttonBarFrame);
        openButton->setObjectName(QString::fromUtf8("openButton"));
        QIcon icon;
        QString iconThemeName = QString::fromUtf8("document-open");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        openButton->setIcon(icon);
        openButton->setCheckable(false);
        openButton->setFlat(false);

        horizontalLayout->addWidget(openButton);

        exitButton = new QPushButton(buttonBarFrame);
        exitButton->setObjectName(QString::fromUtf8("exitButton"));
        exitButton->setFocusPolicy(Qt::StrongFocus);
        QIcon icon1;
        iconThemeName = QString::fromUtf8("application-exit");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        exitButton->setIcon(icon1);
        exitButton->setAutoDefault(false);
        exitButton->setDefault(false);
        exitButton->setFlat(false);

        horizontalLayout->addWidget(exitButton);


        verticalLayout->addWidget(buttonBarFrame);

        blackBackground = new QWidget(centralwidget);
        blackBackground->setObjectName(QString::fromUtf8("blackBackground"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(blackBackground->sizePolicy().hasHeightForWidth());
        blackBackground->setSizePolicy(sizePolicy1);
        blackBackground->setMinimumSize(QSize(48, 48));
        blackBackground->setFocusPolicy(Qt::NoFocus);
        blackBackground->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout = new QGridLayout(blackBackground);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        glWidget = new QWidget(blackBackground);
        glWidget->setObjectName(QString::fromUtf8("glWidget"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy2);
        glWidget->setMinimumSize(QSize(1, 1));
        glWidget->setFocusPolicy(Qt::NoFocus);
        glWidget->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout_6 = new QGridLayout(glWidget);
        gridLayout_6->setContentsMargins(0, 0, 0, 0);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        glWidgetLayout = new QGridLayout();
        glWidgetLayout->setObjectName(QString::fromUtf8("glWidgetLayout"));

        gridLayout_6->addLayout(glWidgetLayout, 0, 0, 1, 1);


        gridLayout->addWidget(glWidget, 0, 0, 1, 1);


        verticalLayout->addWidget(blackBackground);

        textureUpdater->setCentralWidget(centralwidget);

        retranslateUi(textureUpdater);
        QObject::connect(exitButton, SIGNAL(clicked()), textureUpdater, SLOT(close()));
        QObject::connect(openButton, SIGNAL(clicked()), textureUpdater, SLOT(openFile()));

        QMetaObject::connectSlotsByName(textureUpdater);
    } // setupUi

    void retranslateUi(QMainWindow *textureUpdater)
    {
        textureUpdater->setWindowTitle(QApplication::translate("textureUpdater", "MainWindow", 0, QApplication::UnicodeUTF8));
        openButton->setText(QApplication::translate("textureUpdater", "Open", 0, QApplication::UnicodeUTF8));
        exitButton->setText(QApplication::translate("textureUpdater", "Exit", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class textureUpdater: public Ui_textureUpdater {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TEXTURE_2D_UPDATER_2D_GUIEV9029_H
