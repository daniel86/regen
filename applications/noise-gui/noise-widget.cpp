#include <iostream>

#include <QtGui/QMouseEvent>
#include <QtCore/QList>
#include <QtCore/QMimeData>

#include "regen/gl/states/blit-state.h"
#include "regen/scene/state-configurer.h"
#include "regen/textures/fbo-state.h"
#include "regen/passes/fullscreen-pass.h"
#include <regen/animation/animation-manager.h>
#include <QInputDialog>

#include "noise-widget.h"

#include <QFileDialog>

#include "regen/textures/devil-loader.h"

using namespace regen;
using namespace std;

////////////
////////////

NoiseWidget::NoiseWidget(QtApplication *app)
		: QMainWindow(),
		  Animation(true, true),
		  app_(app) {
	setMouseTracking(true);

	ui_.setupUi(this);
	app_->glWidget()->setEnabled(false);
	app_->glWidget()->setFocusPolicy(Qt::NoFocus);
	ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0, 0, 1, 1);

	resize(1600, 1200);
	ui_.splitter->setSizes(QList<int>({1200, 400}));
	updateSize();
	app_->withGLContext([this]() { gl_loadScene(); });
}

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler {
public:
	FBOResizer(const ref_ptr<FBOState> &fbo, float wScale, float hScale)
			: EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) {}

	~FBOResizer() override = default;

	void call(EventObject *evObject, EventData *) {
		auto *app = (Scene *) evObject;
		auto winSize = app->screen()->viewport();
		fboState_->resize(winSize.r.x, winSize.r.y);
	}

protected:
	ref_ptr<FBOState> fboState_;
	float wScale_, hScale_;
};

void NoiseWidget::gl_loadScene() {
	AnimationManager::get().pause(true);
	AnimationManager::get().setRootState(app_->renderTree()->state());

	// create render target
	auto winSize = app_->screen()->viewport().r;
	ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(winSize.x, winSize.y);
	ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE);
	ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
	fboState->setDrawBuffers({GL_COLOR_ATTACHMENT0});
	fboState->setClearColor({
		Vec4f(0.26, 0.26, 0.36, 1.0),
		GL_COLOR_ATTACHMENT0 });

	// create a root node (that binds the render target)
	ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::alloc(fboState);
	app_->renderTree()->addChild(sceneRoot);

	// add the video widget to the root node
	texture_ = ref_ptr<NoiseTexture2D>::alloc(4096, 4096);

	auto pass = ref_ptr<FullscreenPass>::alloc("regen.filter.sampling");
	sceneRoot->state()->joinStates(
		ref_ptr<TextureState>::alloc(texture_, "inputTexture"));
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(sceneRoot.get());
	shaderConfigurer.addState(pass.get());
	pass->createShader(shaderConfigurer.cfg());
	sceneRoot->state()->joinStates(pass);

	//createTextureWidget(app_, texture_, sceneRoot);
	sceneRoot->state()->joinStates(ref_ptr<BlitToScreen>::alloc(
			fbo, app_->screen(),
			GL_COLOR_ATTACHMENT0,
			true));
	GL_ERROR_LOG();

	// resize fbo with window
	app_->connect(Scene::RESIZE_EVENT, ref_ptr<FBOResizer>::alloc(fboState, 1.0, 1.0));
	// Update frustum when window size changes
	//app_->connect(Scene::RESIZE_EVENT, ref_ptr<ProjectionUpdater>::alloc(userCamera_, app_->screen()));

	//updateSize();
	AnimationManager::get().resume();
	REGEN_INFO("Scene Loaded.");
}

//////////////////////////////
//////// Slots
//////////////////////////////

void NoiseWidget::updateSize() {
	auto w = ui_.blackBackground->width();
	auto h = ui_.blackBackground->height();
	ui_.glWidget->setMinimumSize(QSize(max(2, w), max(2, h)));
}

void NoiseWidget::updateTexture() {
	updateTexture_ = true;
}

