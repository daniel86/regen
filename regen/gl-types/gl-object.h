/*
 * gl-object.h
 *
 *  Created on: 09.12.2011
 *      Author: daniel
 */

#ifndef GL_OBJECT_H_
#define GL_OBJECT_H_

#include <regen/config.h>
#include <regen/gl-types/render-state.h>
#include <regen/utility/ref-ptr.h>

#include <GL/glew.h>

namespace regen {
	class ShaderInput2f;

	/**
	 * \brief Base class for buffer objects.
	 *
	 * Each buffer can generate multiple GL buffers,
	 * the active buffer can be changed with nextObject()
	 * and set_objectIndex().
	 */
	class GLObject {
	public:
		/**
		 * Obtain n buffers.
		 */
		typedef void (GLAPIENTRY *CreateObjectFunc)(GLsizei n, GLuint *buffers);

		/**
		 * Release n buffers.
		 */
		typedef void (GLAPIENTRY *ReleaseObjectFunc)(GLsizei n, const GLuint *buffers);

		/**
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLObject(
				CreateObjectFunc createObjects,
				ReleaseObjectFunc releaseObjects,
				GLuint numObjects = 1);

		~GLObject();

		/**
		 * Releases and allocates resources again.
		 */
		void resetGL();

		/**
		 * Switch to the next allocated buffer.
		 * Next bind() call will bind the activated buffer.
		 */
		void nextObject();

		/**
		 * Returns the currently active buffer index.
		 */
		GLuint objectIndex() const;

		/**
		 * Sets the index of the active buffer.
		 */
		void set_objectIndex(GLuint bufferIndex);

		/**
		 * Returns number of buffers allocation
		 * for this Bufferobject.
		 */
		GLuint numObjects() const;

		/**
		 * GL handle for currently active buffer.
		 */
		GLuint id() const;

		/**
		 * Array of GL handles allocated for this buffer.
		 */
		GLuint *ids() const;

	protected:
		GLuint *ids_;
		GLuint numObjects_;
		GLuint objectIndex_;
		ReleaseObjectFunc releaseObjects_;
		CreateObjectFunc createObjects_;

		/**
		 * copy not allowed.
		 */
		GLObject(const GLObject &other);

		GLObject &operator=(const GLObject &other) { return *this; }
	};

	/**
	 * \brief A 2D rectangular buffer.
	 */
	class GLRectangle : public GLObject {
	public:
		/**
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLRectangle(
				CreateObjectFunc createObjects,
				ReleaseObjectFunc releaseObjects,
				GLuint numObjects = 1);

		/**
		 * Set the buffer size.
		 */
		void set_rectangleSize(GLuint width, GLuint height);

		/**
		 * Width of the buffer.
		 */
		GLuint width() const;

		/**
		 * Height of the buffer.
		 */
		GLuint height() const;

		/**
		 * @return The inverse rectangle size.
		 */
		const ref_ptr<ShaderInput2f> &sizeInverse() const;

		/**
		 * @return The rectangle size.
		 */
		const ref_ptr<ShaderInput2f> &size() const;

	protected:
		ref_ptr<ShaderInput2f> size_;
		ref_ptr<ShaderInput2f> sizeInverse_;
	};
} // namespace

#endif /* BUFFER_OBJECT_H_ */
