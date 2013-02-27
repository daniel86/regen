/*
 * gl-util.h
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#ifndef __GL_UTIL__
#define __GL_UTIL__

#include <sstream>
using namespace std;

#include <GL/glew.h>
#include <GL/gl.h>

/**
 * Check for usual GL errors.
 */
void handleGLError(const string &context);
/**
 * Check for usual GL errors.
 */
void handleFBOError(const string &context, GLenum target=GL_FRAMEBUFFER);

GLint getGLInteger(GLenum e);
GLfloat getGLFloat(GLenum e);

#endif /* __GL_UTIL__ */
