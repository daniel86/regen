#ifndef REGEN_FILTER_H_
#define REGEN_FILTER_H_

#include <regen/states/fullscreen-pass.h>
#include <regen/textures/texture-state.h>
#include "regen/textures/fbo-state.h"
#include <regen/scene/loading-context.h>

namespace regen {
	/**
	 * \brief Apply a filter on input Texture.
	 */
	class Filter : public FullscreenPass {
	public:
		/**
		 * \brief Ping-Pong filter output target.
		 */
		struct Output {
			/** FBO reference. */
			ref_ptr<FBO> fbo_;
			/** Ping texture. */
			ref_ptr<Texture> tex0_;
			/** Pong texture. */
			ref_ptr<Texture> tex1_;
		};

		/**
		 * \note You have to call setInput() once or add the filter to a
		 * FilterSequence before using the filter.
		 */
		explicit Filter(std::string_view shaderKey, float scaleFactor = 1.0);

		/**
		 * @param v toggles binding the input texture before filter is executed.
		 */
		void set_bindInput(bool v);

		/**
		 * @param v target format.
		 */
		void set_format(GLenum v) { format_ = v; }

		/**
		 * @param v target internal format.
		 */
		void set_internalFormat(GLenum v) { internalFormat_ = v; }

		/**
		 * @param v target pixel type.
		 */
		void set_pixelType(GLenum v) { pixelType_ = v; }

		/**
		 * Scale factor that is applied to the input texture when
		 * filtering.
		 */
		float scaleFactor() const { return scaleFactor_; }

		/**
		 * Filter render target with ping-pong attachment points.
		 */
		auto &output() const { return out_; }

		/**
		 * The color attachment point for the filter result texture.
		 */
		auto outputAttachment() const { return outputAttachment_; }

		/**
		 * Set input texture and create a framebuffer for this filter.
		 */
		void setInput(const ref_ptr<Texture> &input);

		/**
		 * Set input texture and use provided framebuffer.
		 */
		void setInput(const ref_ptr<Output> &lastOutput, GLenum lastAttachment);

	protected:
		ref_ptr<Texture> input_;
		ref_ptr<Output> out_;
		GLenum outputAttachment_;

		ref_ptr<DrawBufferState> drawBufferState_;
		ref_ptr<TextureState> inputState_;
		ref_ptr<ShaderState> shader_;

		float scaleFactor_;

		GLenum format_;
		GLenum internalFormat_;
		GLenum pixelType_;
		bool bindInput_;

		void set_input(const ref_ptr<Texture> &input);

		ref_ptr<Texture> createTexture();
	};
} // end namespace

namespace regen {
	/**
	 * \brief Filters input texture by applying a sequence of
	 * filters to the input.
	 */
	class FilterSequence : public State {
	public:
		/**
		 * @param input the input texture.
		 * @param bindInput bind and activate input before filtering.
		 */
		explicit FilterSequence(const ref_ptr<Texture> &input, bool bindInput = true);

		static ref_ptr<FilterSequence> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Creates filter shaders.
		 * @param cfg the shader config.
		 */
		void createShader(StateConfig &cfg);

		/**
		 * Set color for clearing the color buffer
		 * before the filter is executed.
		 * @param v
		 */
		void setClearColor(const Vec4f &v);

		/**
		 * @param v target format.
		 */
		void set_format(GLenum v) { format_ = v; }

		/**
		 * @param v target internal format.
		 */
		void set_internalFormat(GLenum v) { internalFormat_ = v; }

		/**
		 * @param v target pixel type.
		 */
		void set_pixelType(GLenum v) { pixelType_ = v; }

		/**
		 * Adds a filter to the sequence of filters.
		 * @param f
		 */
		void addFilter(const ref_ptr<Filter> &f);

		/**
		 * @return the input texture.
		 */
		auto &input() const { return input_; }

		/**
		 * @return the output texture.
		 */
		const ref_ptr<Texture> &output() const;

		// override
		void enable(RenderState *state) override;

	protected:
		struct FilterData {
			ref_ptr<Filter> filter;
			ref_ptr<ShaderInput2f> viewport;
			ref_ptr<ShaderInput2f> inverseViewport;
		};
		std::list<FilterData> filterSequence_;
		ref_ptr<Texture> input_;

		bool clearFirstFilter_;
		Vec4f clearColor_;
		uint32_t lastWidth_;
		uint32_t lastHeight_;

		bool bindInput_;
		GLenum format_;
		GLenum internalFormat_;
		GLenum pixelType_;

		void resize();
	};
} // end namespace

#endif /* REGEN_FILTER_H_ */
