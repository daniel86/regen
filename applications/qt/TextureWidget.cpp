#include "TextureWidget.h"
#include "RegenWidgetGL.h"
#include "regen/textures/fbo.h"
#include "regen/textures/fbo-state.h"
#include "regen/textures/texture-state.h"
#include "regen/shader/shader-state.h"
#include "regen/scene/state-configurer.h"
#include "regen/objects/primitives/rectangle.h"
#include "regen/textures/texture-loader.h"
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>

using namespace regen;

namespace regen {
	class TextureWidgetGL : public RegenWidgetGL, public State {
	public:
		explicit TextureWidgetGL(TextureWidget *widget, QWidget *container)
				: RegenWidgetGL(widget, container),
				  State(),
				  widget_(widget) {
			setAutoFillBackground(false);
			setAttribute(Qt::WA_OpaquePaintEvent);
			updateSize();
			updateState_ = ref_ptr<State>::alloc();
			updateState_->shaderDefine("DRAW_BORDERS", "TRUE");
		}

		void initializeGL() override {
		}

		void initializeGL_() {
			if(isInitialized_) return;

			auto colorBuffer = ref_ptr<Texture2D>::alloc();
			colorBuffer->set_rectangleSize(width_, height_);
			colorBuffer->set_pixelType(GL_UNSIGNED_BYTE);
			colorBuffer->set_format(GL_RGB);
			colorBuffer->set_internalFormat(GL_RGB8);
			colorBuffer->allocTexture();
			colorBuffer->set_filter(TextureFilter::create(GL_LINEAR));
			colorBuffer->set_wrapping(TextureWrapping::create(GL_REPEAT));

			// create a framebuffer object for the texture
			auto fbo = ref_ptr<FBO>::alloc(width_, height_);
			auto attachment = fbo->addTexture(colorBuffer);
			fboState_ = ref_ptr<FBOState>::alloc(fbo);
			ClearColorState::Data clearData;
			clearData.clearColor = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
			clearData.colorBuffers = { attachment };
			fboState_->addDrawBuffer(attachment);
			fboState_->setClearColor(clearData);
			updateState_->joinStates(fboState_);

			texture_ = ref_ptr<Texture>::dynamicCast(widget_->input());
			if(!texture_.get()) return;
			switch (texture_->textureBind().target_) {
				case GL_TEXTURE_BUFFER:
					// skip rendering for this texture type
					return;
				default:
					break;
			}

			// Note: this state is used in the main render thread, attached
			// to the node that hosts the texture.
			auto textureState = ref_ptr<TextureState>::alloc(texture_, "inputTexture");
			updateState_->joinStates(textureState);

			auto fullscreenMesh = Rectangle::getUnitQuad();

			auto rs = RenderState::get();
			auto oldVAO = rs->vao().current();

			StateConfigurer shaderConfigurer;
			shaderConfigurer.addState(updateState_.get());
			shaderConfigurer.addState(textureState.get());
			shaderState_ = ref_ptr<ShaderState>::alloc();
			shaderState_->createShader(shaderConfigurer.cfg(), getShaderKey(texture_));
			fullscreenMesh->updateVAO(shaderConfigurer.cfg(), shaderState_->shader());
			updateState_->joinStates(shaderState_);
			updateState_->joinStates(fullscreenMesh);
			isInitialized_ = true;

			rs->vao().apply(oldVAO);
		}

		static std::string getShaderKey(const ref_ptr<Texture> &texture) {
			auto isDepth = texture->format() == GL_DEPTH_COMPONENT;
			auto isShadow = (texture->samplerType() == "samplerShadow" ||
							texture->samplerType() == "sampler2DShadow" ||
							texture->samplerType() == "samplerCubeShadow");
			std::stringstream ss;
			ss << "regen.filter.sampling";
			switch (texture->textureBind().target_) {
				case GL_TEXTURE_CUBE_MAP:
					ss << ".cube";
					break;
				case GL_TEXTURE_2D_ARRAY:
					ss << ".array";
					break;
				default:
					break;
			}
			if (isShadow) {
				ss << ".shadow";
			} else if (isDepth) {
				ss << ".depth";
			} else {
				ss << ".color";
			}
			return ss.str();
		}

