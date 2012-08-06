/*
 * gl-error.h
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#ifndef __GL_ERROR_
#define __GL_ERROR_

#include <sstream>
using namespace std;

#include <GL/glew.h>
#include <GL/gl.h>

/**
 * Check for usual GL errors.
 */
#ifdef ENABLE_DEBUG
void handleGLError(const string &context);
void handleFBOError(const string &context, GLenum target=GL_FRAMEBUFFER);
#else
#define handleGLError(c)
#define handleFBOError(c)
#define handleFBOError(c,t)
#endif

#endif /* __GL_ERROR_ */
