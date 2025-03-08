#include <iostream>

#include <QtGui/QMouseEvent>
#include <QtCore/QList>
#include <QtCore/QMimeData>

#include "regen/states/blit-state.h"
#include "regen/states/state-configurer.h"
#include "regen/states/fbo-state.h"
#include "regen/meshes/primitives/rectangle.h"
#include <regen/animations/animation-manager.h>
#include <QInputDialog>

#include "noise-widget.h"

using namespace regen;
using namespace std;

////////////
////////////

class InitAnimation : public Animation {
public:
	explicit InitAnimation(NoiseWidget *widget)
			: Animation(GL_TRUE, GL_FALSE), widget_(widget) {}

	void glAnimate(RenderState *rs, GLdouble dt) override { widget_->gl_loadScene(); }

	NoiseWidget *widget_;
};

NoiseWidget::NoiseWidget(QtApplication *app)
		: QMainWindow(),
		  Animation(GL_TRUE, GL_TRUE),
		  app_(app) {
	setMouseTracking(true);

	ui_.setupUi(this);
	app_->glWidget()->setEnabled(false);
	app_->glWidget()->setFocusPolicy(Qt::NoFocus);
	ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0, 0, 1, 1);

	// initially table size
	QList<int> initialSizes;
	initialSizes.append(500);
	initialSizes.append(300);
	ui_.splitter->setSizes(initialSizes);

	updateTexture_ = GL_TRUE;
	updateSize();
	initAnim_ = ref_ptr<InitAnimation>::alloc(this);
}

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler {
public:
	FBOResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
			: EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) {}

	void call(EventObject *evObject, EventData *) {
		auto *app = (Application *) evObject;
		auto winSize = app->windowViewport()->getVertex(0).r;
		fboState_->resize(winSize.x * wScale_, winSize.y * hScale_);
	}

protected:
	ref_ptr<FBOState> fboState_;
	GLfloat wScale_, hScale_;
};

void setBlitToScreen(
		Application *app,
		const ref_ptr<FBO> &fbo,
		GLenum attachment) {
	ref_ptr<State> blitState = ref_ptr<BlitToScreen>::alloc(fbo, app->windowViewport(), attachment);
	app->renderTree()->addChild(ref_ptr<StateNode>::alloc(blitState));
}

ref_ptr<Mesh> createTextureWidget(
		Application *app,
		const ref_ptr<Texture> &texture,
		const ref_ptr<StateNode> &root) {
	Rectangle::Config quadConfig;
	quadConfig.levelOfDetails = {0};
	quadConfig.isTexcoRequired = GL_TRUE;
	quadConfig.isNormalRequired = GL_FALSE;
	quadConfig.isTangentRequired = GL_FALSE;
	quadConfig.centerAtOrigin = GL_TRUE;
	quadConfig.rotation = Vec3f(0.5 * M_PI, 0.0 * M_PI, 0.0 * M_PI);
	quadConfig.posScale = Vec3f(1.0f);
	quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
	quadConfig.levelOfDetails = {0};
	quadConfig.isTexcoRequired = GL_TRUE;
	quadConfig.isNormalRequired = GL_FALSE;
	quadConfig.centerAtOrigin = GL_TRUE;
	ref_ptr<Mesh> mesh = ref_ptr<regen::Rectangle>::alloc(quadConfig);

	auto texState = ref_ptr<TextureState>::alloc(texture);
	texState->set_mapTo(TextureState::MAP_TO_COLOR);
	mesh->joinStates(texState);

	ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
	mesh->joinStates(shaderState);

	ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
	root->addChild(meshNode);

	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(meshNode.get());
	shaderConfigurer.define("USE_NORMALIZED_COORDINATES", "TRUE");
	shaderState->createShader(shaderConfigurer.cfg(), "regen.gui.widget");
	mesh->updateVAO(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

	return mesh;
}

void NoiseWidget::gl_loadScene() {
	AnimationManager::get().pause(GL_TRUE);

	// create render target
	auto winSize = app_->windowViewport()->getVertex(0).r;
	ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(winSize.x, winSize.y);
	ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
	fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
	// resize fbo with window
	auto resizer = ref_ptr<FBOResizer>::alloc(fboState, 1.0, 1.0);
	app_->connect(Application::RESIZE_EVENT, resizer);

	// create a root node (that binds the render target)
	ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::alloc(fboState);
	app_->renderTree()->addChild(sceneRoot);

	// add the video widget to the root node
	texture_ = ref_ptr<NoiseTexture2D>::alloc(256, 256);
	createTextureWidget(app_, texture_, sceneRoot);
	setBlitToScreen(app_, fbo, GL_COLOR_ATTACHMENT0);
	GL_ERROR_LOG();

	updateSize();
	initAnim_ = ref_ptr<Animation>();
	AnimationManager::get().resume();
}

//////////////////////////////
//////// Slots
//////////////////////////////

void NoiseWidget::updateSize() {
	GLfloat widgetRatio = ui_.blackBackground->width() / (GLfloat) ui_.blackBackground->height();
	GLfloat texRatio = 1.0;
	GLint w, h;
	if (widgetRatio > texRatio) {
		w = (GLint) (ui_.blackBackground->height() * texRatio);
		h = ui_.blackBackground->height();
	} else {
		w = ui_.blackBackground->width();
		h = (GLint) (ui_.blackBackground->width() / texRatio);
	}
	if (w % 2 != 0) { w -= 1; }
	if (h % 2 != 0) { h -= 1; }
	ui_.glWidget->setMinimumSize(QSize(max(2, w), max(2, h)));
}

void NoiseWidget::updateTexture() {
	updateTexture_ = GL_TRUE;
}

void NoiseWidget::animate(GLdouble dt) {}

void NoiseWidget::glAnimate(RenderState *rs, GLdouble dt) {
	if (!updateTexture_ || !texture_.get()) return;

	auto noiseModuleName = ui_.textureSelectionBox->itemText(
			ui_.textureSelectionBox->currentIndex()).toStdString();
	if (noiseModuleName.empty()) { return; }
	auto generator = noiseGenerators_[noiseModuleName];
	if (!generator.get()) { return; }

	lock();
	texture_->setNoiseGenerator(generator);
	unlock();
	updateTexture_ = GL_FALSE;
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
			GLdouble min,
			GLdouble max,
			GLdouble value,
			const std::function<void(GLdouble)> &setter) {
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
		GLdouble val = (static_cast<GLdouble>(value) / REGEN_QT_SLIDER_MAX_d) * ((max - min) + min);
		setter(val);
		ui_.parameterTable->item(itemRow, 1)->setText(QString::number(val));
		updateTexture();
	});
}