void NoiseWidget::cpuUpdate(double dt) {}

void NoiseWidget::gpuUpdate(RenderState *rs, double dt) {
	if (!updateTexture_ || !texture_.get()) return;

	auto noiseModuleName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	if (noiseModuleName.empty()) { return; }
	auto generator = noiseGenerators_[noiseModuleName];
	if (!generator.get()) {
		REGEN_INFO("no generator found for " << noiseModuleName);
		return;
	}

	texture_->setNoiseGenerator(generator);
	updateTexture_ = false;
}

void NoiseWidget::updateNoiseGenerators(const ref_ptr<NoiseGenerator> &generator) {
	// create a <string, NoiseGenerator> map of all noise generators
	noiseGenerators_.clear();
	std::queue<ref_ptr<NoiseGenerator>> queue;
	queue.push(generator);
	while (!queue.empty()) {
		auto gen = queue.front();
		queue.pop();
		if (noiseGenerators_.find(gen->name()) != noiseGenerators_.end()) {
			continue;
		}
		noiseGenerators_[gen->name()] = gen;
		for (auto &source : gen->sources()) {
			queue.push(source);
		}
	}

	// update the combo box, and auto select `generator`
	ui_.textureSelectionBox->clear();
	for (auto &gen : noiseGenerators_) {
		ui_.textureSelectionBox->addItem(gen.first.c_str());
	}
	ui_.textureSelectionBox->setCurrentText(generator->name().c_str());
}

#define REGEN_QT_SLIDER_MAX 1000000
#define REGEN_QT_SLIDER_MAX_d 1000000.0

void NoiseWidget::addProperty(
			std::string_view name,
			double min,
			double max,
			double value,
			const std::function<void(double)> &setter) {
	auto *slider = new QSlider(Qt::Horizontal);
	int sliderMin = 0;
	int sliderMax = REGEN_QT_SLIDER_MAX;
	int sliderValue = (int) ((value - min) / (max - min) * (sliderMax - sliderMin));
	int itemRow = ui_.parameterTable->rowCount();
	slider->setRange(sliderMin, sliderMax);
	slider->setValue(sliderValue);
	ui_.parameterTable->setRowCount(itemRow + 1);
	ui_.parameterTable->setItem(itemRow, 0, new QTableWidgetItem(name.data()));
	ui_.parameterTable->setItem(itemRow, 1, new QTableWidgetItem(QString::number(value)));
	ui_.parameterTable->setCellWidget(itemRow, 2, slider);
	slider->connect(slider, &QSlider::valueChanged, [this,setter,min,max,itemRow](int value) {
		double val = (static_cast<double>(value) / REGEN_QT_SLIDER_MAX_d) * ((max - min) + min);
		setter(val);
		ui_.parameterTable->item(itemRow, 1)->setText(QString::number(val));
		//updateTexture();
	});
}

void NoiseWidget::addProperty_i(
			std::string_view name,
			int min,
			int max,
			int value,
			const std::function<void(int)> &setter) {
	auto *slider = new QSlider(Qt::Horizontal);
	int sliderMin = 0;
	int sliderMax = REGEN_QT_SLIDER_MAX;
	int sliderValue = static_cast<int>(
			(static_cast<double>(value) / static_cast<double>(max-min)) * static_cast<double>(sliderMax - sliderMin));
	int itemRow = ui_.parameterTable->rowCount();
	slider->setRange(sliderMin, sliderMax);
	slider->setValue(sliderValue);
	ui_.parameterTable->setRowCount(itemRow + 1);
	ui_.parameterTable->setItem(itemRow, 0, new QTableWidgetItem(name.data()));
	ui_.parameterTable->setItem(itemRow, 1, new QTableWidgetItem(QString::number(value)));
	ui_.parameterTable->setCellWidget(itemRow, 2, slider);
	slider->connect(slider, &QSlider::valueChanged, [this,setter,min,max,itemRow](int value) {
		auto val = static_cast<int>(
			((static_cast<double>(value) / REGEN_QT_SLIDER_MAX_d) *
			 static_cast<double>(max - min)) + min);
		setter(val);
		ui_.parameterTable->item(itemRow, 1)->setText(QString::number(val));
		//updateTexture();
	});
}

