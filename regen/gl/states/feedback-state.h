#ifndef FEEDBACK_STATE_H_
#define FEEDBACK_STATE_H_

#include <regen/scene/state.h>
#include <regen/memory/vbo.h>

namespace regen {
	class FeedbackSpecification : public State {
	public:
		explicit FeedbackSpecification(uint32_t feedbackCount);

		/**
		 * @return number of captured vertices.
		 */
		auto feedbackCount() const { return feedbackCount_; }

		/**
		 * @param mode transform feedback mode.
		 */
		void set_feedbackMode(GLenum mode) { feedbackMode_ = mode; }

		/**
		 * Allowed values are GL_INTERLEAVED_ATTRIBS and
		 * GL_SEPARATE_ATTRIBS.
		 * @param mode transform feedback mode.
		 */
		auto feedbackMode() const { return feedbackMode_; }

		/**
		 * @param stage the Shader stage that should be captured.
		 */
		void set_feedbackStage(GLenum stage) { feedbackStage_ = stage; }

		/**
		 * @return the Shader stage that should be captured.
		 */
		auto feedbackStage() const { return feedbackStage_; }

		/**
		 * @return list of captured attributes.
		 */
		auto &feedbackAttributes() const { return feedbackAttributes_; }

		/**
		 * Add an attribute to the list of feedback attributes.
		 * @param in the attribute.
		 */
		ref_ptr<ShaderInput> addFeedback(const ref_ptr<ShaderInput> &in);

		/**
		 * @param in remove previously added feedback attribute.
		 */
		void removeFeedback(ShaderInput *in);

		/**
		 * @param name name of an attribute.
		 * @return true if there is a feedback attribute with given name.
		 */
		bool hasFeedback(const std::string &name) const;

		/**
		 * @param name feedback name
		 * @return previously added feedback attribute.
		 */
		ref_ptr<ShaderInput> getFeedback(const std::string &name);

	protected:
		typedef std::vector<ref_ptr<ShaderInput> > FeedbackList;

		uint32_t feedbackCount_;
		GLenum feedbackMode_;
		GLenum feedbackStage_;
		uint32_t requiredBufferSize_;

		FeedbackList feedbackAttributes_;
		std::map<std::string, FeedbackList::iterator> feedbackAttributeMap_;
	};


	/**
	 * \brief Transform feedback state.
	 *
	 * The state is supposed to be wrapped around a draw call.
	 * It will update the transform feedback buffer but it will not do anything with
	 * the acquired data.
	 */
	class FeedbackState : public FeedbackSpecification {
	public:
		/**
		 * @param feedbackPrimitive face primitive type.
		 * @param feedbackCount number of captured vertices. With 0 each vertex is captured.
		 */
		FeedbackState(GLenum feedbackPrimitive, uint32_t feedbackCount);

		/**
		 * Initialize the transform feedback buffer.
		 */
		void initializeResources();

		/**
		 * @return VBO reference.
		 */
		auto &feedbackRef() const { return feedbackRef_; }

		/**
		 * @return allocated buffer size.
		 */
		auto feedbackBufferSize() const { return allocatedBufferSize_; }

		/**
		 * Render primitives from transform feedback array data.
		 */
		void draw(uint32_t numInstances);

		// override
		void enable(RenderState *rs) override;

		void disable(RenderState *rs) override;

	protected:
		GLenum feedbackPrimitive_;

		uint32_t allocatedBufferSize_;
		ref_ptr<VBO> feedbackBuffer_;
		BufferRange bufferRange_;
		ref_ptr<BufferReference> feedbackRef_;
	};
} // namespace

#endif /* FEEDBACK_STATE_H_ */
