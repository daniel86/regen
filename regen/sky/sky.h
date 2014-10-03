/*
 * sky.h
 *
 *  Created on: Oct 3, 2014
 *      Author: daniel
 */

#ifndef SKY_H_
#define SKY_H_

#include <list>

#include <regen/camera/camera.h>
#include <regen/states/light-state.h>
#include <regen/sky/sky-layer.h>
#include <regen/states/blend-state.h>

#include <regen/external/osghimmel/timef.h>
#include <regen/external/osghimmel/astronomy.h>

namespace regen {

  class Sky : public StateNode, public Animation
  {
  public:
    Sky(const ref_ptr<Camera> &cam,
        const ref_ptr<ShaderInput2i> &viewport);



    void set_time(const time_t &time);

    void set_timestamp(GLdouble timestamp);

    void set_date(const string &date);

    void set_utcOffset(const time_t &offset);

    void set_secondsPerCycle(GLdouble secondsPerCycle);

    std::string date() const;

    /**
     * Float time in the interval [0;1]
     */
    GLdouble timef() const;


    void set_altitude(const GLdouble altitude);

    GLdouble altitude() const;

    void set_longitude(const GLdouble longitude);

    GLdouble longitude() const;

    void set_latitude(const GLdouble latitude);

    GLdouble latitude() const;

    ref_ptr<Light>& sun();

    osgHimmel::AbstractAstronomy& astro();

    void set_astro(const ref_ptr<osgHimmel::AbstractAstronomy> &astro);

    void addLayer(const ref_ptr<SkyLayer> &layer, BlendMode blendMode);

    void createShader(RenderState *rs, const StateConfig &stateCfg);

    // override
    void animate(GLdouble dt);
    void glAnimate(RenderState *rs, GLdouble dt);

  protected:
    ref_ptr<Camera> cam_;
    ref_ptr<ShaderInput2i> viewport_;

    ref_ptr<osgHimmel::TimeF> timef_;
    ref_ptr<osgHimmel::AbstractAstronomy> astro_;

    std::list< ref_ptr<SkyLayer> > layer_;

    ref_ptr<Light> sun_;
    ref_ptr<ShaderInput3f> sunPosition_;
    ref_ptr<ShaderInput3f> sunPositionR_;
    ref_ptr<ShaderInput1f> timeUniform_;
    ref_ptr<ShaderInput4f> cmnUniform_;


    void updateSeed();
  };
}

#endif /* SKY_H_ */
