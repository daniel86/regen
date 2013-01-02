
/********************************************************************************
** Form generated from reading UI file 'video-player-guinV9896.ui'
**
** Created: Tue Jan 1 14:46:26 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef VIDEO_PLAYER_GUI_H_
#define VIDEO_PLAYER_GUI_H_

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QStatusBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_mainWindow
{
public:
    QAction *actionOpen_File;
    QAction *actionQuit;
    QAction *actionAudio_Track;
    QAction *actionVideo_Track;
    QAction *actionPlaylist;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QWidget *blackBackground;
    QGridLayout *gridLayout_3;
    QWidget *glWidget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_5;
    QLabel *progressLabel;
    QSlider *progressSlider;
    QLabel *movieLengthLabel;
    QSpacerItem *horizontalSpacer_4;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *playButton;
    QSpacerItem *horizontalSpacer;
    QPushButton *prevButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *fullscreenButton;
    QSpacerItem *horizontalSpacer_6;
    QPushButton *repeatButton;
    QPushButton *shuffleButton;
    QSpacerItem *horizontalSpacer_3;
    QVBoxLayout *verticalLayout_2;
    QLabel *volumeLabel;
    QSlider *volumeSlider;
    QMenuBar *menubar;
    QMenu *menuMedia;
    QMenu *menuAudio;
    QMenu *menuVideo;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *mainWindow, QWidget *glWidget_)
    {
        if (mainWindow->objectName().isEmpty())
            mainWindow->setObjectName(QString::fromUtf8("mainWindow"));
        mainWindow->resize(400, 277);
        actionOpen_File = new QAction(mainWindow);
        actionOpen_File->setObjectName(QString::fromUtf8("actionOpen_File"));
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("document-open")));
        actionOpen_File->setIcon(icon);
        actionOpen_File->setIconVisibleInMenu(true);
        actionQuit = new QAction(mainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("application-exit")));
        actionQuit->setIcon(icon1);
        actionQuit->setIconVisibleInMenu(true);
        actionAudio_Track = new QAction(mainWindow);
        actionAudio_Track->setObjectName(QString::fromUtf8("actionAudio_Track"));
        actionVideo_Track = new QAction(mainWindow);
        actionVideo_Track->setObjectName(QString::fromUtf8("actionVideo_Track"));
        actionPlaylist = new QAction(mainWindow);
        actionPlaylist->setObjectName(QString::fromUtf8("actionPlaylist"));
        centralwidget = new QWidget(mainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        blackBackground = new QWidget(centralwidget);
        blackBackground->setObjectName(QString::fromUtf8("blackBackground"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(blackBackground->sizePolicy().hasHeightForWidth());
        blackBackground->setSizePolicy(sizePolicy);
        blackBackground->setMinimumSize(QSize(48, 48));
        blackBackground->setFocusPolicy(Qt::NoFocus);
        blackBackground->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout_3 = new QGridLayout(blackBackground);
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        glWidget = glWidget_;
        glWidget->setParent(blackBackground);
        glWidget->setObjectName(QString::fromUtf8("glWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy1);
        glWidget->setMinimumSize(QSize(1, 1));
        glWidget->setFocusPolicy(Qt::NoFocus);
        glWidget->setStyleSheet(QString::fromUtf8("background-color: black;"));

        gridLayout_3->addWidget(glWidget, 0, 0, 1, 1);


        verticalLayout->addWidget(blackBackground);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_5 = new QSpacerItem(8, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_5);

        progressLabel = new QLabel(centralwidget);
        progressLabel->setObjectName(QString::fromUtf8("progressLabel"));
        progressLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(progressLabel);

        progressSlider = new QSlider(centralwidget);
        progressSlider->setObjectName(QString::fromUtf8("progressSlider"));
        progressSlider->setAcceptDrops(false);
        progressSlider->setMaximum(100000);
        progressSlider->setPageStep(10);
        progressSlider->setTracking(true);
        progressSlider->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(progressSlider);

        movieLengthLabel = new QLabel(centralwidget);
        movieLengthLabel->setObjectName(QString::fromUtf8("movieLengthLabel"));

        horizontalLayout->addWidget(movieLengthLabel);

        horizontalSpacer_4 = new QSpacerItem(8, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(2);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(2, -1, 2, 2);
        playButton = new QPushButton(centralwidget);
        playButton->setObjectName(QString::fromUtf8("playButton"));
        sizePolicy1.setHeightForWidth(playButton->sizePolicy().hasHeightForWidth());
        playButton->setSizePolicy(sizePolicy1);
        playButton->setMinimumSize(QSize(34, 34));
        playButton->setMaximumSize(QSize(34, 34));
        playButton->setText(QString::fromUtf8(""));
        QIcon icon2;
        QString iconThemeName = QString::fromUtf8("media-playback-start");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        playButton->setIcon(icon2);

        horizontalLayout_2->addWidget(playButton);

        horizontalSpacer = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        prevButton = new QPushButton(centralwidget);
        prevButton->setObjectName(QString::fromUtf8("prevButton"));
        prevButton->setMinimumSize(QSize(28, 28));
        prevButton->setMaximumSize(QSize(28, 28));
        QIcon icon3;
        iconThemeName = QString::fromUtf8("media-skip-backward");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon3 = QIcon::fromTheme(iconThemeName);
        } else {
            icon3.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        prevButton->setIcon(icon3);
        prevButton->setIconSize(QSize(18, 18));

        horizontalLayout_2->addWidget(prevButton);

        stopButton = new QPushButton(centralwidget);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        stopButton->setMinimumSize(QSize(28, 28));
        stopButton->setMaximumSize(QSize(28, 28));
        QIcon icon4;
        iconThemeName = QString::fromUtf8("media-playback-stop");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon4 = QIcon::fromTheme(iconThemeName);
        } else {
            icon4.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        stopButton->setIcon(icon4);
        stopButton->setIconSize(QSize(18, 18));
        stopButton->setFlat(false);

        horizontalLayout_2->addWidget(stopButton);

        nextButton = new QPushButton(centralwidget);
        nextButton->setObjectName(QString::fromUtf8("nextButton"));
        nextButton->setMinimumSize(QSize(28, 28));
        nextButton->setMaximumSize(QSize(28, 28));
        QIcon icon5;
        iconThemeName = QString::fromUtf8("media-skip-forward");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon5 = QIcon::fromTheme(iconThemeName);
        } else {
            icon5.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        nextButton->setIcon(icon5);
        nextButton->setIconSize(QSize(18, 18));

        horizontalLayout_2->addWidget(nextButton);

        horizontalSpacer_2 = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        fullscreenButton = new QPushButton(centralwidget);
        fullscreenButton->setObjectName(QString::fromUtf8("fullscreenButton"));
        fullscreenButton->setMinimumSize(QSize(28, 28));
        fullscreenButton->setMaximumSize(QSize(28, 28));
        QIcon icon6;
        iconThemeName = QString::fromUtf8("view-fullscreen");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon6 = QIcon::fromTheme(iconThemeName);
        } else {
            icon6.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        fullscreenButton->setIcon(icon6);
        fullscreenButton->setIconSize(QSize(18, 18));

        horizontalLayout_2->addWidget(fullscreenButton);

        horizontalSpacer_6 = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_6);

        repeatButton = new QPushButton(centralwidget);
        repeatButton->setObjectName(QString::fromUtf8("repeatButton"));
        repeatButton->setMinimumSize(QSize(28, 28));
        repeatButton->setMaximumSize(QSize(28, 28));
        QIcon icon7;
        iconThemeName = QString::fromUtf8("media-playlist-repeat");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon7 = QIcon::fromTheme(iconThemeName);
        } else {
            icon7.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        repeatButton->setIcon(icon7);
        repeatButton->setIconSize(QSize(18, 18));
        repeatButton->setCheckable(true);

        horizontalLayout_2->addWidget(repeatButton);

        shuffleButton = new QPushButton(centralwidget);
        shuffleButton->setObjectName(QString::fromUtf8("shuffleButton"));
        shuffleButton->setMinimumSize(QSize(28, 28));
        shuffleButton->setMaximumSize(QSize(28, 28));
        shuffleButton->setFocusPolicy(Qt::StrongFocus);
        QIcon icon8;
        iconThemeName = QString::fromUtf8("media-playlist-shuffle");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon8 = QIcon::fromTheme(iconThemeName);
        } else {
            icon8.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        shuffleButton->setIcon(icon8);
        shuffleButton->setIconSize(QSize(18, 18));
        shuffleButton->setCheckable(true);

        horizontalLayout_2->addWidget(shuffleButton);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        volumeLabel = new QLabel(centralwidget);
        volumeLabel->setObjectName(QString::fromUtf8("volumeLabel"));
        QFont font;
        font.setPointSize(8);
        volumeLabel->setFont(font);
        volumeLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        verticalLayout_2->addWidget(volumeLabel);

        volumeSlider = new QSlider(centralwidget);
        volumeSlider->setObjectName(QString::fromUtf8("volumeSlider"));
        sizePolicy1.setHeightForWidth(volumeSlider->sizePolicy().hasHeightForWidth());
        volumeSlider->setSizePolicy(sizePolicy1);
        volumeSlider->setMinimumSize(QSize(88, 0));
        volumeSlider->setMaximumSize(QSize(88, 16777215));
        volumeSlider->setMaximum(100);
        volumeSlider->setValue(100);
        volumeSlider->setOrientation(Qt::Horizontal);
        volumeSlider->setInvertedAppearance(false);
        volumeSlider->setInvertedControls(false);

        verticalLayout_2->addWidget(volumeSlider);


        horizontalLayout_2->addLayout(verticalLayout_2);


        verticalLayout->addLayout(horizontalLayout_2);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

        mainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(mainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 400, 22));
        menuMedia = new QMenu(menubar);
        menuMedia->setObjectName(QString::fromUtf8("menuMedia"));
        menuAudio = new QMenu(menubar);
        menuAudio->setObjectName(QString::fromUtf8("menuAudio"));
        menuVideo = new QMenu(menubar);
        menuVideo->setObjectName(QString::fromUtf8("menuVideo"));
        menuView = new QMenu(menubar);
        menuView->setObjectName(QString::fromUtf8("menuView"));
        mainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(mainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        mainWindow->setStatusBar(statusbar);

        menubar->addAction(menuMedia->menuAction());
        menubar->addAction(menuAudio->menuAction());
        menubar->addAction(menuVideo->menuAction());
        menubar->addAction(menuView->menuAction());
        menuMedia->addAction(actionOpen_File);
        menuMedia->addSeparator();
        menuMedia->addAction(actionQuit);
        menuAudio->addAction(actionAudio_Track);
        menuVideo->addAction(actionVideo_Track);
        menuView->addAction(actionPlaylist);

        retranslateUi(mainWindow);
        QObject::connect(actionQuit, SIGNAL(activated()), mainWindow, SLOT(close()));
        QObject::connect(actionOpen_File, SIGNAL(activated()), mainWindow, SLOT(openVideoFile()));
        QObject::connect(fullscreenButton, SIGNAL(clicked()), mainWindow, SLOT(toggleFullscreen()));
        QObject::connect(nextButton, SIGNAL(clicked()), mainWindow, SLOT(nextVideo()));
        QObject::connect(playButton, SIGNAL(clicked()), mainWindow, SLOT(togglePlayVideo()));
        QObject::connect(prevButton, SIGNAL(clicked()), mainWindow, SLOT(previousVideo()));
        QObject::connect(progressSlider, SIGNAL(valueChanged(int)), mainWindow, SLOT(skipVideo(int)));
        QObject::connect(repeatButton, SIGNAL(toggled(bool)), mainWindow, SLOT(toggleRepeat(bool)));
        QObject::connect(shuffleButton, SIGNAL(toggled(bool)), mainWindow, SLOT(toggleShuffle(bool)));
        QObject::connect(stopButton, SIGNAL(clicked()), mainWindow, SLOT(stopVideo()));
        QObject::connect(volumeSlider, SIGNAL(valueChanged(int)), mainWindow, SLOT(changeVolume(int)));

        QMetaObject::connectSlotsByName(mainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *mainWindow)
    {
        mainWindow->setWindowTitle(QApplication::translate("mainWindow", "OpenGL Video Player", 0, QApplication::UnicodeUTF8));
        actionOpen_File->setText(QApplication::translate("mainWindow", "Open File", 0, QApplication::UnicodeUTF8));
        actionQuit->setText(QApplication::translate("mainWindow", "Quit", 0, QApplication::UnicodeUTF8));
        actionAudio_Track->setText(QApplication::translate("mainWindow", "Audio Track", 0, QApplication::UnicodeUTF8));
        actionVideo_Track->setText(QApplication::translate("mainWindow", "Video Track", 0, QApplication::UnicodeUTF8));
        actionPlaylist->setText(QApplication::translate("mainWindow", "Playlist", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        progressLabel->setToolTip(QApplication::translate("mainWindow", "Elapsed time", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        progressLabel->setText(QApplication::translate("mainWindow", "00:00", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        movieLengthLabel->setToolTip(QApplication::translate("mainWindow", "Total time", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        movieLengthLabel->setText(QApplication::translate("mainWindow", "00:00", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        playButton->setToolTip(QApplication::translate("mainWindow", "Start play back", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        playButton->setStatusTip(QString());
#endif // QT_NO_STATUSTIP
#ifndef QT_NO_TOOLTIP
        prevButton->setToolTip(QApplication::translate("mainWindow", "Previous video", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        prevButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        stopButton->setToolTip(QApplication::translate("mainWindow", "Stop playback", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        stopButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        nextButton->setToolTip(QApplication::translate("mainWindow", "Next video", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        nextButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        fullscreenButton->setToolTip(QApplication::translate("mainWindow", "Fullscreen", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        fullscreenButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        repeatButton->setToolTip(QApplication::translate("mainWindow", "Repeat", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        repeatButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        shuffleButton->setToolTip(QApplication::translate("mainWindow", "Shuffle", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        shuffleButton->setText(QString());
        volumeLabel->setText(QApplication::translate("mainWindow", "100%", 0, QApplication::UnicodeUTF8));
        menuMedia->setTitle(QApplication::translate("mainWindow", "Media", 0, QApplication::UnicodeUTF8));
        menuAudio->setTitle(QApplication::translate("mainWindow", "Audio", 0, QApplication::UnicodeUTF8));
        menuVideo->setTitle(QApplication::translate("mainWindow", "Video", 0, QApplication::UnicodeUTF8));
        menuView->setTitle(QApplication::translate("mainWindow", "View", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class mainWindow: public Ui_mainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif /* VIDEO_PLAYER_GUI_H_ */
