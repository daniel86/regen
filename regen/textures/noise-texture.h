/*
 * noise-texture.h
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#ifndef NOISE_TEXTURE_H_
#define NOISE_TEXTURE_H_

#include <regen/gl-types/texture.h>
#include <regen/utility/ref-ptr.h>
#include <regen/external/libnoise/src/noise/module/module.h>

namespace regen {
	/**
	 * A noise generator.
	 */
	class NoiseGenerator {
	public:
		/**
		 * @param name the name of the noise generator.
		 * @param handle the noise module handle.
		 */
		NoiseGenerator(std::string_view name, const ref_ptr<noise::module::Module> &handle);

		virtual ~NoiseGenerator() = default;

		/**
		 * @return the noise module handle.
		 */
		auto &handle() const { return handle_; }

		/**
		 * @return the name of the noise generator.
		 */
		auto &name() const { return name_; }

		/**
		 * @param source the noise generator.
		 */
		void addSource(const ref_ptr<NoiseGenerator> &source);

		/**
		 * @return the noise sources.
		 */
		auto &sources() const { return sources_; }

		/**
		 * @param source the noise generator.
		 */
		void removeSource(const ref_ptr<NoiseGenerator> &source);

		/**
		 * @param x the x coordinate.
		 * @param y the y coordinate.
		 * @param z the z coordinate.
		 * @return the noise value.
		 */
		GLdouble GetValue(GLdouble x, GLdouble y, GLdouble z) const;

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_perlin(GLint randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_wood(GLint randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_granite(GLint randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_clouds(GLint randomSeed);

	protected:
		std::string name_;
		ref_ptr<noise::module::Module> handle_;
		std::list<ref_ptr<NoiseGenerator>> sources_;
	};

	/**
	 * Base class for noise textures.
	 */
	class NoiseTexture {
	public:
		/**
		 * @param isSeamless true if the texture should be seamless.
		 */
		explicit NoiseTexture(GLboolean isSeamless) : isSeamless_(isSeamless) {}

		virtual ~NoiseTexture() = default;

		/**
		 * @param generator the noise generator.
		 */
		void setNoiseGenerator(const ref_ptr<NoiseGenerator> &generator);

		/**
		 * @return the noise generator.
		 */
		auto &noiseGenerator() const { return generator_; }

		virtual void updateNoise() = 0;

	protected:
		ref_ptr<NoiseGenerator> generator_;
		GLboolean isSeamless_;
	};

	/**
	 * 2D noise texture.
	 */
	class NoiseTexture2D : public Texture2D, public NoiseTexture {
	public:
		/**
		 * @param width the width of the texture.
		 * @param height the height of the texture.
		 * @param isSeamless true if the texture should be seamless.
		 */
		NoiseTexture2D(GLuint width, GLuint height, GLboolean isSeamless = GL_FALSE);

		~NoiseTexture2D() override = default;

		void updateNoise() override;
	};

	/**
	 * 3D noise texture.
	 */
	class NoiseTexture3D : public Texture3D, public NoiseTexture {
	public:
		/**
		 * @param width the width of the texture.
		 * @param height the height of the texture.
		 * @param depth the depth of the texture.
		 * @param isSeamless true if the texture should be seamless.
		 */
		NoiseTexture3D(GLuint width, GLuint height, GLuint depth, GLboolean isSeamless = GL_FALSE);

		~NoiseTexture3D() override = default;

		void updateNoise() override;
	};

} // namespace

#endif /* NOISE_TEXTURE_H_ */