void NoiseWidget::addProperty_i(
			std::string_view name,
			GLint min,
			GLint max,
			GLint value,
			const std::function<void(GLint)> &setter) {
	auto *slider = new QSlider(Qt::Horizontal);
	int sliderMin = 0;
	int sliderMax = REGEN_QT_SLIDER_MAX;
	int sliderValue = static_cast<int>(
			(static_cast<GLdouble>(value) / static_cast<GLdouble>(max-min)) * static_cast<GLdouble>(sliderMax - sliderMin));
	int itemRow = ui_.parameterTable->rowCount();
	slider->setRange(sliderMin, sliderMax);
	slider->setValue(sliderValue);
	ui_.parameterTable->setRowCount(itemRow + 1);
	ui_.parameterTable->setItem(itemRow, 0, new QTableWidgetItem(name.data()));
	ui_.parameterTable->setItem(itemRow, 1, new QTableWidgetItem(QString::number(value)));
	ui_.parameterTable->setCellWidget(itemRow, 2, slider);
	slider->connect(slider, &QSlider::valueChanged, [this,setter,min,max,itemRow](int value) {
		auto val = static_cast<GLint>(
			((static_cast<GLdouble>(value) / REGEN_QT_SLIDER_MAX_d) *
			 static_cast<GLdouble>(max - min)) + min);
		setter(val);
		ui_.parameterTable->item(itemRow, 1)->setText(QString::number(val));
		updateTexture();
	});
}

