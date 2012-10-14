/*
 * fluid-test.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "fluid-parser.h"
#include "fluid-animation.h"

#include <boost/filesystem.hpp>

#include <ogle/utility/string-util.h>
#include <ogle/external/glsw/glsw.h>
#include <ogle/external/rapidxml/rapidxml.hpp>
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/quad.h>
#include <ogle/textures/image-texture.h>
#include <ogle/animations/animation-manager.h>
#include <applications/glut-render-tree.h>

class FluidApp;
class ReloadFluidTimeout : public Animation
{
public:
  FluidApp *app_;
  std::time_t lastModified_;
  GLdouble dt_;

  ReloadFluidTimeout(FluidApp *app);
  virtual void animate(GLdouble dt);
  virtual void updateGraphics(GLdouble dt);
};

class FluidApp : public GlutRenderTree
{
public:
  FluidApp(
      int argc, char** argv,
      const string &file)
  : GlutRenderTree(
      argc, argv,
      "Fluid simulation",
      600,600,
      GLUT_RGB,
      ref_ptr<RenderTree>(),
      ref_ptr<RenderState>(),
      GL_FALSE
      ),
    file_(FORMAT_STRING("applications/eulerian-fluid/res/" << file)),
    fluid_(NULL)
  {
    ref_ptr<FBOState> fboState = setRenderToTexture(
        1.0f,1.0f,
        GL_RGBA,
        GL_NONE,
        GL_FALSE,
        GL_FALSE,
        Vec4f(0.0f)
    );

    glswSetPath("applications/eulerian-fluid/shader/", ".glsl");

    fluid_ = FluidParser::readFluidFileXML(orthoQuad_.get(), file_);
    FluidBuffer *fluidBuffer = fluid_->getBuffer("fluid");
    fluidTexture_ = fluidBuffer->fluidTexture();
    fluidTexture_->addMapTo(MAP_TO_COLOR);

    {
      UnitQuad::Config quadConfig;
      quadConfig.levelOfDetail = 0;
      quadConfig.isTexcoRequired = GL_TRUE;
      quadConfig.isNormalRequired = GL_FALSE;
      quadConfig.centerAtOrigin = GL_TRUE;
      quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 1.0*M_PI);
      quadConfig.posScale = Vec3f(3.32f, 3.32f, 3.32f);
      ref_ptr<MeshState> quad =
          ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

      ref_ptr<ModelTransformationState> modelMat;
      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->translate(Vec3f(0.0f, 1.0f, 0.0f), 0.0f);
      modelMat->setConstantUniforms(GL_TRUE);

      material_ = ref_ptr<Material>::manage(new Material);
      material_->set_shading( Material::NO_SHADING );
      material_->addTexture(fluidTexture_);
      material_->setConstantUniforms(GL_TRUE);

      addMesh(quad, modelMat, material_);
    }

    // at this point the ortho quad geometry was not added to a vbo yet.
    // Animations may want to use this quad for updating textures
    // without a post pass added.
    // for this reason we add a hidden node that is skipped during traversal
    addDummyOrthoPass();

    // TODO: show fluid buffers....
    //   * render on the fly again.
    //   * an app where fluids can be modified live would be nice

    setShowFPS();
    setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
    render(0.0f);

    fluid_->executeOperations(fluid_->initialOperations());
    fluidAnim_ = ref_ptr<FluidAnimation>::manage(new FluidAnimation(fluid_));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluidAnim_));

    reloadTimeout_ = ref_ptr<Animation>::manage(new ReloadFluidTimeout(this));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(reloadTimeout_));
  }
  ~FluidApp()
  {
    if(fluid_) delete fluid_;
  }
  void loadFluid()
  {
    if(fluid_) {
      delete fluid_;
      fluid_ = NULL;
    }
    if(fluidTexture_.get()) {
      material_->removeTexture(fluidTexture_.get());
      fluidTexture_ = ref_ptr<Texture>();
    }
    if(fluidAnim_.get()) {
      AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(fluidAnim_));
      fluidAnim_ = ref_ptr<FluidAnimation>();
    }

    fluid_ = FluidParser::readFluidFileXML(orthoQuad_.get(), file_);
    FluidBuffer *fluidBuffer = fluid_->getBuffer("fluid");

    fluidTexture_ = fluidBuffer->fluidTexture();
    fluidTexture_->addMapTo(MAP_TO_COLOR);
    material_->addTexture(fluidTexture_);

    fluid_->executeOperations(fluid_->initialOperations());

    fluidAnim_ = ref_ptr<FluidAnimation>::manage(new FluidAnimation(fluid_));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluidAnim_));
  }
  ref_ptr<Animation> reloadTimeout_;
  Fluid *fluid_;
  ref_ptr<FluidAnimation> fluidAnim_;
  ref_ptr<Texture> fluidTexture_;
  ref_ptr<Material> material_;
  string file_;
};

ReloadFluidTimeout::ReloadFluidTimeout(FluidApp *app)
: Animation(), app_(app)
{
  lastModified_ = boost::filesystem::last_write_time(app_->file_) ;
  dt_ = 0.0;
}
void ReloadFluidTimeout::updateGraphics(GLdouble dt)
{
  dt_ += dt;
  if(dt_ > 1000.0) {
    std::time_t mod =
        boost::filesystem::last_write_time(app_->file_) ;
    if(mod > lastModified_) {
      lastModified_ = mod;
      try {
        app_->loadFluid();
      } catch (rapidxml::parse_error e) {
        ERROR_LOG("parsing the fluid file failed. " << e.what())
      }
    };
  }
}
void ReloadFluidTimeout::animate(GLdouble dt)
{
}

int main(int argc, char** argv)
{
  //string fluidFile = "dummy.xml";
  //string fluidFile = "fluid-test.xml";
  string fluidFile = "smoke-test.xml";
  //string fluidFile = "fire-test.xml";
  //string fluidFile = "liquid-test.xml";

  FluidApp *application = new FluidApp(argc, argv, fluidFile);

  application->mainLoop();

  return 0;
}