#define NOISE_SEED_MIN 0
#define NOISE_SEED_MAX 999999
#define NOISE_FREQUENCY_MIN 0.0
#define NOISE_FREQUENCY_MAX 20.0
#define NOISE_PERSISTENCE_MIN 0.0
#define NOISE_PERSISTENCE_MAX 1.0
#define NOISE_LACUNARITY_MIN 1.0
#define NOISE_LACUNARITY_MAX 5.0
#define NOISE_OCTAVES_MIN 1
#define NOISE_OCTAVES_MAX noise::module::BILLOW_MAX_OCTAVE

void NoiseWidget::updateTable(const ref_ptr<NoiseGenerator> &generator) {
	auto *handle = generator->handle().get();
	ui_.parameterTable->clear();
	ui_.parameterTable->setRowCount(0);
	ui_.parameterTable->setColumnCount(3);
	ui_.parameterTable->setHorizontalHeaderLabels(QStringList() << "Parameter" << "Value" << "");

	auto perlin = dynamic_cast<noise::module::Perlin *>(handle);
	if (perlin) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, perlin->GetSeed(),
			[perlin](int value) { perlin->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, perlin->GetFrequency(),
			[perlin](double value) { perlin->SetFrequency(value); });
		addProperty("Persistence", NOISE_PERSISTENCE_MIN, NOISE_PERSISTENCE_MAX, perlin->GetPersistence(),
			[perlin](double value) { perlin->SetPersistence(value); });
		addProperty("Lacunarity", NOISE_LACUNARITY_MIN, NOISE_LACUNARITY_MAX, perlin->GetLacunarity(),
			[perlin](double value) { perlin->SetLacunarity(value); });
		addProperty_i("Octave Count", NOISE_OCTAVES_MIN, NOISE_OCTAVES_MAX, perlin->GetOctaveCount(),
			[perlin](int value) { perlin->SetOctaveCount(value); });
		return;
	}

	auto billow = dynamic_cast<noise::module::Billow *>(handle);
	if (billow) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, billow->GetSeed(),
			[billow](int value) { billow->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, billow->GetFrequency(),
			[billow](double value) { billow->SetFrequency(value); });
		addProperty("Lacunarity", NOISE_LACUNARITY_MIN, NOISE_LACUNARITY_MAX, billow->GetLacunarity(),
			[billow](double value) { billow->SetLacunarity(value); });
		addProperty_i("Octave Count", NOISE_OCTAVES_MIN, NOISE_OCTAVES_MAX, billow->GetOctaveCount(),
			[billow](int value) { billow->SetOctaveCount(value); });
		return;
	}

	auto turbulence = dynamic_cast<noise::module::Turbulence *>(handle);
	if (turbulence) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, turbulence->GetSeed(),
			[turbulence](int value) { turbulence->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, turbulence->GetFrequency(),
			[turbulence](double value) { turbulence->SetFrequency(value); });
		addProperty("Power", 0.0, 10.0, turbulence->GetPower(),
			[turbulence](double value) { turbulence->SetPower(value); });
		addProperty_i("Roughness", 1, 10, turbulence->GetRoughnessCount(),
			[turbulence](int value) { turbulence->SetRoughness(value); });
		return;
	}

	auto voronoi = dynamic_cast<noise::module::Voronoi *>(handle);
	if (voronoi) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, voronoi->GetSeed(),
			[voronoi](int value) { voronoi->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, voronoi->GetFrequency(),
			[voronoi](double value) { voronoi->SetFrequency(value); });
		addProperty("Displacement", 0.0, 100.0, voronoi->GetDisplacement(),
			[voronoi](double value) { voronoi->SetDisplacement(value); });
		return;
	}

	auto cylinders = dynamic_cast<noise::module::Cylinders *>(handle);
	if (cylinders) {
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, cylinders->GetFrequency(),
			[cylinders](double value) { cylinders->SetFrequency(value); });
		return;
	}

	auto scaleBias = dynamic_cast<noise::module::ScaleBias *>(handle);
	if (scaleBias) {
		addProperty("Scale", -100.0, 100.0, scaleBias->GetScale(),
			[scaleBias](double value) { scaleBias->SetScale(value); });
		addProperty("Bias", -100.0, 100.0, scaleBias->GetBias(),
			[scaleBias](double value) { scaleBias->SetBias(value); });
		return;
	}

	auto scalePoint = dynamic_cast<noise::module::ScalePoint *>(handle);
	if (scalePoint) {
		addProperty("X Scale", 0.0, 100.0, scalePoint->GetXScale(),
			[scalePoint](double value) { scalePoint->SetXScale(value); });
		addProperty("Y Scale", 0.0, 100.0, scalePoint->GetYScale(),
			[scalePoint](double value) { scalePoint->SetYScale(value); });
		addProperty("Z Scale", 0.0, 100.0, scalePoint->GetZScale(),
			[scalePoint](double value) { scalePoint->SetZScale(value); });
		return;
	}

	auto riggedMultifractal = dynamic_cast<noise::module::RidgedMulti *>(handle);
	if (riggedMultifractal) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, riggedMultifractal->GetSeed(),
			[riggedMultifractal](int value) { riggedMultifractal->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, riggedMultifractal->GetFrequency(),
			[riggedMultifractal](double value) { riggedMultifractal->SetFrequency(value); });
		addProperty("Lacunarity", NOISE_LACUNARITY_MIN, NOISE_LACUNARITY_MAX, riggedMultifractal->GetLacunarity(),
			[riggedMultifractal](double value) { riggedMultifractal->SetLacunarity(value); });
		addProperty_i("Octave Count", NOISE_OCTAVES_MIN, NOISE_OCTAVES_MAX, riggedMultifractal->GetOctaveCount(),
			[riggedMultifractal](int value) { riggedMultifractal->SetOctaveCount(value); });
		return;
	}

	auto add = dynamic_cast<noise::module::Add *>(handle);
	if (add) {
		return;
	}

	auto multiply = dynamic_cast<noise::module::Multiply *>(handle);
	if (multiply) {
		return;
	}

	auto max = dynamic_cast<noise::module::Max *>(handle);
	if (max) {
		return;
	}

	auto min = dynamic_cast<noise::module::Min *>(handle);
	if (min) {
		return;
	}

	auto select = dynamic_cast<noise::module::Select *>(handle);
	if (select) {
		addProperty("Lower Bound", -1.0, 1.0, select->GetLowerBound(),
			[select](double value) { select->SetBounds(value, select->GetUpperBound()); });
		addProperty("Upper Bound", -1.0, 1.0, select->GetUpperBound(),
			[select](double value) { select->SetBounds(select->GetLowerBound(), value); });
		addProperty("Edge Falloff", 0.0, 1.0, select->GetEdgeFalloff(),
			[select](double value) { select->SetEdgeFalloff(value); });
		return;
	}

	auto translatePoint = dynamic_cast<noise::module::TranslatePoint *>(handle);
	if (translatePoint) {
		addProperty("X Translation", -100.0, 100.0, translatePoint->GetXTranslation(),
			[translatePoint](double value) { translatePoint->SetXTranslation(value); });
		addProperty("Y Translation", -100.0, 100.0, translatePoint->GetYTranslation(),
			[translatePoint](double value) { translatePoint->SetYTranslation(value); });
		addProperty("Z Translation", -100.0, 100.0, translatePoint->GetZTranslation(),
			[translatePoint](double value) { translatePoint->SetZTranslation(value); });
		return;
	}

	auto rotatePoint = dynamic_cast<noise::module::RotatePoint *>(handle);
	if (rotatePoint) {
		addProperty("X Angle", 0.0, 360.0, rotatePoint->GetXAngle(),
			[rotatePoint](double value) { rotatePoint->SetXAngle(value); });
		addProperty("Y Angle", 0.0, 360.0, rotatePoint->GetYAngle(),
			[rotatePoint](double value) { rotatePoint->SetYAngle(value); });
		addProperty("Z Angle", 0.0, 360.0, rotatePoint->GetZAngle(),
			[rotatePoint](double value) { rotatePoint->SetZAngle(value); });
		return;
	}
}

