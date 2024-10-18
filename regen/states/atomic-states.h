/*
 * atomic-states.h
 *
 *  Created on: 28.02.2013
 *      Author: daniel
 */

#ifndef ATOMIC_STATES_H_
#define ATOMIC_STATES_H_

#include <regen/states/state.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	/**
	 * \brief Base class for 'atomic' states.
	 *
	 * Atomic states just push and pop to the RenderState.
	 */
	class ServerSideState : public State {
	};

	/**
	 * \brief Toggles server side GL state.
	 */
	class ToggleState : public ServerSideState {
	public:
		/**
		 * @param key the toggle key.
		 * @param toggle the toggle value.
		 */
		ToggleState(RenderState::Toggle key, GLboolean toggle)
				: ServerSideState(), key_(key), toggle_(toggle) {}

		/**
		 * @return the toggle key.
		 */
		RenderState::Toggle key() const { return key_; }

		/**
		 * @return the toggle value.
		 */
		GLboolean toggle() const { return toggle_; }

		void enable(RenderState *rs) override { rs->toggles().push(key_, toggle_); }

		void disable(RenderState *rs) override { rs->toggles().pop(key_); }

	protected:
		RenderState::Toggle key_;
		GLboolean toggle_;
	};

	/**
	 * \brief Specifies the depth comparison function.
	 */
	class DepthFuncState : public ServerSideState {
	public:
		/**
		 * Symbolic constants GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,
		 * GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS are accepted.
		 * The initial value is GL_LESS.
		 */
		explicit DepthFuncState(GLenum depthFunc)
				: ServerSideState(), depthFunc_(depthFunc) {}

		void enable(RenderState *rs) override { rs->depthFunc().push(depthFunc_); }

		void disable(RenderState *rs) override { rs->depthFunc().pop(); }

	protected:
		GLenum depthFunc_;
	};

	/**
	 * \brief Specify mapping of depth values from normalized device coordinates
	 * to window coordinates.
	 */
	class DepthRangeState : public ServerSideState {
	public:
		/**
		 * @param nearVal specifies the mapping of the near clipping plane to window coordinates.
		 *    The initial value is 0.
		 * @param farVal specifies the mapping of the far clipping plane to window coordinates.
		 *    The initial value is 1.
		 */
		DepthRangeState(GLdouble nearVal, GLdouble farVal)
				: ServerSideState(), nearVal_(nearVal), farVal_(farVal) {}

		void enable(RenderState *rs) override { rs->depthRange().push(DepthRange(nearVal_, farVal_)); }

		void disable(RenderState *rs) override { rs->depthRange().pop(); }

	protected:
		GLdouble nearVal_, farVal_;
	};

	/**
	 * \brief Specifies whether the depth buffer is enabled for writing.
	 */
	class ToggleDepthWriteState : public ServerSideState {
	public:
		/**
		 * If flag is GL_FALSE, depth buffer writing is disabled.
		 * Otherwise, it is enabled. Initially, depth buffer writing is enabled.
		 */
		explicit ToggleDepthWriteState(GLboolean toggle)
				: ServerSideState(), toggle_(toggle) {}

		void enable(RenderState *rs) override { rs->depthMask().push(toggle_); }

		void disable(RenderState *rs) override { rs->depthMask().pop(); }

	protected:
		GLboolean toggle_;
	};

	/**
	 * \brief Set the blend color.
	 */
	class BlendColorState : public ServerSideState {
	public:
		/**
		 * Initially the GL_BLEND_COLOR is set to (0,0,0,0).
		 */
		explicit BlendColorState(const Vec4f &col) : ServerSideState(), col_(col) {}

		void enable(RenderState *state) override { state->blendColor().push(col_); }

		void disable(RenderState *state) override { state->blendColor().pop(); }

	protected:
		Vec4f col_;
	};

	/**
	 * \brief Specify the equation used for both the RGB blend equation and the
	 * Alpha blend equation.
	 */
	class BlendEquationState : public ServerSideState {
	public:
		/**
		 * @param equation specifies how source and destination colors are combined.
		 * It must be GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX.
		 * Initially, both the RGB blend equation and the alpha blend equation
		 * are set to GL_FUNC_ADD.
		 */
		explicit BlendEquationState(GLenum equation)
				: ServerSideState(), equation_(BlendEquation(equation, equation)) {}

		void enable(RenderState *state) override { state->blendEquation().push(equation_); }

		void disable(RenderState *state) override { state->blendEquation().pop(); }

	protected:
		BlendEquation equation_;
	};

	/**
	 * \brief Specify pixel arithmetic.
	 */
	class BlendFuncState : public ServerSideState {
	public:
		/**
		 * The following symbolic constants are accepted:
		 * GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_DST_COLOR,
		 * GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
		 * GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA.GL_CONSTANT_COLOR,
		 * GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,
		 * GL_ONE_MINUS_CONSTANT_ALPHA. The initial value is GL_ZERO.
		 *
		 * @param srcRGB specifies how the red, green, blue source blending
		 * factors are computed. The initial value is GL_ONE.
		 * @param dstRGB specifies how the red, green, blue destination
		 * blending factors are computed.
		 * @param srcAlpha specifies how the alpha source blending
		 * factor is computed. The initial value is GL_ONE.
		 * @param dstAlpha specifies how the alpha destination
		 * blending factor is computed.
		 */
		BlendFuncState(
				GLenum srcRGB, GLenum dstRGB,
				GLenum srcAlpha, GLenum dstAlpha)
				: ServerSideState(), func_(BlendFunction(srcRGB, dstRGB, srcAlpha, dstAlpha)) {}

		void enable(RenderState *state) override { state->blendFunction().push(func_); }

		void disable(RenderState *state) override { state->blendFunction().pop(); }

	protected:
		BlendFunction func_;
	};

	/**
	 * Specifies whether front- or back-facing facets are candidates for culling.
	 */
	class CullFaceState : public ServerSideState {
	public:
		/**
		 * Symbolic constants GL_FRONT,GL_BACK, GL_FRONT_AND_BACK are accepted.
		 * The initial value is GL_BACK.
		 */
		explicit CullFaceState(GLenum face) : ServerSideState(), face_(face) {}

		void enable(RenderState *rs) override { rs->cullFace().push(face_); }

		void disable(RenderState *rs) override { rs->cullFace().pop(); }

	protected:
		GLenum face_;
	};

	/**
	 * Specifies the orientation of front-facing polygons.
	 * GL_CW and GL_CCW are accepted. The initial value is GL_CCW.
	 */
	class FrontFaceState : public ServerSideState {
	public:
		/**
		 * @param ordering GL_CW and GL_CCW are accepted.
		 */
		explicit FrontFaceState(GLenum ordering) : ServerSideState(), ordering_(ordering) {}

		// Override
		void enable(RenderState *rs) override { rs->frontFace().push(ordering_); }

		void disable(RenderState *rs) override { rs->frontFace().pop(); }

	protected:
		GLenum ordering_;
	};

	/**
	 * \brief Set the scale and units used to calculate depth values.
	 *
	 * This state also enables the polygon offset toggle.
	 */
	class PolygonOffsetState : public ServerSideState {
	public:
		/**
		 * @param factor specifies a scale factor that is used to create a variable
		 *    depth offset for each polygon. The initial value is 0.
		 * @param units is multiplied by an implementation-specific value to
		 *    create a constant depth offset. The initial value is 0.
		 */
		PolygonOffsetState(GLfloat factor, GLfloat units)
				: ServerSideState(), factor_(factor), units_(units) {}

		void enable(RenderState *rs) override {
			rs->toggles().push(RenderState::POLYGON_OFFSET_FILL, GL_TRUE);
			rs->polygonOffset().push(Vec2f(factor_, units_));
		}

		void disable(RenderState *rs) override {
			rs->polygonOffset().pop();
			rs->toggles().pop(RenderState::POLYGON_OFFSET_FILL);
		}

	protected:
		GLfloat factor_, units_;
	};

	/**
	 * \brief Specifies how polygons will be rasterized.
	 */
	class FillModeState : public ServerSideState {
	public:
		/**
		 * Accepted values are GL_POINT,GL_LINE,GL_FILL.
		 * The initial value is GL_FILL for both front- and back-facing polygons.
		 */
		explicit FillModeState(GLenum mode) : ServerSideState(), mode_(mode) {}

		void enable(RenderState *rs) override { rs->polygonMode().push(mode_); }

		void disable(RenderState *rs) override { rs->polygonMode().pop(); }

	protected:
		GLenum mode_;
	};

	/**
	 * \brief Specifies the number of vertices that
	 * will be used to make up a single patch primitive.
	 */
	class PatchVerticesState : public ServerSideState {
	public:
		/**
		 * @param numPatchVertices Specifies the number of vertices that
		 * will be used to make up a single patch primitive.
		 */
		explicit PatchVerticesState(GLuint numPatchVertices)
				: ServerSideState(), numPatchVertices_(numPatchVertices) {}

		void enable(RenderState *rs) override { rs->patchVertices().push(numPatchVertices_); }

		void disable(RenderState *rs) override { rs->patchVertices().pop(); }

	protected:
		GLuint numPatchVertices_;
	};

	/**
	 * \brief Specifies the default outer or inner tessellation levels
	 * to be used when no tessellation control shader is present.
	 */
	class PatchLevelState : public ServerSideState {
	public:
		/**
		 * Specifies the default outer or inner tessellation levels
		 * to be used when no tessellation control shader is present.
		 * @param inner the inner level.
		 * @param outer the outer level.
		 */
		PatchLevelState(const ref_ptr<ShaderInput4f> &inner, const ref_ptr<ShaderInput4f> &outer)
				: ServerSideState(), inner_(inner), outer_(outer) {}

		void enable(RenderState *rs) override { rs->patchLevel().push(PatchLevels(inner(), outer())); }

		void disable(RenderState *rs) override { rs->patchLevel().pop(); }

		/**
		 * @return the inner patch level.
		 */
		const Vec4f &inner() const { return inner_->getVertex(0); }

		/**
		 * @return the outer patch level.
		 */
		const Vec4f &outer() const { return outer_->getVertex(0); }

	protected:
		ref_ptr<ShaderInput4f> inner_;
		ref_ptr<ShaderInput4f> outer_;
	};

	/**
	 * \brief Clear depth buffer to preset values.
	 */
	class ClearDepthState : public ServerSideState {
	public:
		void enable(RenderState *state) override { glClear(GL_DEPTH_BUFFER_BIT); }
	};

	/**
	 * \brief Clear color buffers to preset values.
	 */
	class ClearColorState : public ServerSideState {
	public:
		/**
		 * \brief color buffers to be cleared.
		 */
		struct Data {
			/** the clear color. */
			Vec4f clearColor;
			/** list of color buffers to be cleared. */
			DrawBuffers colorBuffers;
		};
		/** list of color buffers to be cleared. */
		std::list<Data> data;

		/** @param fbo the framebuffer */
		explicit ClearColorState(const ref_ptr<FBO> &fbo)
				: ServerSideState(), fbo_(fbo) {}

		// override
		void enable(RenderState *rs) override {
			for (std::list<Data>::iterator it = data.begin(); it != data.end(); ++it) {
				if (!rs->drawFrameBuffer().isLocked()) {
					fbo_->drawBuffers().push(it->colorBuffers);
					rs->clearColor().push(it->clearColor);
					glClear(GL_COLOR_BUFFER_BIT);
					rs->clearColor().pop();
					fbo_->drawBuffers().pop();
				}
			}
		}

	protected:
		ref_ptr<FBO> fbo_;
	};

	/**
	 * \brief Specifies a list of color buffers to be drawn into.
	 */
	class DrawBufferState : public ServerSideState {
	public:
		/** list of color buffers to be drawn into. */
		DrawBuffers colorBuffers;

		/** @param fbo the framebuffer */
		explicit DrawBufferState(const ref_ptr<FBO> &fbo)
				: ServerSideState(), fbo_(fbo) {}

		// override
		void enable(RenderState *rs) override { fbo_->drawBuffers().push(colorBuffers); }

		void disable(RenderState *rs) override { fbo_->drawBuffers().pop(); }

	protected:
		ref_ptr<FBO> fbo_;
	};

	/**
	 * \brief Specifies a list of color buffers to be drawn into. Each frame
	 *            activates a single draw buffer and in the next frame the next draw buffer
	 *            the list is activated.
	 */
	class PingPongBufferState : public ServerSideState {
	public:
		/** list of color buffers to be drawn into. */
		DrawBuffers colorBuffers;

		/** @param fbo the framebuffer */
		explicit PingPongBufferState(const ref_ptr<FBO> &fbo)
				: ServerSideState(), fbo_(fbo), index_(0u) {}

		// override
		void enable(RenderState *rs) override {
			DrawBuffers v(colorBuffers.buffers_[index_]);
			fbo_->drawBuffers().push(v);
			index_ = (index_ + 1) % colorBuffers.buffers_.size();
		}

		void disable(RenderState *rs) override { fbo_->drawBuffers().pop(); }

	protected:
		ref_ptr<FBO> fbo_;
		/** Current index. */
		GLuint index_;
	};
} // namespace

#endif /* ATOMIC_STATES_H_ */
