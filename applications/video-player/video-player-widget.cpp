#include <iostream>

#include "video-player-widget.h"
#include "regen/textures/fbo.h"
#include <regen/textures/texture-state.h>
#include "regen/shader/shader-state.h"
#include "regen/textures/fbo-state.h"
#include <regen/gl/states/blit-state.h>
#include <regen/objects/primitives/rectangle.h>
#include <regen/scene/state-configurer.h>

#include <QtWidgets/QFileDialog>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QFont>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtCore/QDirIterator>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

#if LIBAVFORMAT_VERSION_MAJOR > 53
#define __CLOSE_INPUT__(x) avformat_close_input(&x)
#else
#define __CLOSE_INPUT__(x) av_close_input_file(x)
#endif

#include <regen/animation/animation-manager.h>

using namespace std;

static QString formatTime(float elapsedSeconds) {
	uint32_t seconds = (uint32_t) elapsedSeconds;
	uint32_t minutes = seconds / 60;
	seconds = seconds % 60;
	string label = REGEN_STRING(
			(minutes < 10 ? "0" : "") << minutes <<
									  ":" <<
									  (seconds < 10 ? "0" : "") << seconds);
	return QString(label.c_str());
}

static void hideLayout(QLayout *layout) {
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem *item = layout->itemAt(i);
		if (item->widget()) { item->widget()->hide(); }
		if (item->layout()) { hideLayout(item->layout()); }
	}
}

static void showLayout(QLayout *layout) {
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem *item = layout->itemAt(i);
		if (item->widget()) { item->widget()->show(); }
		if (item->layout()) { showLayout(item->layout()); }
	}
}

static bool isRegularFile(const string &f) {
	boost::filesystem::path p(f);
	return boost::filesystem::is_regular_file(p);
}

static bool isDirectory(const string &f) {
	boost::filesystem::path p(f);
	return boost::filesystem::is_directory(p);
}

////////////
////////////

class VideoInitAnimation : public Animation {
public:
	explicit VideoInitAnimation(VideoPlayerWidget *widget)
			: Animation(true, false), widget_(widget) {}

	void gpuUpdate(RenderState *rs, double dt) override { widget_->gl_loadScene(); }

	VideoPlayerWidget *widget_;
};

VideoPlayerWidget::VideoPlayerWidget(QtApplication *app)
		: QMainWindow(),
		  app_(app),
		  gain_(1.0f),
		  elapsedTimer_(this),
		  activePlaylistRow_(nullptr),
		  wereControlsShown_(false) {
	setMouseTracking(true);
	setAcceptDrops(true);
	controlsShown_ = true;

	ui_.setupUi(this);
	app_->glWidget()->setEnabled(false);
	app_->glWidget()->setFocusPolicy(Qt::NoFocus);
	ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0, 0, 1, 1);
	ui_.repeatButton->click();

	fullscreenLayout_ = new QVBoxLayout();
	fullscreenLayout_->setObjectName(QString::fromUtf8("fullscreenLayout"));
	ui_.gridLayout_2->addLayout(fullscreenLayout_, 0, 0, 1, 1);
	hideLayout(fullscreenLayout_);

	// initially playlist is hidden
	QList<int> initialSizes;
	initialSizes.append(1);
	initialSizes.append(0);
	ui_.splitter->setSizes(initialSizes);

	// update elapsed time label
	elapsedTimer_.setInterval(1000);
	connect(&elapsedTimer_, SIGNAL(timeout()), this, SLOT(updateElapsedTime()));
	elapsedTimer_.start();

	srand(time(nullptr));

	initAnim_ = ref_ptr<VideoInitAnimation>::alloc(this);
	initAnim_->startAnimation();
}

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler {
public:
	FBOResizer(const ref_ptr<FBOState> &fbo, float wScale, float hScale)
			: EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) {}

	~FBOResizer() override = default;

	void call(EventObject *evObject, EventData *) override {
		auto *app = (Scene *) evObject;
		auto winSize = app->screen()->viewport();
		fboState_->resize(winSize.r.x * wScale_, winSize.r.y * hScale_);
	}

protected:
	ref_ptr<FBOState> fboState_;
	float wScale_, hScale_;
};

void setBlitToScreen(Scene *app, const ref_ptr<FBO> &fbo, GLenum attachment) {
	ref_ptr<State> blitState = ref_ptr<BlitToScreen>::alloc(fbo, app->screen(), attachment);
	app->renderTree()->addChild(ref_ptr<StateNode>::alloc(blitState));
}

