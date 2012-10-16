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
#include <ogle/states/tesselation-state.h>

#include <applications/application-config.h>
#include <applications/glut-ogle-application.h>
#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class FluidScene
: public OGLEGlutApplication
{
public:

  FluidScene(TestRenderTree *renderTree, int &argc, char** argv)
: OGLEGlutApplication(renderTree, argc, argv, 600, 600),
  renderTree_(renderTree),
  fluidFile_("dummy.xml"),
  fluidPath_("applications/eulerian-fluid/res")
  {
    set_windowTitle("Fluid simulation");
  }

  virtual void initTree()
  {
    OGLEGlutApplication::initTree();

    glswSetPath("applications/eulerian-fluid/shader/", ".glsl");

    // load the fluid
    loadFluid(GL_FALSE);

    ref_ptr<FBOState> fboState = renderTree_->setRenderToTexture(
        1.0f,1.0f,
        GL_RGBA,
        GL_DEPTH_COMPONENT24,
        GL_TRUE,
        GL_TRUE,
        Vec4f(0.4f)
    );

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

      renderTree_->addMesh(quad, modelMat, material_);
    }

    // at this point the ortho quad geometry was not added to a vbo yet.
    // Animations may want to use this quad for updating textures
    // without a post pass added.
    // for this reason we add a hidden node that is skipped during traversal
    renderTree_->addDummyOrthoPass();

    //FXAA::Config aaCfg;
    //renderTree_->addAntiAliasingPass(aaCfg);

    // FIXME: must be done after ortho quad added to tree :/
    fluid_->executeOperations(fluid_->initialOperations());

    renderTree_->setShowFPS();
    renderTree_->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  }

  void set_fluidFile(const string &fileName)
  {
    fluidFile_ = fileName;
  }
  void set_fluidPath(const string &path)
  {
    fluidPath_ = path;
  }

  GLboolean loadFluid(GLboolean executeInitialOperations=GL_TRUE)
  {
    string fluidFile = FORMAT_STRING(fluidPath_ << "/" << fluidFile_);
    // TODO: validate fluid file existence!

    // try parsing the fluid
    Fluid *newFluid = NULL;
    try {
      newFluid = FluidParser::readFluidFileXML(renderTree_->orthoQuad().get(), fluidFile);
    }
    catch (rapidxml::parse_error e) {
      ERROR_LOG("parsing the fluid file failed. " << e.what());
      return GL_FALSE;
    }
    if(newFluid==NULL) {
      ERROR_LOG("parsing the fluid file failed.");
      return GL_FALSE;
    }

    if(fluidAnim_.get()) {
      AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(fluidAnim_));
    }
    if(reloadTimeout_.get()) {
      AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(reloadTimeout_));
    }
    if(fluidTexture_.get() && material_.get()) {
      material_->removeTexture(fluidTexture_.get());
    }

    if(fluid_) { delete fluid_; }
    fluid_ = newFluid;

    // add and remove fluid texture from scene object
    FluidBuffer *fluidBuffer = fluid_->getBuffer("fluid");
    fluidTexture_ = fluidBuffer->fluidTexture();
    fluidTexture_->addMapTo(MAP_TO_COLOR);
    if(material_.get()) {
      material_->addTexture(fluidTexture_);
    }

    // execute the initial operations
    if(executeInitialOperations) {
      fluid_->executeOperations(fluid_->initialOperations());
    }

    // finally remove old and add new animation for updating the fluid
    fluidAnim_ = ref_ptr<FluidAnimation>::manage(new FluidAnimation(fluid_));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluidAnim_));
    reloadTimeout_ = ref_ptr<Animation>::manage(new ReloadFluidTimeout(this,fluidFile));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(reloadTimeout_));

    return GL_TRUE;
  }

protected:
  string fluidFile_;
  string fluidPath_;

  Fluid *fluid_;

  TestRenderTree *renderTree_;

  ref_ptr<FluidAnimation> fluidAnim_;
  ref_ptr<Texture> fluidTexture_;
  ref_ptr<Material> material_;

  class ReloadFluidTimeout : public Animation
  {
  public:
    GLdouble dt_;
    std::time_t lastModified_;
    string file_;

    ReloadFluidTimeout(FluidScene *editor, const string &file)
    : Animation(),
      file_(file),
      editor_(editor)
    {
      dt_ = 0.0;
      lastModified_ = boost::filesystem::last_write_time(file_) ;
    }
    virtual void animate(GLdouble dt) {}

    virtual void updateGraphics(GLdouble dt)
    {
      dt_ += dt;
      if(dt_ > 1000.0) {
        std::time_t mod = boost::filesystem::last_write_time(file_) ;
        if(mod > lastModified_) {
          lastModified_ = mod;
          editor_->loadFluid();
        };
      }
    }
    FluidScene *editor_;
  };
  ref_ptr<Animation> reloadTimeout_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

  FluidScene *application = new FluidScene(renderTree, argc, argv);
  //application->set_fluidFile("dummy.xml");
  //application->set_fluidFile("fluid-test.xml");
  //application->set_fluidFile("smoke-test.xml");
  //application->set_fluidFile("fire-test.xml");
  //application->set_fluidFile("rgb-fluid-test.xml");
  application->set_fluidFile("liquid-test.xml");
  application->show();

  // TODO: allow loading 3D fluids
  // TODO: allow switching fluid file
  // TODO: allow to set the operation out to the screen fbo
  // TODO: allow moving obstacles
  // TODO: allow creating obstacles using the mouse
  // TODO: reload fluid if shader file changed
  // TODO: allow pressure solve with smaller texture size
  // TODO: allow creating new fluids
  // TODO: obstacle aliasing
  // TODO: obstacle fighting
  // TODO: fire and liquid
  // TODO: less glUniform calls

  return application->mainLoop();
}
