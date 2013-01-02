/*
 * video-player-widget.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <iostream>
using namespace std;

#include "video-player-widget.h"

#include <QtGui/QFileDialog>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

#include <boost/filesystem/path.hpp>

#include <ogle/utility/string-util.h>

static QString formatTime(GLfloat elapsedSeconds)
{
  GLuint seconds = (GLuint)elapsedSeconds;
  GLuint minutes = seconds/60;
  seconds = seconds%60;
  string label = FORMAT_STRING(
      (minutes<10 ? "0" : "") << minutes <<
      ":" <<
      (seconds<10 ? "0" : "") << seconds);
  return QString(label.c_str());
}

VideoPlayerWidget::VideoPlayerWidget(QtOGLEApplication *app)
: QMainWindow(),
  EventCallable(),
  app_(app),
  gain_(1.0f),
  elapsedTimer_(this)
{
  setMouseTracking(true);
  ui_.setupUi(this, &app_->glWidget());

  vid_ = ref_ptr<VideoTexture>::manage(new VideoTexture);
  ui_.repeatButton->click();

  // update elapsed time label
  elapsedTimer_.setInterval(1000);
  connect(&elapsedTimer_, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
  elapsedTimer_.start();
}

ref_ptr<Texture> VideoPlayerWidget::texture() const
{
  return ref_ptr<Texture>::cast(vid_);
}
const ref_ptr<VideoTexture>& VideoPlayerWidget::video() const
{
  return vid_;
}

void VideoPlayerWidget::call(EventObject *ev, void *data)
{
  OGLEApplication::KeyEvent *keyEv = static_cast<OGLEApplication::KeyEvent*>(data);
  if(keyEv != NULL) {
    if(!keyEv->isUp) { return; }

    if(keyEv->keyValue == Qt::Key_Left) {
      vid_->seekBackward(10.0);
    }
    else if(keyEv->keyValue == Qt::Key_Right) {
      vid_->seekForward(10.0);
    }
    else if(keyEv->keyValue == Qt::Key_Up) {
      vid_->seekForward(60.0);
    }
    else if(keyEv->keyValue == Qt::Key_Down) {
      vid_->seekBackward(60.0);
    }
    else if(keyEv->key == 'f' || keyEv->key == 'F') {
      toggleFullscreen();
    }
    else if(keyEv->key == ' ') {
      togglePlayVideo();
    }
    else if(keyEv->key == 'o' || keyEv->key == 'O') {
      openVideoFile();
    }
  }

  OGLEApplication::ButtonEvent *mouseEv = static_cast<OGLEApplication::ButtonEvent*>(data);
  if(mouseEv != NULL) {
    if(mouseEv->isDoubleClick && mouseEv->button==1) {
      toggleFullscreen();
    }
  }
}

void VideoPlayerWidget::resizeEvent(QResizeEvent * event)
{
  updateSize();
}
void VideoPlayerWidget::updateSize()
{
  GLfloat widgetRatio = ui_.blackBackground->width()/(GLfloat)ui_.blackBackground->height();
  GLfloat videoRatio = vid_->width()/(GLfloat)vid_->height();
  if(widgetRatio>videoRatio) {
    ui_.glWidget->setMinimumSize(
        (GLint)(ui_.blackBackground->height()*videoRatio),
        ui_.blackBackground->height());
  }
  else {
    ui_.glWidget->setMinimumSize(
        ui_.blackBackground->width(),
        (GLint)(ui_.blackBackground->width()/videoRatio));
  }
}

void VideoPlayerWidget::keyPressEvent(QKeyEvent* event)
{
  app_->keyDown(event->key(),app_->mouseX(),app_->mouseY());
}
void VideoPlayerWidget::keyReleaseEvent(QKeyEvent *event)
{
  app_->keyUp(event->key(),app_->mouseX(),app_->mouseY());
}

void VideoPlayerWidget::togglePlayVideo()
{
  if(vid_->isFileSet()) {
    vid_->togglePlay();
    if(vid_->isPlaying()) {
      ui_.playButton->setIcon(QIcon(QIcon::fromTheme(
          QString::fromUtf8("media-playback-pause"))));
    } else {
      ui_.playButton->setIcon(QIcon(QIcon::fromTheme(
          QString::fromUtf8("media-playback-start"))));
    }
  }
}

void VideoPlayerWidget::stopVideo()
{
  if(vid_->isFileSet()) {
    vid_->stop();
    ui_.playButton->setIcon(QIcon(QIcon::fromTheme(
        QString::fromUtf8("media-playback-start"))));
  }
}

void VideoPlayerWidget::changeVolume(int val)
{
  gain_ = ((float)val)/100.0f;
  if(vid_->audioSource().get()) {
    vid_->audioSource()->set_gain(gain_);
  }
  string label = FORMAT_STRING(val << "%");
  ui_.volumeLabel->setText(QString(label.c_str()));
}

void VideoPlayerWidget::updateElapsedTime()
{
  GLfloat elapsed = vid_->elapsedSeconds();
  GLfloat total = vid_->totalSeconds();
  ui_.progressLabel->setText(formatTime(elapsed));
  ui_.progressSlider->blockSignals(true);
  ui_.progressSlider->setValue((int) (100000.0f*elapsed/total));
  ui_.progressSlider->blockSignals(false);
}

void VideoPlayerWidget::openVideoFile()
{
  QWidget *parent = NULL;
  QFileDialog dialog(parent);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setFilter(QDir::System | QDir::AllEntries | QDir::Hidden);
  dialog.setFilter("Videos (*.avi *.mpg);;All files (*.*)");
  dialog.setViewMode(QFileDialog::Detail);
  if(!dialog.exec()) { return; }

  QStringList fileNames = dialog.selectedFiles();
  QString selected = fileNames.first();
  string filePath = selected.toStdString();

  vid_->set_file(filePath);
  boost::filesystem::path bdir(filePath.c_str());
  app_->set_windowTitle(bdir.filename().c_str());
  vid_->play();
  if(vid_->audioSource().get()) {
    vid_->audioSource()->set_gain(gain_);
  }
  ui_.playButton->setIcon(QIcon(QIcon::fromTheme(
      QString::fromUtf8("media-playback-pause"))));
  ui_.movieLengthLabel->setText(formatTime(vid_->totalSeconds()));
  updateElapsedTime();
  updateSize();
}

void VideoPlayerWidget::skipVideo(int val)
{
  vid_->seekTo(((float)val)/100000.0f);
}

static void hideLayout(QLayout *layout)
{
  for(GLint i=0; i<layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if(item->widget()) { item->widget()->hide(); }
    if(item->layout()) { hideLayout(item->layout()); }
  }
}
static void showLayout(QLayout *layout)
{
  for(GLint i=0; i<layout->count(); ++i) {
    QLayoutItem *item = layout->itemAt(i);
    if(item->widget()) { item->widget()->show(); }
    if(item->layout()) { showLayout(item->layout()); }
  }
}

void VideoPlayerWidget::toggleFullscreen()
{
  QLayoutItem *item;
  while ((item = ui_.verticalLayout->takeAt(0)) != 0) {
    ui_.verticalLayout->removeItem(item);
  }
  ui_.horizontalLayout->setParent(NULL);
  ui_.horizontalLayout_2->setParent(NULL);

  if(isFullScreen()) {
    showNormal();

    ui_.verticalLayout->addWidget(ui_.blackBackground);
    ui_.verticalLayout->addLayout(ui_.horizontalLayout);
    ui_.verticalLayout->addLayout(ui_.horizontalLayout_2);
    ui_.menubar->show();
    ui_.statusbar->show();
    showLayout(ui_.horizontalLayout);
    showLayout(ui_.horizontalLayout_2);
  }
  else {
    ui_.verticalLayout->addWidget(ui_.blackBackground);
    ui_.menubar->hide();
    ui_.statusbar->hide();
    hideLayout(ui_.horizontalLayout);
    hideLayout(ui_.horizontalLayout_2);

    showFullScreen();
  }
}

void VideoPlayerWidget::toggleRepeat(bool v)
{
  // XXX
}
void VideoPlayerWidget::toggleShuffle(bool v)
{
  // XXX
}

void VideoPlayerWidget::nextVideo()
{
  // XXX
}
void VideoPlayerWidget::previousVideo()
{
  // XXX
}
