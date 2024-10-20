/*
 * scene-display-widget.h
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#ifndef SCENE_DISPLAY_WIDGET_H_
#define SCENE_DISPLAY_WIDGET_H_

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QFileDialog>
#include <QtCore/QString>

#include <regen/physics/bullet-physics.h>
#include <regen/camera/camera-manipulator.h>
#include <regen/animations/animation-node.h>
#include <applications/qt/qt-application.h>
#include <applications/qt/qt-camera-events.h>
#include <applications/qt/shader-input-widget.h>
#include "scene-display-gui.h"

using namespace regen;

struct ViewNode {
	std::string name;
	ref_ptr<StateNode> node;
};
typedef std::list<ViewNode> ViewNodeList;

class SceneDisplayWidget : public QMainWindow {
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

	void nextView();

	void previousView();

protected:
	std::list<ref_ptr<EventHandler> > eventHandler_;
	std::list<ref_ptr<Animation> > animations_;
	ref_ptr<Animation> fbsWidgetUpdater_;
	ref_ptr<Animation> loadAnim_;

	QDialog *inputDialog_;
	ShaderInputWidget *inputWidget_;
	QtApplication *app_;
	ref_ptr<BulletPhysics> physics_;
	Ui_sceneViewer ui_;
	std::string activeFile_;
	ViewNodeList viewNodes_;
	ViewNodeList::iterator activeView_;

	void loadScene(const std::string &sceneFile);

	void loadSceneGraphicsThread(const std::string &sceneFile);

	void resizeEvent(QResizeEvent *event);

	friend class SceneLoaderAnimation;
};

#endif /* SCENE_DISPLAY_WIDGET_H_ */