#define NOISE_SEED_MIN 0
#define NOISE_SEED_MAX 999999
#define NOISE_FREQUENCY_MIN 0.0
#define NOISE_FREQUENCY_MAX 100.0
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
			[perlin](GLint value) { perlin->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, perlin->GetFrequency(),
			[perlin](GLdouble value) { perlin->SetFrequency(value); });
		addProperty("Persistence", NOISE_PERSISTENCE_MIN, NOISE_PERSISTENCE_MAX, perlin->GetPersistence(),
			[perlin](GLdouble value) { perlin->SetPersistence(value); });
		addProperty("Lacunarity", NOISE_LACUNARITY_MIN, NOISE_LACUNARITY_MAX, perlin->GetLacunarity(),
			[perlin](GLdouble value) { perlin->SetLacunarity(value); });
		addProperty_i("Octave Count", NOISE_OCTAVES_MIN, NOISE_OCTAVES_MAX, perlin->GetOctaveCount(),
			[perlin](GLint value) { perlin->SetOctaveCount(value); });
		return;
	}

	auto billow = dynamic_cast<noise::module::Billow *>(handle);
	if (billow) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, billow->GetSeed(),
			[billow](GLint value) { billow->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, billow->GetFrequency(),
			[billow](GLdouble value) { billow->SetFrequency(value); });
		addProperty("Lacunarity", NOISE_LACUNARITY_MIN, NOISE_LACUNARITY_MAX, billow->GetLacunarity(),
			[billow](GLdouble value) { billow->SetLacunarity(value); });
		addProperty_i("Octave Count", NOISE_OCTAVES_MIN, NOISE_OCTAVES_MAX, billow->GetOctaveCount(),
			[billow](GLint value) { billow->SetOctaveCount(value); });
		return;
	}

	auto turbulence = dynamic_cast<noise::module::Turbulence *>(handle);
	if (turbulence) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, turbulence->GetSeed(),
			[turbulence](GLint value) { turbulence->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, turbulence->GetFrequency(),
			[turbulence](GLdouble value) { turbulence->SetFrequency(value); });
		addProperty("Power", 0.0, 10.0, turbulence->GetPower(),
			[turbulence](GLdouble value) { turbulence->SetPower(value); });
		addProperty_i("Roughness", 1, 10, turbulence->GetRoughnessCount(),
			[turbulence](GLint value) { turbulence->SetRoughness(value); });
		return;
	}

	auto voronoi = dynamic_cast<noise::module::Voronoi *>(handle);
	if (voronoi) {
		addProperty_i("Seed", NOISE_SEED_MIN, NOISE_SEED_MAX, voronoi->GetSeed(),
			[voronoi](GLint value) { voronoi->SetSeed(value); });
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, voronoi->GetFrequency(),
			[voronoi](GLdouble value) { voronoi->SetFrequency(value); });
		addProperty("Displacement", 0.0, 100.0, voronoi->GetDisplacement(),
			[voronoi](GLdouble value) { voronoi->SetDisplacement(value); });
		return;
	}

	auto cylinders = dynamic_cast<noise::module::Cylinders *>(handle);
	if (cylinders) {
		addProperty("Frequency", NOISE_FREQUENCY_MIN, NOISE_FREQUENCY_MAX, cylinders->GetFrequency(),
			[cylinders](GLdouble value) { cylinders->SetFrequency(value); });
		return;
	}

	auto scaleBias = dynamic_cast<noise::module::ScaleBias *>(handle);
	if (scaleBias) {
		addProperty("Scale", -100.0, 100.0, scaleBias->GetScale(),
			[scaleBias](GLdouble value) { scaleBias->SetScale(value); });
		addProperty("Bias", -100.0, 100.0, scaleBias->GetBias(),
			[scaleBias](GLdouble value) { scaleBias->SetBias(value); });
		return;
	}

	auto scalePoint = dynamic_cast<noise::module::ScalePoint *>(handle);
	if (scalePoint) {
		addProperty("X Scale", 0.0, 100.0, scalePoint->GetXScale(),
			[scalePoint](GLdouble value) { scalePoint->SetXScale(value); });
		addProperty("Y Scale", 0.0, 100.0, scalePoint->GetYScale(),
			[scalePoint](GLdouble value) { scalePoint->SetYScale(value); });
		addProperty("Z Scale", 0.0, 100.0, scalePoint->GetZScale(),
			[scalePoint](GLdouble value) { scalePoint->SetZScale(value); });
		return;
	}

	auto add = dynamic_cast<noise::module::Add *>(handle);
	if (add) {
		return;
	}

	auto translatePoint = dynamic_cast<noise::module::TranslatePoint *>(handle);
	if (translatePoint) {
		addProperty("X Translation", -100.0, 100.0, translatePoint->GetXTranslation(),
			[translatePoint](GLdouble value) { translatePoint->SetXTranslation(value); });
		addProperty("Y Translation", -100.0, 100.0, translatePoint->GetYTranslation(),
			[translatePoint](GLdouble value) { translatePoint->SetYTranslation(value); });
		addProperty("Z Translation", -100.0, 100.0, translatePoint->GetZTranslation(),
			[translatePoint](GLdouble value) { translatePoint->SetZTranslation(value); });
		return;
	}

	auto rotatePoint = dynamic_cast<noise::module::RotatePoint *>(handle);
	if (rotatePoint) {
		addProperty("X Angle", 0.0, 360.0, rotatePoint->GetXAngle(),
			[rotatePoint](GLdouble value) { rotatePoint->SetXAngle(value); });
		addProperty("Y Angle", 0.0, 360.0, rotatePoint->GetYAngle(),
			[rotatePoint](GLdouble value) { rotatePoint->SetYAngle(value); });
		addProperty("Z Angle", 0.0, 360.0, rotatePoint->GetZAngle(),
			[rotatePoint](GLdouble value) { rotatePoint->SetZAngle(value); });
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
	updateTexture();
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
	typeBox->addItem("Add");
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
	GLint randomSeed = 36433;
	auto gen = NoiseGenerator::preset_perlin(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadClouds() {
	GLint randomSeed = 754643;
	auto gen = NoiseGenerator::preset_clouds(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadGranite() {
			GLint randomSeed = 45245;
	auto gen = NoiseGenerator::preset_granite(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
}

void NoiseWidget::loadWood() {
	GLint randomSeed = 9674;
	auto gen = NoiseGenerator::preset_wood(randomSeed);
	updateTexture();
	updateNoiseGenerators(gen);
	updateWidgets(gen);
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