void NoiseWidget::updateList(const ref_ptr<NoiseGenerator> &generator) {
	ui_.inputList->clear();
	for (auto &source : generator->sources()) {
		ui_.inputList->addItem(source->name().c_str());
	}
}

void NoiseWidget::updateWidgets(const ref_ptr<NoiseGenerator> &generator) {
	updateTable(generator);
	updateList(generator);
}

void NoiseWidget::updateTextureSelection() {
	auto noiseModuleName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	auto generator = noiseGenerators_[noiseModuleName];
	if (!generator.get()) {
		REGEN_WARN("No noise generator found for '" << noiseModuleName << "'.");
		return;
	}
	updateWidgets(generator);
	//updateTexture();
}

void NoiseWidget::toggleFullscreen() {
	app_->toggleFullscreen();
}

void NoiseWidget::addInput() {
	auto selectedName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	auto selected = noiseGenerators_[selectedName];
	if (!selected.get()) {
		REGEN_WARN("No noise generator found for '" << selectedName << "'.");
		return;
	}
	// let the user select a noise generator to add from keys of noiseGenerators_
	QStringList items;
	for (auto &gen : noiseGenerators_) {
		if (gen.first == selectedName) { continue; }
		items.append(gen.first.c_str());
	}
	bool ok;
	QString item = QInputDialog::getItem(this, "Select a noise generator to add",
			"Available noise generators:", items, 0, false, &ok);
	if (!ok) {
		return;
	}
	auto toAddName = item.toStdString();
	auto toAdd = noiseGenerators_[toAddName];
	if (!toAdd.get()) {
		REGEN_WARN("No noise generator found for '" << toAddName << "'.");
		return;
	}
	selected->addSource(toAdd);
	updateWidgets(selected);
}

