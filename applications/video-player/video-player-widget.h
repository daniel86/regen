/*
 * video-player-widget.h
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#ifndef VIDEO_PLAYER_WIDGET_H_
#define VIDEO_PLAYER_WIDGET_H_

#include <QtGui/QMainWindow>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtCore/QTimer>

#include <ogle/textures/video-texture.h>
#include <applications/qt-ogle-application.h>

#include <applications/video-player/video-player-gui.h>

class VideoPlayerWidget : public QMainWindow, public EventCallable
{
Q_OBJECT

public:
  VideoPlayerWidget(QtOGLEApplication *app);

  ref_ptr<Texture> texture() const;
  const ref_ptr<VideoTexture>& video() const;

  void updateSize();
  // EventCallable Override
  virtual void call(EventObject *ev, void *data);

public slots:
  void togglePlayVideo();
  void toggleFullscreen();
  void nextVideo();
  void previousVideo();
  void stopVideo();
  void changeVolume(int);
  void skipVideo(int);
  void toggleShuffle(bool);
  void toggleRepeat(bool);
  void openVideoFile();
  void updateElapsedTime();

protected:
  QtOGLEApplication *app_;
  Ui_mainWindow ui_;
  ref_ptr<VideoTexture> vid_;
  GLfloat gain_;
  QTimer elapsedTimer_;

  void keyPressEvent(QKeyEvent* event);
  void keyReleaseEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent * event);
};

#endif /* VIDEO_PLAYER_WIDGET_H_ */
