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
#include <ogle/utility/string-util.h>

namespace ogle {
/**
 * Log the GL error state.
 */
#define GL_ERROR_LOG() ERROR_LOG( getGLError() )
/**
 * Log the FBO error state.
 */
#define FBO_ERROR_LOG() ERROR_LOG( getGLError() )

/**
 * Query GL error state.
 */
string getGLError();
/**
 * Query FBO error state.
 */
string getFBOError(GLenum target=GL_FRAMEBUFFER);

/**
 * Query a GL query result.
 */
GLuint getGLQueryResult(GLuint query);
/**
 * Query a GL integer attribute.
 */
GLint getGLInteger(GLenum e);
/**
 * Query a GL float attribute.
 */
GLfloat getGLFloat(GLenum e);

} // end ogle namespace

#endif /* __GL_UTIL__ */