ref_ptr<Mesh> createVideoWidget(
		Scene *app,
		const ref_ptr<Texture> &videoTexture,
		const ref_ptr<StateNode> &root) {
	Rectangle::Config quadConfig;
	quadConfig.levelOfDetails = {0};
	quadConfig.isTexcoRequired = true;
	quadConfig.isNormalRequired = false;
	quadConfig.isTangentRequired = false;
	quadConfig.centerAtOrigin = true;
	quadConfig.rotation = Vec3f(0.5 * M_PI, 0.0 * M_PI, 0.0 * M_PI);
	quadConfig.posScale = Vec3f::one();
	quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
	quadConfig.levelOfDetails = {0};
	quadConfig.isTexcoRequired = true;
	quadConfig.isNormalRequired = false;
	quadConfig.centerAtOrigin = true;
	auto mesh = regen::Rectangle::create(quadConfig);

	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(videoTexture);
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
	mesh->updateVAO(shaderConfigurer.cfg(), shaderState->shader());

	return mesh;
}

void VideoPlayerWidget::gl_loadScene() {
	AnimationManager::get().pause(true);

	vid_ = ref_ptr<VideoTexture>::alloc();
	demuxer_ = vid_->demuxer();

	// create render target
	auto winSize = app_->screen()->viewport();
	ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(winSize.r.x, winSize.r.y);
	ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE);
	ref_ptr<FBOState> fboState = ref_ptr<FBOState>::alloc(fbo);
	fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
	// resize fbo with window
	app_->connect(Scene::RESIZE_EVENT, ref_ptr<FBOResizer>::alloc(fboState, 1.0, 1.0));

	// create a root node (that binds the render target)
	ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::alloc(fboState);
	app_->renderTree()->addChild(sceneRoot);

	// add the video widget to the root node
	createVideoWidget(app_, video(), sceneRoot);
	setBlitToScreen(app_, fbo, GL_COLOR_ATTACHMENT0);
	GL_ERROR_LOG();

	vid_->startAnimation();
	initAnim_ = ref_ptr<Animation>();
	AnimationManager::get().resume();
	REGEN_INFO("Video Scene Loaded.");
}

const ref_ptr<VideoTexture> &VideoPlayerWidget::video() const {
	return vid_;
}

void VideoPlayerWidget::activatePlaylistRow(int row) {
	QFont font;
	int activeRow = (activePlaylistRow_ != NULL ? activePlaylistRow_->row() : -1);
	if (activeRow != -1) {
		font.setBold(false);
		ui_.playlistTable->item(activeRow, 0)->setFont(font);
		ui_.playlistTable->item(activeRow, 1)->setFont(font);
	}
	font.setBold(true);
	if (row != -1) {
		activePlaylistRow_ = ui_.playlistTable->item(row, 0);
		ui_.playlistTable->item(row, 0)->setFont(font);
		ui_.playlistTable->item(row, 1)->setFont(font);
	} else {
		activePlaylistRow_ = NULL;
	}
}

void VideoPlayerWidget::changeVolume(int val) {
	gain_ = val / (float) ui_.volumeSlider->maximum();
	if (vid_->audioSource().get()) {
		vid_->audioSource()->set1f(AL_GAIN, gain_);
	}
	string label = REGEN_STRING(val << "%");
	ui_.volumeLabel->setText(QString(label.c_str()));
}

void VideoPlayerWidget::updateElapsedTime() {
	float elapsed = vid_->elapsedSeconds();
	ui_.progressLabel->setText(formatTime(elapsed));
	ui_.progressSlider->blockSignals(true);
	ui_.progressSlider->setValue((int) (
			ui_.progressSlider->maximum() * elapsed / demuxer_->totalSeconds()));
	ui_.progressSlider->blockSignals(false);

	if (!demuxer_->isPlaying() && demuxer_->hasInput() &&
		demuxer_->totalSeconds() < vid_->elapsedSeconds() + 1.0) {
		nextVideo();
		vid_->play();
	}
}

