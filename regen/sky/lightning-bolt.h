#ifndef REGEN_LIGHTNING_BOLT_H
#define REGEN_LIGHTNING_BOLT_H

#include <regen/meshes/mesh-state.h>
#include <regen/shapes/bounds.h>

namespace regen {
	/**
	 * \brief A single lightning strike.
	 */
	class LightningStrike {
	public:
		struct StrikePoint {
			ref_ptr<ModelTransformation> tf;
			ref_ptr<ShaderInput3f> pos;
			uint32_t instance = 0u;
			uint32_t numInstances = 1u;
			bool randomizeInstance = false;
		};

		LightningStrike(
				uint32_t strikeIdx,
				const StrikePoint &source,
				const StrikePoint &target,
				const ref_ptr<ShaderInput1f> &alpha);

		LightningStrike(const LightningStrike &) = default;

		LightningStrike &operator=(const LightningStrike &) = default;

		/**
		 * Sets the frequency of lightning strikes.
		 * If set to 0, the lightning bolt will strike only once when calling strike().
		 * Otherwise, the bolt will strike randomly with the given frequency.
		 * @param frequency The frequency of lightning strikes in Hz.
		 */
		void setFrequency(float base, float variance) {
			frequencyConfig_ = Vec2f(base, variance);
			hasFrequency_ = (base > 0.0f);
		}

		/**
		 * Sets the lifetime of the lightning bolt.
		 * @param base the base lifetime.
		 * @param variance the variance of the lifetime, will be added to the base lifetime.
		 */
		void setLifetime(float base, float variance) { lifetimeConfig_ = Vec2f(base, variance); }

		/**
		 * Sets the maximum jitter offset of the bolt.
		 * @param maxOffset the maximum offset of the bolt.
		 */
		void setJitterOffset(float maxOffset) { jitterOffset_ = maxOffset; }

		/**
		 * Sets the probability of a branch to be created.
		 * @param probability the probability of a branch to be created.
		 */
		void setBranchProbability(float probability) { branchProbability_ = probability; }

		/**
		 * Sets the offset factor of a branch.
		 * @param offset the offset factor of a branch.
		 */
		void setBranchOffset(float offset) { branchOffset_ = offset; }

		/**
		 * Sets the length of a branch relative to its parent segment.
		 * @param length the length of a branch [0,1]
		 */
		void setBranchLength(float length) { branchLength_ = length; }

		/**
		 * Sets the darkening factor of a branch relative to their parent segment.
		 * @param darkening the darkening factor of a branch [0,1]
		 */
		void setBranchDarkening(float darkening) { branchDarkening_ = darkening; }

	protected:
		uint32_t strikeIdx_;
		StrikePoint source_;
		StrikePoint target_;
		// configuration for the frequency of the bolt.
		// if>0 then the bolt automatically strikes at the given frequency.
		Vec2f frequencyConfig_ = Vec2f(0.0f, 0.0f);
		bool hasFrequency_ = false;
		// configuration for the lifetime of the bolt.
		Vec2f lifetimeConfig_ = Vec2f(2.0f, 0.5f);
		float jitterOffset_ = 2.0f;
		float branchProbability_ = 0.5f;
		float branchOffset_ = 2.0f;
		float branchLength_ = 0.7f;
		float branchDarkening_ = 0.5f;
		float widthFactor_ = 1.0f;
		// the maximum number of segments in the main branch of the bolt.
		unsigned int maxSubDivisions_ = 7;
		// the maximum number of sub-branches of the main branch.
		unsigned int maxBranches_ = 10;

		// per-strike alpha parameter
		ref_ptr<ShaderInput1f> alpha_;

		struct Vertex {
			Vec3f pos;
			float brightness;
			uint32_t strikeIdx;

			explicit Vertex(const Vec3f &pos, uint32_t strikeIdx, float brightness = 1.0f)
					: pos(pos), brightness(brightness), strikeIdx(strikeIdx) {}
		};

		struct Segment {
			Vertex start;
			Vertex end;

			Segment(const Vec3f &start, const Vec3f &end, uint32_t strikeIdx, float brightness = 1.0f)
					: start(start, strikeIdx, brightness), end(end, strikeIdx, brightness) {}
		};

		std::vector<Segment> segments_[2];
		int segmentIndex_ = 0;
		double u_time_ = 0.0f;
		double u_maxLifetime_ = 0.0f;
		float u_nextStrike_ = 0.0f;
		bool active_ = false;

		void updateSegmentData();

		void updateSegmentData(const Vec3f &source, const Vec3f &target);

		bool updateStrike(double dt_s);

		friend class LightningBolt;
	};

	/**
	 * \brief A lightning bolt.
	 */
	class LightningBolt : public Mesh, public Animation {
	public:
		/**
		 * @param cfg the mesh configuration.
		 */
		LightningBolt();

		void load(LoadingContext &ctx, scene::SceneInputNode &input);

		void addLightningStrike(const ref_ptr<LightningStrike> &strike);

		void createResources();

		// override
		void glAnimate(RenderState *, GLdouble dt) override;

	protected:
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput1f> brightness_;
		ref_ptr<ShaderInput1ui> strikeIdx_;

		ref_ptr<SSBO> strikeSSBO_;
		// per-strike parameters
		ref_ptr<ShaderInput1f> strikeAlpha_;
		ref_ptr<ShaderInput1f> strikeWidth_;

		bool isActive_ = true;

		unsigned int bufferOffset_ = 0;
		unsigned int bufferSize_ = 0;
		unsigned int elementSize_ = 0;

		std::vector<ref_ptr<LightningStrike>> strikes_;

		void updateVertexData();
	};
} // namespace

#endif /* REGEN_LIGHTNING_BOLT_H */
