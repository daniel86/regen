#ifndef NOISE_WIDGET_H_
#define NOISE_WIDGET_H_

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtCore/QTimer>

#include <applications/qt/qt-application.h>

#include <applications/noise-gui/ui_noise-gui.h>
#include "regen/textures/noise-texture.h"

using namespace regen;

class NoiseWidget : public QMainWindow, public Animation {
Q_OBJECT

public:
	NoiseWidget(QtApplication *app);

	auto &texture() const { return texture_; }

	void gl_loadScene();

	// Override
	void call(EventObject *ev, EventData *data);

	void cpuUpdate(double dt);

	void gpuUpdate(RenderState *rs, double dt);

public slots:

	void updateSize();

	void updateTexture();

	void updateTextureSelection();

	void toggleFullscreen();

	void addInput();

	void removeInput();

	void addNoiseModule();

	void removeNoiseModule();

	void loadPerlin();

	void loadClouds();

	void loadGranite();

	void loadWood();

	void saveImage();

protected:
	QtApplication *app_;
	Ui_mainWindow ui_;
	ref_ptr<NoiseTexture2D> texture_;
	bool updateTexture_ = true;

	std::map<std::string, ref_ptr<NoiseGenerator>> noiseGenerators_;

	void mousePressEvent(QMouseEvent *event);

	void mouseDoubleClickEvent(QMouseEvent *event);

	void mouseReleaseEvent(QMouseEvent *event);

	void keyPressEvent(QKeyEvent *event);

	void keyReleaseEvent(QKeyEvent *event);

	void resizeEvent(QResizeEvent *event);

	void updateNoiseGenerators(const ref_ptr<NoiseGenerator> &generator);

	void updateWidgets(const ref_ptr<NoiseGenerator> &generator);

	void updateTable(const ref_ptr<NoiseGenerator> &generator);

	void updateList(const ref_ptr<NoiseGenerator> &generator);

	void addProperty(
			std::string_view name,
			double min,
			double max,
			double value,
			const std::function<void(double)> &setter);

	void addProperty_i(
			std::string_view name,
			int min,
			int max,
			int value,
			const std::function<void(int)> &setter);
};

#endif /* NOISE_WIDGET_H_ */
