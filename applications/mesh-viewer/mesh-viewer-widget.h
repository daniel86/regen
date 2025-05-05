#ifndef MESH_VIEWER_WIDGET_H_
#define MESH_VIEWER_WIDGET_H_

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtCore/QTimer>

#include <regen/av/video-texture.h>
#include <applications/qt/qt-application.h>
#include <QSettings>

#include "ui_mesh-viewer-gui.h"
#include "regen/meshes/assimp-importer.h"
#include "regen/camera/camera-controller.h"
#include "applications/qt/shader-input-widget.h"

using namespace regen;

class MeshViewerWidget : public QMainWindow {
Q_OBJECT

public:
	explicit MeshViewerWidget(QtApplication *app);

	void transformMesh(GLdouble dt);

public slots:

	void openAssetFile();

	void loadMeshes();

	void updateSize();

	void toggleFullscreen();

	void toggleControls();

	void toggleWireframe(bool);

	void toggleRotate(bool);

	void activateLoD_0();

	void activateLoD_1();

	void activateLoD_2();

	void activateLoD_3();

	void activateMeshIndex(int);

	void toggleInputsDialog();

	void gl_loadScene();

protected:
	QtApplication *app_;
	QVBoxLayout *fullscreenLayout_;
	Ui_mainWindow ui_;

	GLboolean controlsShown_;
	GLboolean wereControlsShown_;
	QList<int> splitterSizes_;

	ref_ptr<StateNode> sceneRoot_;
	ref_ptr<StateNode> meshRoot_;
	ref_ptr<Camera> userCamera_;
	ref_ptr<Light> sceneLight_[3];
	ref_ptr<CameraController> cameraController_;
	ref_ptr<State> wireframeState_;
	std::vector<ref_ptr<StateNode>> meshNodes_;

	QDialog *inputDialog_ = nullptr;
	ShaderInputWidget *inputWidget_ = nullptr;

	ref_ptr<AssetImporter> asset_;
	std::vector<ref_ptr<Mesh>> meshes_;
	ref_ptr<ModelTransformation> modelTransform_;
	Quaternion meshQuaternion_;
	float meshOrientation_ = 0.0f;
	float meshScale_ = 1.0f;
	Vec3f meshOrigin_ = Vec3f::zero();
	uint32_t currentLodLevel_ = 0;
	int32_t currentMeshIndex_ = -1;
	ref_ptr<Animation> rotateAnim_;

	QSettings settings_;
	std::string assetFilePath_;

	void loadResources_GL();

	void loadMeshes_GL(const std::string &assetPath);

	void loadAnimation(const ref_ptr<Mesh> &mesh, uint32_t index);

	void simplifyMesh_GL(const Vec4f &simplificationFactors);

	void selectMesh(int32_t meshIndex, uint32_t lodIndex);

	void selectMesh_(uint32_t meshIndex, uint32_t lodIndex);

	void activateLoD(int lodLevel, const ref_ptr<Mesh> &mesh);

	void activateLoD(int lodLevel);

	void updateLoDButtons();

	void setAssImpFlags();

	void createCameraController();
};

#endif /* MESH_VIEWER_WIDGET_H_ */
