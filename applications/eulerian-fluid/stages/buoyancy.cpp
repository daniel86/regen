
#include "../include/buoyancy.h"
#include "../include/smoke.h"
#include "../include/helper.h"

class BuoyancyShader : public ShaderFunctions {
public:
  BuoyancyShader(bool is2D)
  : ShaderFunctions("fluidBuoyancy", makeShaderArgs()),
    is2D_(is2D)
  {
    addUniform( (GLSLUniform) { is2D_ ?
        "sampler2D" : "sampler3D", EulerianSmoke::TEMPERATURE } );
    addUniform( (GLSLUniform) { is2D_ ?
        "vec2" : "vec3", "direction" } );
    addUniform( (GLSLUniform) { is2D_ ?
        "vec2" : "vec3", "inverseGridSize" } );
    addUniform( (GLSLUniform) { "float", "ambientTemperature" } );
    addUniform( (GLSLUniform) { "float", "deltaT" } );
    addUniform( (GLSLUniform) { "float", "buoyancy" } );
  }
  virtual string code() const {
    stringstream s;
    if(is2D_) s << "void " << myName_ << "(out vec2 outCol)" << endl;
    else      s << "void " << myName_ << "(out vec3 outCol)" << endl;
    s << "{" << endl;
    if(is2D_) s << "     ivec2 pos = ivec2(gl_FragCoord.xy);" << endl;
    else      s << "     ivec3 pos = ivec3(vec3(gl_FragCoord.xy,f_layer));" << endl;
    s << endl;
    s << "     float temperature = texelFetch(" <<
        EulerianSmoke::TEMPERATURE << ", pos, 0).r;" << endl;
    s << "     " << endl;
    s << "     float deltaTemperature = temperature - ambientTemperature;" << endl;
    s << "     outCol = deltaT * deltaTemperature * buoyancy * direction;" << endl;
    s << "}" << endl;
    return s.str();
  }
  bool is2D_;
};

EulerianBuoyancy::EulerianBuoyancy(EulerianPrimitive *primitive)
: EulerianStage(primitive),
  temperatureTexture_(),
  buoyancy_( ref_ptr<UniformFloat>::manage( new UniformFloat("buoyancy") )),
  ambTemp_( ref_ptr<UniformFloat>::manage( new UniformFloat("ambientTemperature") ))
{
  set_ambientTemperature(0.0f);
  set_smokeBuoyancy(1.0f);
  if(primitive_->is2D()) {
    direction_ = ref_ptr<Uniform>::manage( new UniformVec2("direction") );
    ((UniformVec2*) direction_.get())->set_value((Vec2f) {0.0f, 1.0f});
  } else {
    direction_ = ref_ptr<Uniform>::manage( new UniformVec3("direction") );
    ((UniformVec3*) direction_.get())->set_value((Vec3f) {0.0f, 1.0f, 0.0f});
  }

  buoyancyShader_ = primitive_->makeShader(
      BuoyancyShader(primitive_->is2D()), primitive_->is2D() ? "vec2" : "vec3" );
  buoyancyShader_->addUniform( primitive_->timeStep() );
  buoyancyShader_->addUniform( primitive_->inverseSize().get());
  buoyancyShader_->addUniform( ambTemp_.get() );
  buoyancyShader_->addUniform( buoyancy_.get() );
  buoyancyShader_->addUniform( direction_.get() );
}

void EulerianBuoyancy::set_velocityBuffer(const FluidBuffer &buffer)
{
  velocityBuffer_ = buffer;
}
void EulerianBuoyancy::set_temperatureTexture(ref_ptr<Texture> tex)
{
  tex->set_name( EulerianSmoke::TEMPERATURE );
  temperatureTexture_ = tex;
  buoyancyShader_->addTexture( temperatureTexture_.get() );
}

void EulerianBuoyancy::update()
{
  primitive_->enableShader(buoyancyShader_);

  activateTexture(buoyancyShader_.get(), temperatureTexture_.get(), 1);

  // render to last updated texture
  swapBuffer(velocityBuffer_); {
    velocityBuffer_.fbo->bind();
    velocityBuffer_.fbo->drawBufferMRT();

    // use additive blending
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    primitive_->draw();
    glDisable(GL_BLEND);
  } swapBuffer(velocityBuffer_);
}