void NoiseWidget::removeInput() {
	auto item = ui_.inputList->currentItem();
	if (!item) {
		REGEN_WARN("No item selected.");
		return;
	}
	auto toRemoveName = item->text().toStdString();
	auto selectedName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	auto selected = noiseGenerators_[selectedName];
	if (!selected.get()) {
		REGEN_WARN("No noise generator found for '" << selectedName << "'.");
		return;
	}
	auto toRemove = noiseGenerators_[toRemoveName];
	if (!toRemove.get()) {
		REGEN_WARN("No noise generator found for '" << toRemoveName << "'.");
		return;
	}
	selected->removeSource(toRemove);
	updateWidgets(selected);
}

void NoiseWidget::removeNoiseModule() {
	auto selectedName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	auto selected = noiseGenerators_[selectedName];
	if (!selected.get()) {
		REGEN_WARN("No noise generator found for '" << selectedName << "'.");
		return;
	}
	noiseGenerators_.erase(selectedName);
	for (auto &gen : noiseGenerators_) {
		gen.second->removeSource(selected);
	}
	ui_.textureSelectionBox->removeItem(ui_.textureSelectionBox->currentIndex());
	updateWidgets(selected);
}

void NoiseWidget::addNoiseModule() {
	auto *dialog = new QDialog(this);
	auto *layout = new QVBoxLayout(dialog);
	auto *nameEdit = new QLineEdit(dialog);
	auto *typeBox = new QComboBox(dialog);
	typeBox->addItem("Perlin");
	typeBox->addItem("Billow");
	typeBox->addItem("Turbulence");
	typeBox->addItem("Voronoi");
	typeBox->addItem("Cylinders");
	typeBox->addItem("ScaleBias");
	typeBox->addItem("ScalePoint");
	typeBox->addItem("RidgedMulti");
	typeBox->addItem("Add");
	typeBox->addItem("Multiply");
	typeBox->addItem("Max");
	typeBox->addItem("Min");
	typeBox->addItem("Select");
	typeBox->addItem("TranslatePoint");
	typeBox->addItem("RotatePoint");
	auto *okButton = new QPushButton("OK", dialog);
	auto *cancelButton = new QPushButton("Cancel", dialog);
	layout->addWidget(nameEdit);
	layout->addWidget(typeBox);
	layout->addWidget(okButton);
	layout->addWidget(cancelButton);
	dialog->setLayout(layout);
	dialog->show();
	okButton->connect(okButton, &QPushButton::clicked, [this,dialog,nameEdit,typeBox]() {
		auto name = nameEdit->text().toStdString();
		auto type = typeBox->currentText().toStdString();
		ref_ptr<NoiseGenerator> newGen;
		if (type == "Perlin") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Perlin>::alloc());
		} else if (type == "Billow") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Billow>::alloc());
		} else if (type == "Turbulence") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Turbulence>::alloc());
		} else if (type == "Voronoi") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Voronoi>::alloc());
		} else if (type == "Cylinders") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Cylinders>::alloc());
		} else if (type == "ScaleBias") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::ScaleBias>::alloc());
		} else if (type == "ScalePoint") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::ScalePoint>::alloc());
		} else if (type == "Add") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Add>::alloc());
		} else if (type == "Multiply") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Multiply>::alloc());
		} else if (type == "Max") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Max>::alloc());
		} else if (type == "Min") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Min>::alloc());
		} else if (type == "RidgedMulti") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::RidgedMulti>::alloc());
		} else if (type == "Select") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::Select>::alloc());
		} else if (type == "TranslatePoint") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::TranslatePoint>::alloc());
		} else if (type == "RotatePoint") {
			newGen = ref_ptr<NoiseGenerator>::alloc(name, ref_ptr<noise::module::RotatePoint>::alloc());
		} else {
			REGEN_WARN("Unknown noise module type '" << type << "'.");
			return;
		}
		noiseGenerators_[name] = newGen;
		ui_.textureSelectionBox->addItem(name.c_str());
		dialog->close();
	});
}