		void enable(RenderState *rs) override {
			initializeGL_();

			// We need to manually reset the draw buffer state.
			// The reason is that currently this is handled by a "parent-FBO" link
			// that we do not provide here, so we need to reset the draw buffer
			// state manually.
			auto oldDB = rs->drawFrameBuffer().current();
			auto oldViewport = rs->viewport().current();

			updateState_->enable(rs);
			updateState_->disable(rs);

			rs->drawFrameBuffer().apply(oldDB);
			rs->viewport().apply(oldViewport);
		}

		void paintGL() override {
			if (!fboState_.get()) return;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fboState_->fbo()->id());
			glBlitFramebuffer(
				0, 0, width_, height_,
				0, 0, width_, height_,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			GL_ERROR_LOG();
		}

		void updateSize() {
			auto texture = dynamic_cast<Texture *>(widget_->input().get());
			if (texture) {
				if (texture->width() >  texture->height()) {
					width_ = 256;
					height_ = static_cast<int>(256.0 *  texture->height() / texture->width());
				} else {
					height_ = 256;
					width_ = static_cast<int>(256.0 * texture->width() /  texture->height());
				}
			}
			setFixedSize(width_, height_);
		}

	private:
		TextureWidget *widget_;
		ref_ptr<FBOState> fboState_;
		ref_ptr<ShaderState> shaderState_;
		ref_ptr<State> updateState_;
		ref_ptr<Texture> texture_;
		int width_ = 256;
		int height_ = 256;
		bool isInitialized_ = false;
	};
}

TextureWidget::TextureWidget(const RegenWidgetData &data, QWidget *parent)
		: RegenWidget(data, parent) {
	auto widget = ref_ptr<TextureWidgetGL>::alloc(this, parent);
	widgetState_ = widget;
	textureWidget_ = widget.get();

	// Create a layout and add the QLabel to it
	auto *layout = new QVBoxLayout(this);
	layout->addWidget(textureWidget_, 0, Qt::AlignCenter);
	setLayout(layout);

    // Need to capture the texture on node traversal
    node_->state()->joinStatesFront(widgetState_);
}

TextureWidget::~TextureWidget() {
	if (widgetState_.get()) {
		node_->state()->disjoinStates(widgetState_);
		widgetState_ = {};
	}
}

void TextureWidget::hideEvent(QHideEvent *event) {
	if (widgetState_.get()) {
		widgetState_->set_isHidden(true);
	}
	RegenWidget::hideEvent(event);
}

void TextureWidget::showEvent(QShowEvent *event) {
	if (widgetState_.get()) {
		widgetState_->set_isHidden(false);
	}
	RegenWidget::showEvent(event);
}

bool TextureWidget::hasTextureFile() const {
	auto texture = dynamic_cast<Texture *>(input().get());
	if (texture) {
		return texture->hasTextureFile();
	}
	return false;
}

void TextureWidget::openFile() {
    auto texture = ref_ptr<Texture>::dynamicCast(input());
    if (!texture.get()) {
        QMessageBox::warning(this, tr("Error"), tr("No texture available."));
        return;
    }

    std::string initialDir;
    if (texture->hasTextureFile()) {
		initialDir = texture->textureFile()->filePath;
		if(initialDir.empty()) {
			initialDir = boost::filesystem::path(
				texture->textureFile()->fileName).parent_path().string();
		}
	}

    auto selectedFile = QFileDialog::getOpenFileName(
    	this,
    	tr("Open Texture File"),
    	QString::fromStdString(initialDir),
    	tr("Image Files (*.png *.jpg *.bmp *.tga *.dds);;All Files (*)"),
    	nullptr,
    	QFileDialog::Options(QFileDialog::DontUseNativeDialog | QFileDialog::ReadOnly));
    if (!selectedFile.isEmpty()) {
        try {
            textures::reload(texture, selectedFile.toStdString());
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to load texture: %1").arg(e.what()));
        }
    }
}
