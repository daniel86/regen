/*
 * gl-util.cpp
 *
 *  Created on: 20.03.2011
 *      Author: daniel
 */

#include <regen/utility/logging.h>

#include "gl-util.h"

namespace regen {

#ifdef REGEN_DEBUG_BUILD

	std::string getGLError() {
		GLenum err = glGetError();
		switch (err) {
			case GL_NO_ERROR:
				return "";
			case GL_INVALID_ENUM:
				return "GL_INVALID_ENUM";
			case GL_INVALID_VALUE:
				return "GL_INVALID_VALUE";
			case GL_INVALID_OPERATION:
				return "GL_INVALID_OPERATION";
			case GL_STACK_OVERFLOW:
				return "GL_STACK_OVERFLOW";
			case GL_STACK_UNDERFLOW:
				return "GL_STACK_UNDERFLOW";
			case GL_OUT_OF_MEMORY:
				return "GL_OUT_OF_MEMORY";
			case GL_TABLE_TOO_LARGE:
				return "GL_TABLE_TOO_LARGE";
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				return "GL_INVALID_FRAMEBUFFER_OPERATION";
			default:
				return REGEN_STRING("0x" << std::hex << err);
		}
	}

	void glHandleError(const char *file, int line) {
		std::string err = getGLError();
		if (!err.empty()) {
			Logging::log(Logging::ERROR, err, file, line);
		}
	}

#endif

#ifdef REGEN_DEBUG_BUILD

	std::string getFBOError(GLenum target) {
		GLenum err = glCheckFramebufferStatus(target);
		switch (err) {
			case GL_FRAMEBUFFER_COMPLETE:
				return "";
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
			case GL_FRAMEBUFFER_UNSUPPORTED:
				return "GL_FRAMEBUFFER_UNSUPPORTED";
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT:
				return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT";
			default:
				return REGEN_STRING("0x" << std::hex << err);
		}
	}

#endif

	uint32_t getGLQueryResult(uint32_t query) {
		uint32_t v = 0;
		glGetQueryObjectuiv(query, GL_QUERY_RESULT, &v);
		return v;
	}

	int glGetInteger(GLenum e) {
		int i = 0;
		glGetIntegerv(e, &i);
		return i;
	}

	int glGetInteger(const std::string &ext, GLenum key, int defaultValue) {
		int i = defaultValue;
		if (glewIsSupported(ext.c_str())) { glGetIntegerv(key, &i); }
		return i;
	}

	float glGetFloat(GLenum e) {
		float i = 0;
		glGetFloatv(e, &i);
		return i;
	}

	int getGLBufferInteger(GLenum target, GLenum e) {
		int i = 0;
		glGetBufferParameteriv(target, e, &i);
		return i;
	}

}