void NoiseWidget::loadPerlin() {
	int randomSeed = 36433;
	auto gen = NoiseGenerator::preset_perlin(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadClouds() {
	int randomSeed = 754643;
	auto gen = NoiseGenerator::preset_clouds(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadGranite() {
			int randomSeed = 45245;
	auto gen = NoiseGenerator::preset_granite(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadWood() {
	int randomSeed = 9674;
	auto gen = NoiseGenerator::preset_wood(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}


void NoiseWidget::saveImage() {
	// select a file name
	auto fileName = QFileDialog::getSaveFileName(
			this,
			"Save Image",
			"",
			"PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp)");
	if (fileName.isEmpty()) {
		return;
	}
	// grab the texture data
	texture_->updateTextureData();
	DevilLoader::save(texture_->textureData(), fileName.toStdString());
}

//////////////////////////////
//////// Qt Events
//////////////////////////////

void NoiseWidget::resizeEvent(QResizeEvent *event) {
	updateSize();
}

void NoiseWidget::keyPressEvent(QKeyEvent *event) {
	event->accept();
}

void NoiseWidget::keyReleaseEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_F) {
		toggleFullscreen();
	}
	event->accept();
}

void NoiseWidget::mousePressEvent(QMouseEvent *event) {
	event->accept();
}

void NoiseWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	toggleFullscreen();
	event->accept();
}

void NoiseWidget::mouseReleaseEvent(QMouseEvent *event) {
	event->accept();
}

