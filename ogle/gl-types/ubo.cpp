/*
 * ubo.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include <ogle/gl-types/ubo.h>

UniformBufferObject::UniformBufferObject()
: BufferObject(glGenBuffers,glDeleteBuffers),
  blockSize_(0)
{
}

UniformBufferObject::Layout UniformBufferObject::layout() const
{
  return layout_;
}

void UniformBufferObject::set_layout(Layout layout)
{
  layout_ = layout;
}
