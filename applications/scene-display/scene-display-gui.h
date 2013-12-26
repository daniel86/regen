/********************************************************************************
** Form generated from reading UI file 'scene-display-guifo1010.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SCENE_2D_DISPLAY_2D_GUIFO1010_H
#define SCENE_2D_DISPLAY_2D_GUIFO1010_H

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

class Ui_sceneViewer
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QFrame *buttonBarFrame;
    QHBoxLayout *horizontalLayout;
    QPushButton *prevView;
    QPushButton *nextView;
    QSpacerItem *buttonSpacer;
    QPushButton *openButton;
    QPushButton *inputsButton;
    QPushButton *exitButton;
    QWidget *blackBackground;
    QGridLayout *gridLayout;
    QWidget *glWidget;
    QGridLayout *gridLayout_6;
    QGridLayout *glWidgetLayout;

    void setupUi(QMainWindow *sceneViewer)
    {
        if (sceneViewer->objectName().isEmpty())
            sceneViewer->setObjectName(QString::fromUtf8("sceneViewer"));
        sceneViewer->resize(590, 430);
        sceneViewer->setStyleSheet(QString::fromUtf8("#buttonBarFrame {\n"
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
"}\n"
"# min-width: 80px;\n"
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
        centralwidget = new QWidget(sceneViewer);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        centralwidget->setFocusPolicy(Qt::StrongFocus);
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
        prevView = new QPushButton(buttonBarFrame);
        prevView->setObjectName(QString::fromUtf8("prevView"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(prevView->sizePolicy().hasHeightForWidth());
        prevView->setSizePolicy(sizePolicy1);
        prevView->setMinimumSize(QSize(48, 24));
        prevView->setMaximumSize(QSize(48, 48));
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("go-previous")));
        prevView->setIcon(icon);

        horizontalLayout->addWidget(prevView);

        nextView = new QPushButton(buttonBarFrame);
        nextView->setObjectName(QString::fromUtf8("nextView"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(nextView->sizePolicy().hasHeightForWidth());
        nextView->setSizePolicy(sizePolicy2);
        nextView->setMinimumSize(QSize(48, 24));
        nextView->setMaximumSize(QSize(48, 48));
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("go-next")));
        nextView->setIcon(icon1);

        horizontalLayout->addWidget(nextView);

        buttonSpacer = new QSpacerItem(298, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(buttonSpacer);

        openButton = new QPushButton(buttonBarFrame);
        openButton->setObjectName(QString::fromUtf8("openButton"));
        openButton->setMinimumSize(QSize(80, 0));
        openButton->setFocusPolicy(Qt::NoFocus);
        QIcon icon2;
        QString iconThemeName = QString::fromUtf8("document-open");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        openButton->setIcon(icon2);
        openButton->setCheckable(false);
        openButton->setFlat(false);

        horizontalLayout->addWidget(openButton);

        inputsButton = new QPushButton(buttonBarFrame);
        inputsButton->setObjectName(QString::fromUtf8("inputsButton"));
        inputsButton->setMinimumSize(QSize(80, 0));
        inputsButton->setFocusPolicy(Qt::NoFocus);
        QIcon icon3;
        iconThemeName = QString::fromUtf8("document-properties");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon3 = QIcon::fromTheme(iconThemeName);
        } else {
            icon3.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        inputsButton->setIcon(icon3);

        horizontalLayout->addWidget(inputsButton);

        exitButton = new QPushButton(buttonBarFrame);
        exitButton->setObjectName(QString::fromUtf8("exitButton"));
        exitButton->setMinimumSize(QSize(80, 0));
        exitButton->setFocusPolicy(Qt::NoFocus);
        QIcon icon4;
        iconThemeName = QString::fromUtf8("application-exit");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon4 = QIcon::fromTheme(iconThemeName);
        } else {
            icon4.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        exitButton->setIcon(icon4);
        exitButton->setAutoDefault(false);
        exitButton->setDefault(false);
        exitButton->setFlat(false);

        horizontalLayout->addWidget(exitButton);


        verticalLayout->addWidget(buttonBarFrame);

        blackBackground = new QWidget(centralwidget);
        blackBackground->setObjectName(QString::fromUtf8("blackBackground"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(blackBackground->sizePolicy().hasHeightForWidth());
        blackBackground->setSizePolicy(sizePolicy3);
        blackBackground->setMinimumSize(QSize(48, 48));
        blackBackground->setFocusPolicy(Qt::NoFocus);
        blackBackground->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout = new QGridLayout(blackBackground);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        glWidget = new QWidget(blackBackground);
        glWidget->setObjectName(QString::fromUtf8("glWidget"));
        sizePolicy3.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy3);
        glWidget->setMinimumSize(QSize(1, 1));
        glWidget->setFocusPolicy(Qt::StrongFocus);
        glWidget->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout_6 = new QGridLayout(glWidget);
        gridLayout_6->setContentsMargins(0, 0, 0, 0);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        glWidgetLayout = new QGridLayout();
        glWidgetLayout->setObjectName(QString::fromUtf8("glWidgetLayout"));

        gridLayout_6->addLayout(glWidgetLayout, 0, 0, 1, 1);


        gridLayout->addWidget(glWidget, 0, 0, 1, 1);


        verticalLayout->addWidget(blackBackground);

        sceneViewer->setCentralWidget(centralwidget);

        retranslateUi(sceneViewer);
        QObject::connect(exitButton, SIGNAL(clicked()), sceneViewer, SLOT(close()));
        QObject::connect(openButton, SIGNAL(clicked()), sceneViewer, SLOT(openFile()));
        QObject::connect(inputsButton, SIGNAL(clicked()), sceneViewer, SLOT(toggleInputsDialog()));
        QObject::connect(nextView, SIGNAL(clicked()), sceneViewer, SLOT(nextView()));
        QObject::connect(prevView, SIGNAL(clicked()), sceneViewer, SLOT(previousView()));

        QMetaObject::connectSlotsByName(sceneViewer);
    } // setupUi

    void retranslateUi(QMainWindow *sceneViewer)
    {
        sceneViewer->setWindowTitle(QApplication::translate("sceneViewer", "Scene Viewer", 0, QApplication::UnicodeUTF8));
        prevView->setText(QString());
        nextView->setText(QString());
        openButton->setText(QApplication::translate("sceneViewer", "Open", 0, QApplication::UnicodeUTF8));
        inputsButton->setText(QApplication::translate("sceneViewer", "Inputs", 0, QApplication::UnicodeUTF8));
        exitButton->setText(QApplication::translate("sceneViewer", "Exit", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class sceneViewer: public Ui_sceneViewer {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SCENE_2D_DISPLAY_2D_GUIFO1010_H
