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

#include <ogle/av/video-texture.h>
#include <applications/qt-ogle-application.h>

#include <applications/video-player/video-player-gui.h>
using namespace ogle;

class VideoPlayerWidget : public QMainWindow, public EventCallable
{
Q_OBJECT

public:
  VideoPlayerWidget(QtOGLEApplication *app);

  ref_ptr<Texture> texture() const;
  const ref_ptr<VideoTexture>& video() const;

  // EventCallable Override
  virtual void call(EventObject *ev, void *data);

  /**
   * Adds a single item to the playlist.
   */
  int addPlaylistItem(const string &filePath);
  /**
   * Recursively adds all video files that are contained
   * within the given file path.
   */
  void addLocalPath(const string &filePath);

public slots:
  void updateSize();

  void togglePlayVideo();
  void toggleFullscreen();
  void toggleShuffle(bool);
  void toggleRepeat(bool);

  void openVideoFile();

  void playlistActivated(QTableWidgetItem*);
  void nextVideo();
  void previousVideo();
  void stopVideo();
  void seekVideo(int);

  void changeVolume(int);
  void updateElapsedTime();

protected:
  QtOGLEApplication *app_;
  Ui_mainWindow ui_;
  ref_ptr<VideoTexture> vid_;
  GLfloat gain_;
  QTimer elapsedTimer_;
  QTableWidgetItem *activePlaylistRow_;

  void keyPressEvent(QKeyEvent* event);
  void keyReleaseEvent(QKeyEvent *event);
  void resizeEvent(QResizeEvent * event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dropEvent(QDropEvent *event);

  void setVideoFile(const string &filePath);
  void activatePlaylistRow(int row);
};

#endif /* VIDEO_PLAYER_WIDGET_H_ */
