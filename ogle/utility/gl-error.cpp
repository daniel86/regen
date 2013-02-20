/*
 * gl-error.cpp
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#include "gl-error.h"
#include "logging.h"

void handleGLError(const string &context)
{
  GLenum err = glGetError();
  switch(err) {
  case GL_NO_ERROR:
    break;
  case GL_INVALID_ENUM:
    ERROR_LOG(context << ": GL_INVALID_ENUM");
    break;
  case GL_INVALID_VALUE:
    ERROR_LOG(context << ": GL_INVALID_VALUE");
    break;
  case GL_INVALID_OPERATION:
    ERROR_LOG(context << ": GL_INVALID_OPERATION");
    break;
  case GL_STACK_OVERFLOW:
    ERROR_LOG(context << ": GL_STACK_OVERFLOW");
    break;
  case GL_STACK_UNDERFLOW:
    ERROR_LOG(context << ": GL_STACK_UNDERFLOW");
    break;
  case GL_OUT_OF_MEMORY:
    ERROR_LOG(context << ": GL_OUT_OF_MEMORY");
    break;
  case GL_TABLE_TOO_LARGE:
    ERROR_LOG(context << ": GL_TABLE_TOO_LARGE");
    break;
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    ERROR_LOG(context << ": GL_INVALID_FRAMEBUFFER_OPERATION");
    break;
  default:
    ERROR_LOG(context << ": unknown error 0x" << hex << err);
    break;
  }
}

void handleFBOError(const string &context, GLenum target)
{
  GLenum err = glCheckFramebufferStatus(target);
  switch(err)
  {
  case GL_FRAMEBUFFER_COMPLETE:
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
    break;
  case GL_FRAMEBUFFER_UNSUPPORTED:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_UNSUPPORTED");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
    break;
  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT:
    ERROR_LOG(context << ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT");
    break;
  default:
    ERROR_LOG(context << ": unknown error 0x" << hex << err);
    break;
  }
}
