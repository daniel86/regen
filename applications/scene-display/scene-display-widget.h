/*
 * scene-display-widget.h
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#ifndef SCENE_DISPLAY_WIDGET_H_
#define SCENE_DISPLAY_WIDGET_H_

#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QString>

#include <regen/physics/bullet-physics.h>
#include <regen/camera/camera-manipulator.h>
#include <regen/animations/animation-node.h>
#include <applications/qt/qt-application.h>
#include <applications/qt/shader-input-widget.h>
#include "scene-display-gui.h"
using namespace regen;

class SceneDisplayWidget : public QMainWindow
{
Q_OBJECT

public:
  SceneDisplayWidget(QtApplication *app);
  ~SceneDisplayWidget();

  void resetFile();
  void readConfig();
  void writeConfig();
  void init();

public slots:
  void openFile();
  void toggleInputsDialog();
  void updateSize();

protected:
  ref_ptr<CameraManipulator> manipulator_;
  ref_ptr<Animation> fbsWidgetUpdater_;
  ref_ptr<Animation> loadAnim_;
  ref_ptr<EventHandler> camKeyHandler_;
  ref_ptr<EventHandler> camMotionHandler_;
  ref_ptr<EventHandler> camButtonHandler_;

  QDialog *inputDialog_;
  ShaderInputWidget *inputWidget_;
  QtApplication *app_;
  list< ref_ptr<EventHandler> > eventHandler_;
  ref_ptr<BulletPhysics> physics_;
  vector< ref_ptr<NodeAnimation> > nodeAnimations_;
  Ui_sceneViewer ui_;
  string activeFile_;

  void loadScene(const string &sceneFile);
  void loadSceneGraphicsThread(const string &sceneFile);
  void resizeEvent(QResizeEvent *event);

  friend class SceneLoaderAnimation;
};

#endif /* SCENE_DISPLAY_WIDGET_H_ */
