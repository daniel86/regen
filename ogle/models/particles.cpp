/*
 * particles.cpp
 *
 *  Created on: 02.11.2012
 *      Author: daniel
 */

#include "particles.h"

Particles::Particles(list< ref_ptr<ShaderInput> > &atts)
: MeshState(GL_POINTS),
  Animation(),
  numParticles_()
{
  GLuint bufferSize = 0;
  for(list< ref_ptr<ShaderInput> >::iterator
      it=atts.begin(); it!=atts.end(); ++it)
  {
    ref_ptr<ShaderInput> &in = *it;
    setInput(in);
    if(in->numVertices()<=1) { continue; }
    bufferSize += in->elementSize();
  }
  bufferSize *= numVertices_;

  //
  particleBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, bufferSize));
  particleBuffer_->allocateInterleaved(particleAttributes);
  //
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_DYNAMIC, bufferSize));

  {
    map<GLenum,string> shaderNames;
    map<string,string> shaderConfig;
    map<string,string> functions;

    shaderNames[GL_VERTEX_SHADER] = "particles.update.vs";
    shaderNames[GL_GEOMETRY_SHADER] = "particles.update.gs";

    updateShader_ = Shader::create(shaderConfig,functions,shaderNames);
    updateShader_->setTransformFeedback(transformFeedback, GL_INTERLEAVED_ATTRIBS);
    if(updateShader_->compile() && updateShader_->link())
    {

    }
  }

  {
    map<GLenum,string> shaderNames;
    map<string,string> shaderConfig;
    map<string,string> functions;

    shaderNames[GL_VERTEX_SHADER] = "particles.draw.vs";
    shaderNames[GL_FRAGMENT_SHADER] = "particles.draw.fs";

    drawShader_ = Shader::create(shaderConfig,functions,shaderNames);
    drawShader_->setTransformFeedback(transformFeedback, GL_INTERLEAVED_ATTRIBS);
    if(drawShader_->compile() && drawShader_->link())
    {

    }
  }
}

void Particles::animate(GLdouble dt){}

void Particles::updateGraphics(GLdouble dt)
{
  glEnable(GL_RASTERIZER_DISCARD);
  glDepthMask(GL_FALSE);

  // setup the interpolation shader
  glUseProgram(updateShader_->id());
  updateShader_->uploadInputs();

  // currently active frames are saved in animation buffer
  glBindBuffer(GL_ARRAY_BUFFER, particleBuffer_->id());
  // setup the transform feedback
  glBindBufferRange(
      GL_TRANSFORM_FEEDBACK_BUFFER,
      0, feedbackBuffer_->id(),
      0, feedbackBuffer_->bufferSize()
  );
  glBeginTransformFeedback(GL_POINTS);

  // setup attributes
  for(list<ShaderAttributeLocation>::iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    it->att->enable(it->location);
  }

  // finally the draw call
  glDrawArrays(GL_POINTS, 0, numVertices_);

  // cleanup
  glEndTransformFeedback();
  glDisable(GL_RASTERIZER_DISCARD);
  glDepthMask(GL_TRUE);

  // switch buffers
  ref_ptr<VertexBufferObject> buf = particleBuffer_;
  particleBuffer_ = feedbackBuffer_;
  feedbackBuffer_ = buf;
}
