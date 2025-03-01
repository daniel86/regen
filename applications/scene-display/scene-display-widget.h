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
#include <regen/camera/camera-controller.h>
#include <regen/camera/key-frame-controller.h>
#include <regen/camera/camera-anchor.h>
#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/animations/animation-node.h>
#include <applications/qt/qt-application.h>
#include <applications/qt/qt-camera-events.h>
#include <applications/qt/shader-input-widget.h>
#include "ui_scene-display-gui.h"
#include "regen/av/video-recorder.h"

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

	void toggleOffCameraTransform();

	void toggleOnCameraTransform();

	void updateGameTimeWidget();

public slots:

	void openFile();

	void toggleInputsDialog();

	void toggleCameraPopup();

	void updateSize();

	void nextView();

	void previousView();

	void nextAnchor();

	void playAnchor();

	void makeVideo(bool isClicked);

	void toggleWireframe();

	void toggleVSync();

	void toggleInfo(bool isOn);

	void onWorldTimeChanged();

	void onWorldTimeFactorChanged(double value);

protected:
	std::list<ref_ptr<EventHandler> > eventHandler_;
	std::list<ref_ptr<Animation> > animations_;
	std::map<std::string, NamedObject> namedObjects_;
	ref_ptr<Animation> fbsWidgetUpdater_;
	ref_ptr<Animation> loadAnim_;
	ref_ptr<VideoRecorder> videoRecorder_;

	ref_ptr<Camera> mainCamera_;
	ref_ptr<CameraController> cameraController_;

	ref_ptr<KeyFrameController> anchorAnim_;
	std::vector<ref_ptr<CameraAnchor>> anchors_;
	GLuint anchorIndex_;
	GLdouble anchorEaseInOutIntensity_;
	GLdouble anchorPauseTime_;
	GLdouble anchorTimeScale_;

	QDialog *inputDialog_;
	ShaderInputWidget *inputWidget_;
	QtApplication *app_;
	ref_ptr<BulletPhysics> physics_;
	std::map<std::string, ref_ptr<SpatialIndex>> spatialIndices_;
	Ui_sceneViewer ui_;
	std::string activeFile_;
	ViewNodeList viewNodes_;
	ViewNodeList::iterator activeView_;

	ref_ptr<Animation> timeWidgetAnimation_;
	boost::posix_time::ptime lastUpdateTime_;

	ref_ptr<State> wireframeState_;
	std::map<std::string, ref_ptr<Light>> lightStates_;

	void loadScene(const std::string &sceneFile);

	void loadSceneGraphicsThread(const std::string &sceneFile);

	void resizeEvent(QResizeEvent *event);

	static double getAnchorTime(const Vec3f &fromPosition, const Vec3f &toPosition);

	void activateAnchor();

	void handleCameraConfiguration(
		scene::SceneParser &sceneParser,
		const ref_ptr<regen::scene::SceneInputNode> &cameraNode);

	friend class SceneLoaderAnimation;
};

#endif /* SCENE_DISPLAY_WIDGET_H_ */
