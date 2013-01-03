/********************************************************************************
** Form generated from reading UI file 'video-player-guiQa2967.ui'
**
** Created: Thu Jan 3 22:59:54 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef VIDEO_2D_PLAYER_2D_GUIQA2967_H
#define VIDEO_2D_PLAYER_2D_GUIQA2967_H

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
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QTableWidget>
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
    QGridLayout *gridLayout_2;
    QVBoxLayout *mainLayout;
    QSplitter *splitter;
    QWidget *blackBackground;
    QGridLayout *gridLayout;
    QWidget *glWidget;
    QGridLayout *gridLayout_6;
    QGridLayout *glWidgetLayout;
    QTableWidget *playlistTable;
    QHBoxLayout *progressLayout;
    QSpacerItem *progressSpacer1;
    QLabel *progressLabel;
    QSlider *progressSlider;
    QLabel *movieLengthLabel;
    QSpacerItem *progressSpacer0;
    QHBoxLayout *buttonBarLayout;
    QPushButton *playButton;
    QSpacerItem *buttonSpacer0;
    QPushButton *prevButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QSpacerItem *buttonSpacer1;
    QPushButton *fullscreenButton;
    QSpacerItem *buttonSpacer3;
    QPushButton *repeatButton;
    QPushButton *shuffleButton;
    QSpacerItem *buttonSpacer2;
    QVBoxLayout *volumeLayout;
    QLabel *volumeLabel;
    QSlider *volumeSlider;
    QMenuBar *menubar;
    QMenu *menuMedia;
    QMenu *menuAudio;
    QMenu *menuVideo;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *mainWindow)
    {
        if (mainWindow->objectName().isEmpty())
            mainWindow->setObjectName(QString::fromUtf8("mainWindow"));
        mainWindow->resize(443, 340);
        actionOpen_File = new QAction(mainWindow);
        actionOpen_File->setObjectName(QString::fromUtf8("actionOpen_File"));
        QIcon icon;
        QString iconThemeName = QString::fromUtf8("document-open");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        actionOpen_File->setIcon(icon);
        actionOpen_File->setIconVisibleInMenu(true);
        actionQuit = new QAction(mainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        QIcon icon1;
        iconThemeName = QString::fromUtf8("application-exit");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
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
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        mainLayout = new QVBoxLayout();
        mainLayout->setObjectName(QString::fromUtf8("mainLayout"));
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy);
        splitter->setOrientation(Qt::Horizontal);
        blackBackground = new QWidget(splitter);
        blackBackground->setObjectName(QString::fromUtf8("blackBackground"));
        sizePolicy.setHeightForWidth(blackBackground->sizePolicy().hasHeightForWidth());
        blackBackground->setSizePolicy(sizePolicy);
        blackBackground->setMinimumSize(QSize(48, 48));
        blackBackground->setFocusPolicy(Qt::NoFocus);
        blackBackground->setStyleSheet(QString::fromUtf8("background-color: black;"));
        gridLayout = new QGridLayout(blackBackground);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        glWidget = new QWidget(blackBackground);
        glWidget->setObjectName(QString::fromUtf8("glWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(glWidget->sizePolicy().hasHeightForWidth());
        glWidget->setSizePolicy(sizePolicy1);
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

        splitter->addWidget(blackBackground);
        playlistTable = new QTableWidget(splitter);
        if (playlistTable->columnCount() < 2)
            playlistTable->setColumnCount(2);
        QFont font;
        font.setPointSize(8);
        font.setBold(true);
        font.setUnderline(false);
        font.setWeight(75);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        __qtablewidgetitem->setFont(font);
        playlistTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QFont font1;
        font1.setPointSize(8);
        font1.setBold(true);
        font1.setWeight(75);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        __qtablewidgetitem1->setFont(font1);
        playlistTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        playlistTable->setObjectName(QString::fromUtf8("playlistTable"));
        sizePolicy.setHeightForWidth(playlistTable->sizePolicy().hasHeightForWidth());
        playlistTable->setSizePolicy(sizePolicy);
        playlistTable->setAcceptDrops(true);
        playlistTable->setFrameShape(QFrame::StyledPanel);
        playlistTable->setFrameShadow(QFrame::Plain);
        playlistTable->setAutoScroll(false);
        playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        playlistTable->setDragEnabled(true);
        playlistTable->setDragDropMode(QAbstractItemView::DropOnly);
        playlistTable->setAlternatingRowColors(true);
        playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
        playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        playlistTable->setTextElideMode(Qt::ElideNone);
        playlistTable->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
        playlistTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        playlistTable->setShowGrid(false);
        playlistTable->setGridStyle(Qt::SolidLine);
        playlistTable->setSortingEnabled(true);
        playlistTable->setWordWrap(false);
        playlistTable->setCornerButtonEnabled(false);
        splitter->addWidget(playlistTable);
        playlistTable->horizontalHeader()->setVisible(true);
        playlistTable->horizontalHeader()->setCascadingSectionResizes(false);
        playlistTable->horizontalHeader()->setDefaultSectionSize(200);
        playlistTable->horizontalHeader()->setHighlightSections(true);
        playlistTable->horizontalHeader()->setMinimumSectionSize(75);
        playlistTable->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        playlistTable->horizontalHeader()->setStretchLastSection(true);
        playlistTable->verticalHeader()->setVisible(false);
        playlistTable->verticalHeader()->setDefaultSectionSize(18);

        mainLayout->addWidget(splitter);

        progressLayout = new QHBoxLayout();
        progressLayout->setSpacing(2);
        progressLayout->setObjectName(QString::fromUtf8("progressLayout"));
        progressSpacer1 = new QSpacerItem(8, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        progressLayout->addItem(progressSpacer1);

        progressLabel = new QLabel(centralwidget);
        progressLabel->setObjectName(QString::fromUtf8("progressLabel"));
        progressLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        progressLayout->addWidget(progressLabel);

        progressSlider = new QSlider(centralwidget);
        progressSlider->setObjectName(QString::fromUtf8("progressSlider"));
        progressSlider->setAcceptDrops(false);
        progressSlider->setMaximum(100000);
        progressSlider->setPageStep(10);
        progressSlider->setTracking(true);
        progressSlider->setOrientation(Qt::Horizontal);

        progressLayout->addWidget(progressSlider);

        movieLengthLabel = new QLabel(centralwidget);
        movieLengthLabel->setObjectName(QString::fromUtf8("movieLengthLabel"));

        progressLayout->addWidget(movieLengthLabel);

        progressSpacer0 = new QSpacerItem(8, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        progressLayout->addItem(progressSpacer0);


        mainLayout->addLayout(progressLayout);

        buttonBarLayout = new QHBoxLayout();
        buttonBarLayout->setSpacing(2);
        buttonBarLayout->setObjectName(QString::fromUtf8("buttonBarLayout"));
        buttonBarLayout->setContentsMargins(2, -1, 2, 2);
        playButton = new QPushButton(centralwidget);
        playButton->setObjectName(QString::fromUtf8("playButton"));
        sizePolicy1.setHeightForWidth(playButton->sizePolicy().hasHeightForWidth());
        playButton->setSizePolicy(sizePolicy1);
        playButton->setMinimumSize(QSize(34, 34));
        playButton->setMaximumSize(QSize(34, 34));
        playButton->setText(QString::fromUtf8(""));
        QIcon icon2;
        iconThemeName = QString::fromUtf8("media-playback-start");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon2 = QIcon::fromTheme(iconThemeName);
        } else {
            icon2.addFile(QString::fromUtf8("../../../../../"), QSize(), QIcon::Normal, QIcon::Off);
        }
        playButton->setIcon(icon2);

        buttonBarLayout->addWidget(playButton);

        buttonSpacer0 = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        buttonBarLayout->addItem(buttonSpacer0);

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

        buttonBarLayout->addWidget(prevButton);

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

        buttonBarLayout->addWidget(stopButton);

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

        buttonBarLayout->addWidget(nextButton);

        buttonSpacer1 = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        buttonBarLayout->addItem(buttonSpacer1);

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

        buttonBarLayout->addWidget(fullscreenButton);

        buttonSpacer3 = new QSpacerItem(14, 2, QSizePolicy::Fixed, QSizePolicy::Minimum);

        buttonBarLayout->addItem(buttonSpacer3);

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

        buttonBarLayout->addWidget(repeatButton);

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

        buttonBarLayout->addWidget(shuffleButton);

        buttonSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        buttonBarLayout->addItem(buttonSpacer2);

        volumeLayout = new QVBoxLayout();
        volumeLayout->setSpacing(0);
        volumeLayout->setObjectName(QString::fromUtf8("volumeLayout"));
        volumeLabel = new QLabel(centralwidget);
        volumeLabel->setObjectName(QString::fromUtf8("volumeLabel"));
        QFont font2;
        font2.setPointSize(8);
        volumeLabel->setFont(font2);
        volumeLabel->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        volumeLayout->addWidget(volumeLabel);

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

        volumeLayout->addWidget(volumeSlider);


        buttonBarLayout->addLayout(volumeLayout);


        mainLayout->addLayout(buttonBarLayout);


        gridLayout_2->addLayout(mainLayout, 0, 0, 1, 1);

        mainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(mainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 443, 22));
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
        QObject::connect(progressSlider, SIGNAL(valueChanged(int)), mainWindow, SLOT(seekVideo(int)));
        QObject::connect(repeatButton, SIGNAL(toggled(bool)), mainWindow, SLOT(toggleRepeat(bool)));
        QObject::connect(shuffleButton, SIGNAL(toggled(bool)), mainWindow, SLOT(toggleShuffle(bool)));
        QObject::connect(stopButton, SIGNAL(clicked()), mainWindow, SLOT(stopVideo()));
        QObject::connect(volumeSlider, SIGNAL(valueChanged(int)), mainWindow, SLOT(changeVolume(int)));
        QObject::connect(splitter, SIGNAL(splitterMoved(int,int)), mainWindow, SLOT(updateSize()));
        QObject::connect(playlistTable, SIGNAL(itemActivated(QTableWidgetItem*)), mainWindow, SLOT(playlistActivated(QTableWidgetItem*)));

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
        QTableWidgetItem *___qtablewidgetitem = playlistTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("mainWindow", "Title", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = playlistTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("mainWindow", "Length", 0, QApplication::UnicodeUTF8));
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

#endif // VIDEO_2D_PLAYER_2D_GUIQA2967_H
