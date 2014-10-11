/*
 * moon.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "moon.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>
#include <regen/textures/texture-loader.h>

using namespace regen;


MoonLayer::MoonLayer(const ref_ptr<Sky> &sky, const string &moonMapFile)
: SkyLayer(sky)
{
  setupMoonTextureCube(moonMapFile);
  setupEclipseTexture();

  eclParams_ = ref_ptr<ShaderInput4f>::alloc("eclParams");
  eclParams_->setUniformData(Vec4f(0.f, 0.f, 0.f, -1.f));
  state()->joinShaderInput(eclParams_);

  moonOrientation_ = ref_ptr<ShaderInputMat4>::alloc("moonOrientationMatrix");
  moonOrientation_->setUniformData(Mat4f::identity());
  state()->joinShaderInput(moonOrientation_);

  scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
  scale_->setUniformData(defaultScale());
  state()->joinShaderInput(scale_);

  sunShine_ = ref_ptr<ShaderInput4f>::alloc("sunShine");
  sunShine_->setUniformData(Vec4f(defaultSunShineColor(),defaultSunShineIntensity()));
  state()->joinShaderInput(sunShine_);

  earthShine_ = ref_ptr<ShaderInput3f>::alloc("earthShine");
  earthShine_->setUniformData(Vec3f(0.0));
  state()->joinShaderInput(earthShine_);
  earthShineColor_ = defaultEarthShineColor();
  earthShineIntensity_ = defaultEarthShineIntensity();

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.moon");
  //state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  //state()->joinStates(meshState_);
}

void MoonLayer::setupMoonTextureCube(const string &moonMapFile)
{
    ref_ptr<TextureCube> texture = textures::loadCube(moonMapFile);
    state()->joinStates(ref_ptr<TextureState>::alloc(texture, "moonmapCube"));
}

void MoonLayer::setupEclipseTexture()
{
  // generate lunar eclipse 2d-texture
  const int sizeS = 128;
  float *map = new float[sizeS * 3];
  const float s_u = 0.05;

  const Vec3f le0 = Vec3f(1.0, 1.0, 1.0) * 0.900f;
  const Vec3f le1 = Vec3f(1.0, 1.0, 1.0) * 0.088f;
  const Vec3f le2 = Vec3f(0.4, 0.7, 1.0) * 0.023f;
  const Vec3f le3 = Vec3f(0.3, 0.5, 1.0) * 0.040f;

  for(int s = 0; s < sizeS; ++s) {
    const float fs = static_cast<float>(s) / sizeS;
    const unsigned int i = s * 3;
    Vec3f l(1.0,1.0,1.0);
    // remove the penumbral soft shadow from the moons coloring
    l -= le0 * (1.0 - math::clamp(2 * fs - 1, 0.0, 1.0));
    // remove the umbral hard shadow from the moons coloring
    l -= le1 * (1 - math::smoothstep(fs, 0.5 * (1.0 - s_u), 0.5 * (1.0 + s_u)));
    // add reddish darkening towards umbral center from atmosphere scattering, linear scaling within the umbral distance of e1
    l -= le2 * (fs < 0.5 ? math::clamp(- 2.0 * fs + 1, 0, 1) : 0);
    // account for blue scattered light visible at the outer edge of the umbral shadow
    l += le3 * (math::smoothstep(fs, 0.5 * (1 - 4 * s_u), 0.5 * (1 + s_u))) *
                  (1 - math::smoothstep(fs, 0.5, 1.0));

    map[i + 0] = l.x < 0 ? 0 : l.x;
    map[i + 1] = l.y < 0 ? 0 : l.y;
    map[i + 2] = l.z < 0 ? 0 : l.z;
  }

  eclTex_ = ref_ptr<Texture1D>::alloc();
  eclTex_->set_rectangleSize(sizeS,1);
  eclTex_->set_format(GL_RGB);
  eclTex_->set_internalFormat(GL_RGB32F_ARB);
  eclTex_->set_pixelType(GL_FLOAT);
  eclTex_->set_data(map);
  eclTex_->begin(RenderState::get());
  eclTex_->texImage();
  eclTex_->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR));
  eclTex_->setupMipmaps();
  eclTex_->wrapping().push(GL_CLAMP_TO_EDGE);
  eclTex_->end(RenderState::get());
  eclTex_->set_data(NULL);

  state()->joinStates(ref_ptr<TextureState>::alloc(eclTex_, "eclCoeffs"));
}


GLdouble MoonLayer::defaultScale()
{ return 2.0; }

Vec3f MoonLayer::defaultSunShineColor()
{ return Vec3f(0.923, 0.786, 0.636); }

GLdouble MoonLayer::defaultSunShineIntensity()
{ return 128.0; }

Vec3f MoonLayer::defaultEarthShineColor()
{ return Vec3f(0.88, 0.96, 1.00); }

GLdouble MoonLayer::defaultEarthShineIntensity()
{ return 4.0; }


void MoonLayer::set_scale(GLdouble scale)
{ scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput1f>& MoonLayer::scale() const
{ return scale_; }

const ref_ptr<ShaderInput4f>& MoonLayer::sunShine() const
{ return sunShine_; }

void MoonLayer::set_sunShineColor(const Vec3f &color)
{
  GLfloat intensity = sunShine_->getVertexPtr(0).w;
  sunShine_->setVertex(0, Vec4f(color,intensity));
}

void MoonLayer::set_sunShineIntensity(GLdouble intensity)
{
  Vec3f &color = sunShine_->getVertexPtr(0).xyz_();
  sunShine_->setVertex(0, Vec4f(color,intensity));
}

const ref_ptr<ShaderInput3f>& MoonLayer::earthShine() const
{ return earthShine_; }

void MoonLayer::set_earthShineColor(const Vec3f &color)
{ earthShineColor_ = color; }

void MoonLayer::set_earthShineIntensity(GLdouble intensity)
{ earthShineIntensity_ = intensity; }

ref_ptr<Mesh> MoonLayer::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> MoonLayer::getShaderState()
{ return shaderState_; }

void MoonLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
  const Vec3f moonv = sky_->astro().getMoonPosition(false);
  const Vec3f sunv = sky_->astro().getSunPosition(false);

  moonOrientation_->setVertex(0, sky_->astro().getMoonOrientation());

  earthShine_->setVertex(0, earthShineColor_ *
     (sky_->astro().getEarthShineIntensity() * earthShineIntensity_));

  // approximate umbra and penumbra size in moon radii
  float e0 = 0, e1 = 0, e2 = 0;
  // 0 hints that eclipse is not happening
  // This allows skipping of the 1d eclipse texture look-up
  // as well as all other eclipse related calcs.
  float B = 0.f;
  if(acos(sunv.dot(moonv)) > M_PI_2) {
    const float dm  = sky_->astro().getMoonDistance();
    const float ds  = sky_->astro().getSunDistance();
    const float ids = 1.f / ds;

    // scale for the normalized earth-moon system
    e0  = sky_->astro().getMoonRadius() / dm;
    e1  = 3.6676 - (397.0001 * dm) * ids;
    e2  = 3.6676 + (404.3354 * dm) * ids;

    // shortest, normalized distance between earth and moon-sun line
    const float Dm = moonv.cross(sunv).length() / e0;
    if(Dm < (e2 + 1.f)) {
      // transform the distance from 0 to e2 + 1 into [0,1] range for simpler brightness term
      const float t = Dm < (e1 + 1) ?
          Dm / (2.f + 2.f * e1) :
          0.5 + (Dm - e1 - 1.f) / (2.f * (e2 - e1));
      // brightness scalar of the eclipse phenomena as function of the distance
      B = 1.0 + 29 * (1.0 - math::smoothstep(t, 0.2, 0.44));
    }
  }
  // encode in uniform
  eclParams_->setVertex(0, Vec4f(e0, e1, e2, B));
}