void VideoPlayerWidget::setVideoFile(const string &filePath) {
	boost::filesystem::path bdir(filePath.c_str());

	string file_ = bdir.filename().string();
	app_->toplevelWidget()->setWindowTitle(file_.c_str());

	vid_->set_file(filePath);
	vid_->play();
	if (vid_->audioSource().get()) {
		vid_->audioSource()->set1f(AL_GAIN, gain_);
	}
	ui_.playButton->setIcon(QIcon::fromTheme("media-playback-pause"));
	ui_.movieLengthLabel->setText(formatTime(demuxer_->totalSeconds()));
	updateElapsedTime();
	updateSize();
}

int VideoPlayerWidget::addPlaylistItem(const string &filePath) {
	// load information about the video file.
	// if libav cannot open the file skip it
	AVFormatContext *formatCtx = nullptr;
	if (avformat_open_input(&formatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
		return -1;
	}
	if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
		return -1;
	}
	double numSeconds = formatCtx->duration / (double) AV_TIME_BASE;
	__CLOSE_INPUT__(formatCtx);

	std::string filename = boost::filesystem::path(filePath).stem().string();
	int row = ui_.playlistTable->rowCount();
	ui_.playlistTable->insertRow(row);

	auto *fileNameItem = new QTableWidgetItem;
	fileNameItem->setText(filename.c_str());
	fileNameItem->setTextAlignment(Qt::AlignLeft);
	fileNameItem->setData(1, QVariant(filePath.c_str()));
	ui_.playlistTable->setItem(row, 0, fileNameItem);

	auto *lengthItem = new QTableWidgetItem;
	lengthItem->setText(formatTime(numSeconds));
	lengthItem->setTextAlignment(Qt::AlignLeft);
	lengthItem->setData(1, QVariant(filePath.c_str()));
	ui_.playlistTable->setItem(row, 1, lengthItem);

	return row;
}

void VideoPlayerWidget::addLocalPath(const string &filePath) {
	if (isRegularFile(filePath)) {
		addPlaylistItem(filePath);
	} else if (isDirectory(filePath)) {
		QDirIterator it(filePath.c_str(), QDirIterator::Subdirectories);
		QStringList files;
		while (it.hasNext()) {
			string childPath = it.next().toStdString();
			if (isRegularFile(childPath)) {
				files.append(childPath.c_str());
			}
		}
		files.sort();
		for (auto & file : files) {
			addPlaylistItem(file.toStdString());
		}
	}
}

//////////////////////////////
//////// Slots
//////////////////////////////

void VideoPlayerWidget::updateSize() {
	if (!vid_.get()) return;
	float widgetRatio = ui_.blackBackground->width() / (float) ui_.blackBackground->height();
	float videoRatio = vid_->width() / (float) vid_->height();
	int w, h;
	if (widgetRatio > videoRatio) {
		w = (int) (ui_.blackBackground->height() * videoRatio);
		h = ui_.blackBackground->height();
	} else {
		w = ui_.blackBackground->width();
		h = (int) (ui_.blackBackground->width() / videoRatio);
	}
	if (w % 2 != 0) { w -= 1; }
	if (h % 2 != 0) { h -= 1; }
	ui_.glWidget->setMinimumSize(QSize(max(2, w), max(2, h)));
}

void VideoPlayerWidget::toggleRepeat(bool v) {
	// nothing to do here for now...
}

void VideoPlayerWidget::toggleShuffle(bool v) {
	// nothing to do here for now...
}

void VideoPlayerWidget::toggleControls() {
	controlsShown_ = !controlsShown_;
	if (controlsShown_) {
		ui_.menubar->show();
		ui_.statusbar->show();

		hideLayout(fullscreenLayout_);
		ui_.splitter->addWidget(ui_.blackBackground);
		ui_.splitter->addWidget(ui_.playlistTable);
		showLayout(ui_.mainLayout);
		ui_.blackBackground->show();
		ui_.splitter->setSizes(splitterSizes_);
	} else {
		splitterSizes_ = ui_.splitter->sizes();

		ui_.menubar->hide();
		ui_.statusbar->hide();

		hideLayout(ui_.mainLayout);
		fullscreenLayout_->addWidget(ui_.blackBackground);
		showLayout(fullscreenLayout_);
		ui_.blackBackground->show();
	}
}

void VideoPlayerWidget::toggleFullscreen() {
	app_->toggleFullscreen();
	// hide controls when the window is fullscreen
	if (isFullScreen()) {
		wereControlsShown_ = controlsShown_;
		if (controlsShown_) toggleControls();
	} else {
		if (wereControlsShown_) toggleControls();
	}
}

