#ifndef REGEN_LIGHTNING_BOLT_H
#define REGEN_LIGHTNING_BOLT_H

#include <regen/meshes/mesh-state.h>
#include <regen/shapes/bounds.h>

namespace regen {
	/**
	 * \brief A lightning bolt.
	 */
	class LightningBolt : public Mesh, public Animation {
	public:
		/**
		 * Vertex data configuration.
		 */
		struct Config {
			/** maximum number of segments in a branch of the bolt. */
			unsigned int maxSubDivisions_ = 100;
			/** maximum number of branches in the bolt. */
			unsigned int maxBranches_ = 10;

			Config();
		};

		/**
		 * @param cfg the mesh configuration.
		 */
		explicit LightningBolt(const Config &cfg);

		/**
		 * @param other Another Rectangle.
		 */
		explicit LightningBolt(const ref_ptr<LightningBolt> &other);

		/**
		 * @return true if there is an active bolt instance.
		 */
		bool isBoltActive() const;

		/**
		 * Sets the source position of the lightning bolt.
		 * @param from the source position.
		 */
		void setSourcePosition(const ref_ptr<ShaderInput3f> &from);

		/**
		 * Sets the target position of the lightning bolt.
		 * @param to the target position.
		 */
		void setTargetPosition(const ref_ptr<ShaderInput3f> &to);

		/**
		 * Sets the source position of the lightning bolt.
		 * @param from the source position.
		 */
		void setSourcePosition(const Vec3f &from);

		/**
		 * Sets the target position of the lightning bolt.
		 * @param to the target position.
		 */
		void setTargetPosition(const Vec3f &to);

		/**
		 * Sets the frequency of lightning strikes.
		 * If set to 0, the lightning bolt will strike only once when calling strike().
		 * Otherwise, the bolt will strike randomly with the given frequency.
		 * @param frequency The frequency of lightning strikes in Hz.
		 */
		void setFrequency(float base, float variance);

		/**
		 * Sets the lifetime of the lightning bolt.
		 * @param base the base lifetime.
		 * @param variance the variance of the lifetime, will be added to the base lifetime.
		 */
		void setLifetime(float base, float variance);

		/**
		 * Sets the maximum jitter offset of the bolt.
		 * @param maxOffset the maximum offset of the bolt.
		 */
		void setJitterOffset(float maxOffset);

		/**
		 * Sets the probability of a branch to be created.
		 * @param probability the probability of a branch to be created.
		 */
		void setBranchProbability(float probability);

		/**
		 * Sets the offset factor of a branch.
		 * @param offset the offset factor of a branch.
		 */
		void setBranchOffset(float offset);

		/**
		 * Sets the length of a branch relative to its parent segment.
		 * @param length the length of a branch [0,1]
		 */
		void setBranchLength(float length);

		/**
		 * Sets the darkening factor of a branch relative to their parent segment.
		 * @param darkening the darkening factor of a branch [0,1]
		 */
		void setBranchDarkening(float darkening);

		/**
		 * Strike the lightning bolt at this moment.
		 * This will update the vertex data to a new random bolt.
		 * The bolt will be visible for a short time, if it is not already.
		 * It will use its last source and target position.
		 */
		void strike();

		/**
		 * Strike the lightning bolt at this moment.
		 * This will update the vertex data to a new random bolt.
		 * The bolt will be visible for a short time, if it is not already.
		 */
		void strike(const Vec3f &source, const Vec3f &target);

		// override
		void glAnimate(RenderState *, GLdouble dt) override;

	protected:
		// configuration for the frequency of the bolt.
		// if>0 then the bolt automatically strikes at the given frequency.
		ref_ptr<ShaderInput2f> frequencyConfig_;
		// configuration for the lifetime of the bolt.
		ref_ptr<ShaderInput2f> lifetimeConfig_;
		ref_ptr<ShaderInput1f> jitterOffset_;
		ref_ptr<ShaderInput1f> branchProbability_;
		ref_ptr<ShaderInput1f> branchOffset_;
		ref_ptr<ShaderInput1f> branchLength_;
		ref_ptr<ShaderInput1f> branchDarkening_;
		ref_ptr<ShaderInput1f> matAlpha_;
		// vertex positions
		ref_ptr<ShaderInput3f> pos_;
		ref_ptr<ShaderInput1f> brightness_;
		// source and target position for the bolt.
		ref_ptr<ShaderInput3f> source_;
		ref_ptr<ShaderInput3f> target_;
		double u_lifetime_ = 0.0f;
		double u_lifetimeBegin_ = 0.0f;
		float u_nextStrike_ = 0.0f;
		bool hasFrequency_ = false;
		bool isActive_ = true;
		// the maximum number of segments in the main branch of the bolt.
		const unsigned int maxSubDivisions_;
		// the maximum number of sub-branches of the main branch.
		const unsigned int maxBranches_;

		unsigned int bufferOffset_ = 0;
		unsigned int bufferSize_ = 0;
		unsigned int elementSize_ = 0;

		struct Vertex {
			float brightness;
			Vec3f pos;

			explicit Vertex(const Vec3f &pos, float brightness = 1.0f)
					: brightness(brightness), pos(pos) {}
		};

		struct Segment {
			Vertex start;
			Vertex end;

			Segment(const Vec3f &start, const Vec3f &end, float brightness = 1.0f)
					: start(start, brightness), end(end, brightness) {}
		};

		std::vector<Segment> segments_[2];
		int segmentIndex_ = 0;

		void setNextStrike();

		void updateBolt(double dt_s);

		void updateVertexData();

		void updateSegmentData();
	};
} // namespace

#endif /* REGEN_LIGHTNING_BOLT_H */
