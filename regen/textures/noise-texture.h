#ifndef NOISE_TEXTURE_H_
#define NOISE_TEXTURE_H_

#include <regen/textures/texture.h>
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
		double GetValue(double x, double y, double z) const;

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_perlin(int randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_wood(int randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_granite(int randomSeed);

		/**
		 * @param randomSeed the random seed.
		 */
		static ref_ptr<NoiseGenerator> preset_clouds(int randomSeed);

		/**
		 * Load noise generator from a scene input node.
		 */
		static ref_ptr<NoiseGenerator> load(LoadingContext &ctx, scene::SceneInputNode &input);

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
		explicit NoiseTexture(bool isSeamless) : isSeamless_(isSeamless) {}

		virtual ~NoiseTexture() = default;

		/**
		 * A scale factor applied when sampling the noise.
		 */
		void setNoiseScale(float scale) { noiseScale_ = scale; }

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
		bool isSeamless_;
		float noiseScale_ = 1.0f;
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
		NoiseTexture2D(uint32_t width, uint32_t height, bool isSeamless = false);

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
		NoiseTexture3D(uint32_t width, uint32_t height, uint32_t depth, bool isSeamless = false);

		~NoiseTexture3D() override = default;

		void updateNoise() override;
	};

} // namespace

#endif /* NOISE_TEXTURE_H_ */
