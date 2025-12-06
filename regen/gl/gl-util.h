/*
 * gl-util.h
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#ifndef __GL_UTIL__
#define __GL_UTIL__

#include <sstream>

#include <GL/glew.h>
#include <regen/utility/strings.h>
#include <regen/utility/logging.h>
#include <regen/config.h>

namespace regen {
	/**
	 * Log the GL error state.
	 */
#ifdef REGEN_DEBUG_BUILD
#define GL_ERROR_LOG() glHandleError(__FILE__, __LINE__)
#else
#define GL_ERROR_LOG()
#endif
	/**
	 * Log the FBO error state.
	 */
#ifdef REGEN_DEBUG_BUILD
#define FBO_ERROR_LOG() REGEN_ERROR( getFBOError(GL_FRAMEBUFFER) )
#else
#define FBO_ERROR_LOG()
#endif

	/**
	 * Query GL error state.
	 */
#ifdef REGEN_DEBUG_BUILD

	std::string getGLError();

	void glHandleError(const char *file, int line);

#else
#define getGLError()
#endif
	/**
	 * Query FBO error state.
	 */
#ifdef REGEN_DEBUG_BUILD

	std::string getFBOError(GLenum target);

#else
#define getFBOError(t)
#endif

	/**
	 * Query a GL query result.
	 */
	uint32_t getGLQueryResult(uint32_t query);

	/**
	 * Query a GL integer attribute.
	 */
	int glGetInteger(GLenum e);

	/**
	 * Query a GL integer attribute.
	 * Check for required extension if not supported return default value.
	 */
	int glGetInteger(const std::string &ext, GLenum key, int defaultValue);

	/**
	 * Query a GL float attribute.
	 */
	float glGetFloat(GLenum e);

	int getGLBufferInteger(GLenum target, GLenum e);

} // namespace

#endif /* __GL_UTIL__ */
