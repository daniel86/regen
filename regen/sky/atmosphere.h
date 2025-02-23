/*
 * atmosphere.h
 *
 *  Created on: Jan 4, 2014
 *      Author: daniel
 */

#ifndef REGEN_ATMOSPHERE_H_
#define REGEN_ATMOSPHERE_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/meshes/sky-box.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	/**
	 * Defines the look of the sky.
	 */
	struct AtmosphereProperties {
		/** nitrogen profile */
		Vec3f rayleigh;
		/** aerosol profile */
		Vec4f mie;
		/** sun-spotlight */
		GLfloat spot;
		/** scattering strength */
		GLfloat scatterStrength;
		/** Absorption color */
		Vec3f absorption;
	};

	/**
	 * \brief Atmospheric scattering, and some simple cloud rendering.
	 * A cube map is created on updated, and then rendered each frame using a sky cube.
	 * @see http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
	 * @see http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
	 * @see https://github.com/shff/opengl_sky
	 */
	class Atmosphere : public SkyLayer {
	public:
		/**
		 * @param sky the sky object.
		 * @param cubeMapSize the size of the cube map.
		 * @param useFloatBuffer use float buffer for cube map?
		 * @param levelOfDetail the level of detail of draw cube, in most cases 0 will do.
		 */
		explicit Atmosphere(
				const ref_ptr<Sky> &sky,
				unsigned int cubeMapSize = 512,
				bool useFloatBuffer = false,
				unsigned int levelOfDetail = 0);

		/**
		 * Sets given planet properties.
		 */
		void setProperties(AtmosphereProperties &p);

		/**
		 * Approximates planet properties for earth.
		 */
		void setEarth();

		/**
		 * Approximates planet properties for mars.
		 */
		void setMars();

		/**
		 * Approximates planet properties for uranus.
		 */
		void setUranus();

		/**
		 * Approximates planet properties for venus.
		 */
		void setVenus();

		/**
		 * Approximates planet properties for imaginary alien planet.
		 */
		void setAlien();

		/**
		 * Sets brightness for nitrogen profile
		 */
		void setRayleighBrightness(float v);

		/**
		 * Sets strength for nitrogen profile
		 */
		void setRayleighStrength(float v);

		/**
		 * Sets collect amount for nitrogen profile
		 */
		void setRayleighCollect(float v);

		/**
		 * rayleigh profile
		 */
		ref_ptr<ShaderInput3f> &rayleigh() { return rayleigh_; }

		/**
		 * Sets brightness for aerosol profile
		 */
		void setMieBrightness(float v);

		/**
		 * Sets strength for aerosol profile
		 */
		void setMieStrength(float v);

		/**
		 * Sets collect amount for aerosol profile
		 */
		void setMieCollect(float v);

		/**
		 * Sets distribution amount for aerosol profile
		 */
		void setMieDistribution(float v);

		/**
		 * aerosol profile
		 */
		ref_ptr<ShaderInput4f> &mie() { return mie_; }

		/**
		 * @param v the spot brightness.
		 */
		void setSpotBrightness(float v);

		/**
		 * @return the spot brightness.
		 */
		ref_ptr<ShaderInput1f> &spotBrightness() { return spotBrightness_; }

		/**
		 * @param v scattering strength.
		 */
		void setScatterStrength(float v);

		/**
		 * @return scattering strength.
		 */
		ref_ptr<ShaderInput1f> &scatterStrength() { return scatterStrength_; }

		/**
		 * @param color the absorbtion color.
		 */
		void setAbsorption(const Vec3f &color);

		/**
		 * @return the absorbtion color.
		 */
		ref_ptr<ShaderInput3f> &absorption() { return skyAbsorption_; }

		const ref_ptr<TextureCube> &cubeMap() const;

		// Override SkyLayer
		void updateSkyLayer(RenderState *rs, GLdouble dt) override;

		// Override SkyLayer
		void createUpdateShader() override;

		ref_ptr<Mesh> getMeshState() override { return drawState_; }

		ref_ptr<HasShader> getShaderState() override { return drawState_; }

	protected:
		ref_ptr<FBO> fbo_;

		ref_ptr<SkyBox> drawState_;
		ref_ptr<ShaderState> updateShader_;
		ref_ptr<Mesh> updateMesh_;

		ref_ptr<ShaderInput3f> rayleigh_;
		ref_ptr<ShaderInput4f> mie_;
		ref_ptr<ShaderInput1f> spotBrightness_;
		ref_ptr<ShaderInput1f> scatterStrength_;
		ref_ptr<ShaderInput3f> skyAbsorption_;
	};
}

#endif /* REGEN_ATMOSPHERE_H_ */
