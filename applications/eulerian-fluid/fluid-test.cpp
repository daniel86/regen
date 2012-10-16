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


#include <applications/application-config.h>
#ifdef USE_QT_FLUID_APPLICATION
  #include <applications/qt-ogle-application.h>
  #include <QtGui/QLabel>
  #include <QtGui/QApplication>
  #include <QtGui/QMenu>
  #include <QtGui/QMenuBar>
  #include <QtGui/QMainWindow>
  #include <QtGui/QStatusBar>
  #include <QtGui/QVBoxLayout>
  #include <QtGui/QPlainTextEdit>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

static Fluid *fluid_;

static string file_ = "";
std::time_t lastModified_;

static ref_ptr<Animation> reloadTimeout_;
static ref_ptr<FluidAnimation> fluidAnim_;
static ref_ptr<Texture> fluidTexture_;
static ref_ptr<Material> material_;
static TestRenderTree *renderTree;

static void loadFluid(bool executeInitialOperations)
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

  fluid_ = FluidParser::readFluidFileXML(renderTree->orthoQuad().get(), file_);
  FluidBuffer *fluidBuffer = fluid_->getBuffer("fluid");

  fluidTexture_ = fluidBuffer->fluidTexture();
  fluidTexture_->addMapTo(MAP_TO_COLOR);
  material_->addTexture(fluidTexture_);

  if(executeInitialOperations) {
    fluid_->executeOperations(fluid_->initialOperations());
  }

  fluidAnim_ = ref_ptr<FluidAnimation>::manage(new FluidAnimation(fluid_));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluidAnim_));
}

class ReloadFluidTimeout : public Animation
{
public:
  GLdouble dt_;

  ReloadFluidTimeout()
  : Animation()
  {
    dt_ = 0.0;
  }
  virtual void animate(GLdouble dt) {}

  virtual void updateGraphics(GLdouble dt)
  {
    dt_ += dt;
    if(dt_ > 1000.0) {
      std::time_t mod = boost::filesystem::last_write_time(file_) ;
      if(mod > lastModified_) {
        lastModified_ = mod;
        try {
          loadFluid(true);
        } catch (rapidxml::parse_error e) {
          ERROR_LOG("parsing the fluid file failed. " << e.what())
        }
      };
    }
  }
};

int main(int argc, char** argv)
{
  renderTree = new TestRenderTree;

  string fluidPath = "applications/eulerian-fluid/res";
  //string fluidFile = "dummy.xml";
  //string fluidFile = "fluid-test.xml";
  //string fluidFile = "smoke-test.xml";
  //string fluidFile = "fire-test.xml";
  //string fluidFile = "rgb-fluid-test.xml";
  string fluidFile = "liquid-test.xml";
  file_ = FORMAT_STRING(fluidPath << "/" << fluidFile);
  lastModified_ = boost::filesystem::last_write_time(file_) ;

#ifdef USE_QT_FLUID_APPLICATION
  OGLEQtApplication *application = new OGLEQtApplication(
      renderTree, argc, argv, OGLEQtApplication::getDefaultFormat(), 600, 600);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv, 600, 600);
#endif
  application->set_windowTitle("Fluid simulation");
  application->show();

  /*

  QMainWindow win(NULL);

  QPlainTextEdit edit;
  edit.setReadOnly(false);

  QTextDocument doc;
  string foo = FORMAT_STRING("applications/eulerian-fluid/res/" << fluidFile);
  QString arrr = QString(foo.c_str());
  QFile fileBuf( arrr );
  if (fileBuf.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QString text = fileBuf.readAll();
    doc.setPlainText(text);
  }
  fileBuf.close();

  QPlainTextDocumentLayout *layout = new QPlainTextDocumentLayout(&doc);
  doc.setDocumentLayout(layout);
  edit.setDocument(&doc);

  edit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  edit.show();

  */


  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_NONE,
      GL_FALSE,
      GL_FALSE,
      Vec4f(1.0f)
  );

  glswSetPath("applications/eulerian-fluid/shader/", ".glsl");

  fluid_ = FluidParser::readFluidFileXML(renderTree->orthoQuad().get(), file_);
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

    renderTree->addMesh(quad, modelMat, material_);
  }

  // at this point the ortho quad geometry was not added to a vbo yet.
  // Animations may want to use this quad for updating textures
  // without a post pass added.
  // for this reason we add a hidden node that is skipped during traversal
  renderTree->addDummyOrthoPass();

  //FXAA::Config aaCfg;
  //addAntiAliasingPass(aaCfg);

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

  //setShowFPS();
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  application->drawGL();
  //application->glWidget().updateGL();
  //TestApplication::widget.render(0.0f);

  fluid_->executeOperations(fluid_->initialOperations());
  fluidAnim_ = ref_ptr<FluidAnimation>::manage(new FluidAnimation(fluid_));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluidAnim_));

  reloadTimeout_ = ref_ptr<Animation>::manage(new ReloadFluidTimeout);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(reloadTimeout_));

  /*

  win.setCentralWidget(&widget);

  QPixmap newpix("new.png");
  QPixmap openpix("open.png");
  QPixmap quitpix("quit.png");

  QAction *newa = new QAction(newpix, "&New", &win);
  QAction *open = new QAction(openpix, "&Open", &win);
  QAction *quit = new QAction(quitpix, "&Quit", &win);
  //quit->setShortcut(tr("CTRL+Q"));

  QMenu *file;
  file = win.menuBar()->addMenu("&File");
  file->addAction(newa);
  file->addAction(open);
  file->addSeparator();
  file->addAction(quit);

  win.connect(quit, SIGNAL(win.triggered()), qApp, SLOT(win.quit()));

  win.statusBar()->show();

  win.show();
  win.resize(600, 600);
  */

  return application->mainLoop();
}