void VideoPlayerWidget::openVideoFile() {
	QWidget *parent = nullptr;
	QFileDialog dialog(parent);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilters({"Videos (*.avi *.mpg)", "All files (*.*)"});
	dialog.setViewMode(QFileDialog::Detail);
	if (!dialog.exec()) { return; }

	QStringList fileNames = dialog.selectedFiles();
	string filePath = fileNames.first().toStdString();

	int row = addPlaylistItem(filePath);
	if (row == -1) {
		REGEN_WARN("Failed to open video file at " << filePath);
	} else {
		setVideoFile(filePath);
		activatePlaylistRow(row);
	}
}

void VideoPlayerWidget::playlistActivated(QTableWidgetItem *item) {
	setVideoFile(item->data(1).toString().toStdString());
	activatePlaylistRow(item->row());
}

void VideoPlayerWidget::nextVideo() {
	int row = ui_.playlistTable->rowCount();
	if (row == 0) return;
	int activeRow = (activePlaylistRow_ != nullptr ? activePlaylistRow_->row() : -1);

	if (ui_.shuffleButton->isChecked()) {
		row = rand() % ui_.playlistTable->rowCount();
	} else {
		row = activeRow + 1;
		if (row >= ui_.playlistTable->rowCount()) {
			row = (ui_.repeatButton->isChecked() ? 0 : -1);
		}
	}

	// update video file
	if (row == -1) {
		vid_->stop();
	} else {
		QTableWidgetItem *item = ui_.playlistTable->item(row, 0);
		setVideoFile(item->data(1).toString().toStdString());
	}
	// make playlist item bold
	activatePlaylistRow(row);
}

void VideoPlayerWidget::previousVideo() {
	int row;
	int activeRow = (activePlaylistRow_ != nullptr ? activePlaylistRow_->row() : -1);

	if (ui_.shuffleButton->isChecked()) {
		row = rand() % ui_.playlistTable->rowCount();
	} else {
		row = activeRow - 1;
		if (row < 0) {
			row = (ui_.repeatButton->isChecked() ? ui_.playlistTable->rowCount() - 1 : -1);
		}
	}

	// update video file
	if (row == -1) {
		vid_->stop();
	} else {
		QTableWidgetItem *item = ui_.playlistTable->item(row, 0);
		setVideoFile(item->data(1).toString().toStdString());
	}
	// make playlist item bold
	activatePlaylistRow(row);
}

void VideoPlayerWidget::seekVideo(int val) {
	vid_->seekTo(val / (float) ui_.progressSlider->maximum());
}

void VideoPlayerWidget::stopVideo() {
	if (demuxer_->hasInput()) {
		vid_->stop();
		ui_.playButton->setIcon(QIcon::fromTheme("media-playback-start"));
	}
}

void VideoPlayerWidget::togglePlayVideo() {
	if (demuxer_->hasInput()) {
		vid_->togglePlay();
		ui_.playButton->setIcon(QIcon::fromTheme(demuxer_->isPlaying() ?
												 "media-playback-pause" : "media-playback-start"));
	} else {
		nextVideo();
	}
}

//////////////////////////////
//////// Qt Events
//////////////////////////////

void VideoPlayerWidget::resizeEvent(QResizeEvent *event) {
	updateSize();
}

void VideoPlayerWidget::keyPressEvent(QKeyEvent *event) {
	event->accept();
}

void VideoPlayerWidget::keyReleaseEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_F) {
		toggleFullscreen();
	} else if (event->key() == Qt::Key_N) {
		nextVideo();
	} else if (event->key() == Qt::Key_P) {
		previousVideo();
	} else if (event->key() == Qt::Key_Space) {
		togglePlayVideo();
	} else if (event->key() == Qt::Key_O) {
		openVideoFile();
	}
	event->accept();
}

void VideoPlayerWidget::mousePressEvent(QMouseEvent *event) {
	event->accept();
}

void VideoPlayerWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	toggleFullscreen();
	event->accept();
}

void VideoPlayerWidget::mouseReleaseEvent(QMouseEvent *event) {
	event->accept();
}

void VideoPlayerWidget::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasFormat("text/uri-list")) {
		event->acceptProposedAction();
	}
}

void VideoPlayerWidget::dropEvent(QDropEvent *event) {
	QList<QUrl> uris = event->mimeData()->urls();
	for (auto & uri : uris) {
		addLocalPath(uri.toLocalFile().toStdString());
	}
}
