#ifndef REGEN_LIGHTNING_BOLT_H
#define REGEN_LIGHTNING_BOLT_H

#include <regen/objects/mesh.h>
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

		struct SegmentData {
			// segment vertex data in pairs for start and end point of line segments.
			std::vector<Vec3f> pos_;
			std::vector<float> brightness_;
			std::vector<uint32_t> strikeIdx_;

			void clear() {
				pos_.clear();
				brightness_.clear();
				strikeIdx_.clear();
			}

			void push_back(
					const Vec3f &start,
					const Vec3f &end,
					uint32_t strikeIdx,
					float brightness = 1.0f);
		};

		// We use triple buffering for the segment data to avoid synchronization issues.
		// Two segments are used for building the L-system, while the third segment is used for rendering.
		// The rendering picks the last updated segment, leaving the other two segments for building the next frame
		// while rendering can use the last completed frame in parallel.
		SegmentData segments_[3];
		// The index of the segment currently being used for drawing.
		// It is atomically updated when a new segment is ready for drawing,
		// however it is not safe to update the draw index while a draw is in progress
		// as maybe the next update would start writing to the same segment before the draw is finished.
		std::atomic<uint32_t> drawIndex_;
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
		void cpuUpdate(double dt) override;

		// override
		void gpuUpdate(RenderState *, double dt) override;

	protected:
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput1f> brightness_;
		ref_ptr<ShaderInput1ui> strikeIdx_;

		ref_ptr<SSBO> strikeSSBO_;
		// per-strike parameters
		ref_ptr<ShaderInput1f> strikeAlpha_;
		ref_ptr<ShaderInput1f> strikeWidth_;

		bool isActive_ = false;

		unsigned int bufferSize_ = 0;

		std::vector<ref_ptr<LightningStrike>> strikes_;
	};
} // namespace

#endif /* REGEN_LIGHTNING_BOLT_H */
