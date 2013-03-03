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

#define GL_ERROR_LOG() ERROR_LOG( getGLError() )
#define FBO_ERROR_LOG() ERROR_LOG( getGLError() )

string getGLError();
string getFBOError(GLenum target=GL_FRAMEBUFFER);

GLuint getGLQueryResult(GLuint query);
GLint getGLInteger(GLenum e);
GLfloat getGLFloat(GLenum e);

#endif /* __GL_UTIL__ */
