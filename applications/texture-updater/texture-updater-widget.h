/*
 * texture-updater-widget.h
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#ifndef TEXTURE_UPDATER_WIDGET_H_
#define TEXTURE_UPDATER_WIDGET_H_

#include <QtGui/QMainWindow>

#include <regen/textures/texture-updater.h>

#include <applications/qt/qt-application.h>
#include <applications/texture-updater/texture-updater-gui.h>
using namespace regen;

class TextureUpdaterWidget : public QMainWindow
{
Q_OBJECT

public:
  TextureUpdaterWidget(QtApplication *app);
  ~TextureUpdaterWidget();

  const ref_ptr<TextureState>& texture() const;

  void readConfig();
  void writeConfig();

public slots:
  void openFile();
  void updateSize();

protected:
  QtApplication *app_;
  Ui_textureUpdater ui_;
  ref_ptr<TextureUpdater> texUpdater_;
  ref_ptr<TextureState> texture_;
  ref_ptr<Animation> updaterLoader_;
  string textureUpdaterFile_;

  void resizeEvent(QResizeEvent *event);

  TextureUpdaterWidget(const TextureUpdaterWidget&);
};

#endif /* TEXTURE_UPDATER_WIDGET_H_ */
