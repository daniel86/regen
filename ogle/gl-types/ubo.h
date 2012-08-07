/*
 * ubo.h
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#ifndef UBO_H_
#define UBO_H_

#include <ogle/gl-types/buffer-object.h>

/**
 * A Buffer Object that is used to store uniform data for a shader program
 * is called a Uniform Buffer Object. They can be used to share
 * uniforms between different programs, as well as quickly change
 * between sets of uniforms for the same program object.
 *
 * The term "Uniform Buffer Object" refers to the OpenGL buffer object
 * that is used to provide storage for uniforms. The term "uniform blocks"
 * refer to the GLSL language grouping of uniforms that must have buffer
 * objects as storage.
 */

/**
 * New Procedures and Functions

   void    BindBufferRange(enum target,
                           uint index,
                           uint buffer,
                           intptr offset,
                           sizeiptr size);

   void    BindBufferBase(enum target,
                          uint index,
                          uint buffer);

   void    UniformBlockBinding(uint program,
                                  uint uniformBlockIndex,
                                  uint uniformBlockBinding);
 */

/*
 * New Tokens

    Accepted by the <target> parameters of BindBuffer, BufferData,
    BufferSubData, MapBuffer, UnmapBuffer, GetBufferSubData, and
    GetBufferPointerv:

        UNIFORM_BUFFER                                  0x8A11
 */
class UniformbufferObject : public BufferObject
{
public:
  // TODO: support UBO's
  UniformbufferObject();
};

#endif /* UBO_H_ */
