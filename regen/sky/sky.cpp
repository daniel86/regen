/*
 * sky.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: daniel
 */

#include "sky.h"

#include <ctime>

#include <regen/external/osghimmel/astronomy.h>
#include <regen/external/osghimmel/earth.h>
#include <regen/external/osghimmel/moon.h>
#include <regen/states/blend-state.h>
#include <regen/states/state-configurer.h>

using namespace regen;

Sky::Sky(
    const ref_ptr<Camera> &cam,
    const ref_ptr<ShaderInput2i> &viewport)
: StateNode(),
  Animation(GL_TRUE,GL_TRUE),
  cam_(cam),
  viewport_(viewport)
{
  timef_ = ref_ptr<osgHimmel::TimeF>::alloc(
      time(NULL),       // Current time
      -3600.0L * 2.0L,  // UTC offset
      3600.0L           // secondsPerCycle
  );
  timef_->start();

  astro_ = ref_ptr<osgHimmel::Astronomy>::alloc();
  astro_->setLatitude(52.5491);
  astro_->setLongitude(13.3611);

  timeUniform_ = ref_ptr<ShaderInput1f>::alloc("time");
  timeUniform_->setUniformData(0.0f);
  state()->joinShaderInput(timeUniform_);

  cmnUniform_ = ref_ptr<ShaderInput4f>::alloc("cmn");
  cmnUniform_->setUniformData(Vec4f(
      0.043,
      osgHimmel::Earth::meanRadius(),
      osgHimmel::Earth::meanRadius() + osgHimmel::Earth::atmosphereThicknessNonUniform(),
      0));
  state()->joinShaderInput(cmnUniform_);

  // directional light that approximates the sun
  sun_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
  sun_->set_isAttenuated(GL_FALSE);
  sun_->specular()->setVertex(0,Vec3f(0.0f));
  sun_->diffuse()->setVertex(0,Vec3f(0.0f));
  sun_->direction()->setVertex(0,Vec3f(1.0f));

  sunPosition_ = ref_ptr<ShaderInput3f>::alloc("sunPosition");
  sunPosition_->setUniformData(Vec3f(0.0f));
  state()->joinShaderInput(sunPosition_);

  sunPositionR_ = ref_ptr<ShaderInput3f>::alloc("sunPositionR");
  sunPositionR_->setUniformData(Vec3f(0.0f));
  state()->joinShaderInput(sunPositionR_);
}

std::string Sky::date() const {
  const time_t t = timef_->gett();
  string dateStr(ctime(&t));
  return dateStr.substr(0, dateStr.size()-1);
}

GLdouble Sky::timef() const
{ return timef_->getf(); }

void Sky::set_time(const time_t &time)
{
  timef_->sett(time, true);
  REGEN_INFO("Set sky time to: " << date());
}

void Sky::set_timestamp(double seconds)
{ set_time(static_cast<time_t>(seconds)); }

void Sky::set_date(const string &date) {
  struct tm tm;
  if (strptime(date.c_str(), "%d-%m-%Y %H:%M:%S", &tm)) {
    set_time(mktime(&tm));
    animate(0.0);
  }
  else {
    REGEN_WARN("Invalid date string: " << date << ".");
  }
}

void Sky::set_utcOffset(const time_t &offset)
{ timef_->setUtcOffset(offset); }

void Sky::set_secondsPerCycle(GLdouble secondsPerCycle)
{ timef_->setSecondsPerCycle(secondsPerCycle); }

osgHimmel::AbstractAstronomy& Sky::astro()
{ return *astro_.get(); }
void Sky::set_astro(const ref_ptr<osgHimmel::AbstractAstronomy> &astro)
{ astro_ = astro; }

GLdouble Sky::altitude() const
{ return cmnUniform_->getVertex(0).x; }
GLdouble Sky::longitude() const
{ return astro_->getLongitude(); }
GLdouble Sky::latitude() const
{ return astro_->getLatitude(); }

void Sky::set_altitude(const GLdouble altitude)
{
  Vec4f &v = *((Vec4f*)cmnUniform_->clientDataPtr());
  // Clamp altitude into non uniform atmosphere. (min alt is 1m)
  v.x = math::clamp(altitude, 0.001f, osgHimmel::Earth::atmosphereThicknessNonUniform());
  cmnUniform_->nextStamp();
}
void Sky::set_longitude(const GLdouble longitude)
{ astro_->setLongitude(longitude); }
void Sky::set_latitude(const GLdouble latitude)
{ astro_->setLatitude(latitude); }

ref_ptr<Light>& Sky::sun()
{ return sun_; }

void Sky::updateSeed() {
  Vec4f &v = *((Vec4f*)cmnUniform_->clientDataPtr());
  v.w = rand();
  cmnUniform_->nextStamp();
}

void Sky::addLayer(const ref_ptr<SkyLayer> &layer, BlendMode blendMode) {
  //ref_ptr<State> x = ref_ptr<State>::alloc();
  //x->joinStates(ref_ptr<BlendState>::alloc(blendMode));
  //x->joinStates(layerState);
  addChild(layer);
  this->layer_.push_back(layer);
}

void Sky::createShader(RenderState *rs, const StateConfig &stateCfg) {
  for(std::list< ref_ptr<SkyLayer> >::iterator
      it=layer_.begin(); it!=layer_.end(); ++it) {
    ref_ptr<SkyLayer> layer = *it;

    StateConfigurer cfg(stateCfg);
    cfg.addNode(layer.get());

    layer->getShaderState()->createShader(cfg.cfg());
    layer->getMeshState()->updateVAO(RenderState::get(), cfg.cfg(),
        layer->getShaderState()->shaderState()->shader());
  }
}

void Sky::animate(GLdouble dt) {
  // TODO: start/stop with animation
  if(!timef_->isRunning())
    timef_->start(false);
  timef_->update();
  //lastElapsed_ = timef_->getNonModf();
  astro_->update(osgHimmel::t_aTime::fromTimeF(*timef_.get()));
}

void Sky::glAnimate(RenderState *rs, GLdouble dt) {
  // Update uniforms
  sunPosition_->setVertex(0, astro_->getSunPosition(false));
  sunPositionR_->setVertex(0, astro_->getSunPosition(true));
  timeUniform_->setVertex(0, timef_->getf());
  sun_->direction()->setVertex(0,sunPosition_->getVertex(0));
  // Update random number in cmn uniform
  updateSeed();

  for(std::list< ref_ptr<SkyLayer> >::iterator
      it=layer_.begin(); it!=layer_.end(); ++it) {
    (*it)->updateSky(rs, dt);
  }
  REGEN_INFO("Sky time: " << date());
  //REGEN_INFO("Sun direction: " << sun_->direction()->getVertex(0));
}

