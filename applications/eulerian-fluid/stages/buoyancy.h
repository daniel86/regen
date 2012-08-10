
#ifndef EULERIAN_BOUYANCY_H_
#define EULERIAN_BOUYANCY_H_

#include "primitive.h"
#include "helper.h"

/**
 * Temperature is an important factor in the flow of many fluids.
 * Convection currents are caused by the changes in density associated with temperature changes.
 * These currents affect our weather, our oceans and lakes, and even our coffee.
 * To simulate these effects, we need to add buoyancy to our simulation.
 */
class EulerianBuoyancy : public EulerianStage
{
public:
  EulerianBuoyancy(EulerianPrimitive*);

  void set_velocityBuffer(const FluidBuffer &buffer);
  void set_temperatureTexture(ref_ptr<Texture> tex);

  float ambientTemperature() const {
      return ambTemp_->value();
  }
  void set_ambientTemperature(float ambientTemperature) {
      this->ambTemp_->set_value( ambientTemperature );
  }

  float smokeBuoyancy() const {
      return buoyancy_->value();
  }
  void set_smokeBuoyancy(float smokeBuoyancy) {
      this->buoyancy_->set_value( smokeBuoyancy );
  }

  virtual void update();

protected:
  ref_ptr<Shader> buoyancyShader_;
  ref_ptr<UniformFloat> ambTemp_;
  ref_ptr<UniformFloat> buoyancy_;
  ref_ptr<Uniform> direction_;

  FluidBuffer velocityBuffer_;
  ref_ptr<Texture> temperatureTexture_;
};

#endif /* EULERIAN_BOUYANCY_H_ */
