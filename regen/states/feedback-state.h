/*
 * feedback-state.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef FEEDBACK_STATE_H_
#define FEEDBACK_STATE_H_

#include <regen/states/state.h>

namespace regen {
  /**
   * \brief Transform feedback state.
   *
   * The state is supposed to be wrapped around a draw call.
   * It will update the transform feedback buffer but it will not do anything with
   * the acquired data.
   */
  class FeedbackState : public State
  {
  public:
    /**
     * @param feedbackPrimitive face primitive type.
     * @param feedbackCount number of captured vertices. With 0 each vertex is captured.
     */
    FeedbackState(GLenum feedbackPrimitive, GLuint feedbackCount);

    /**
     * @return face primitive used for transform feedback.
     */
    GLenum feedbackPrimitive() const;

    /**
     * @param feedbackCount number of captured vertices. With 0 each vertex is captured.
     */
    void set_feedbackCount(GLuint feedbackCount);

    /**
     * Allowed values are GL_INTERLEAVED_ATTRIBS and
     * GL_SEPARATE_ATTRIBS.
     * @param mode transform feedback mode.
     */
    void set_feedbackMode(GLenum mode);
    /**
     * @return transform feedback mode.
     */
    GLenum feedbackMode() const;

    /**
     * @param stage the Shader stage that should be captured.
     */
    void set_feedbackStage(GLenum stage);
    /**
     * @return the Shader stage that should be captured.
     */
    GLenum feedbackStage() const;

    /**
     * Add an attribute to the list of feedback attributes.
     * @param in the attribute.
     */
    void addFeedback(const ref_ptr<ShaderInput> &in);
    /**
     * @param in remove previously added feedback attribute.
     */
    void removeFeedback(ShaderInput *in);
    /**
     * @param name name of an attribute.
     * @return true if there is a feedback attribute with given name.
     */
    GLboolean hasFeedback(const string &name) const;
    /**
     * @param name feedback name
     * @return previously added feedback attribute.
     */
    ref_ptr<ShaderInput> getFeedback(const string &name);
    /**
     * @return list of captured attributes.
     */
    const list< ref_ptr<ShaderInput> >& feedbackAttributes() const;
    /**
     * @return VBO containing the last feedback data.
     */
    const ref_ptr<VBO>& feedbackBuffer() const;

    /**
     * Render primitives from transform feedback array data.
     */
    void draw(GLuint numInstances);

    // override
    void enable(RenderState *rs);
    void disable(RenderState *rs);

  protected:
    typedef list< ref_ptr<ShaderInput> > FeedbackList;

    GLenum feedbackPrimitive_;
    GLenum feedbackMode_;
    GLenum feedbackStage_;
    GLuint feedbackCount_;

    GLuint requiredBufferSize_;
    GLuint allocatedBufferSize_;
    ref_ptr<VBO> feedbackBuffer_;
    BufferRange bufferRange_;
    FeedbackList feedbackAttributes_;
    VBOReference vboRef_;

    map<string, FeedbackList::iterator> feedbackAttributeMap_;

    void (FeedbackState::*enable_)(RenderState *rs);
    void enableInterleaved(RenderState *rs);
    void enableSeparate(RenderState *rs);

    void (FeedbackState::*disable_)(RenderState *rs);
    void disableInterleaved(RenderState *rs);
    void disableSeparate(RenderState *rs);
  };
} // namespace

#endif /* FEEDBACK_STATE_H_ */
