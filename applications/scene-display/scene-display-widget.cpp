/*
 * scene-display-widget.cpp
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#include <regen/camera/camera-manipulator.h>
#include <regen/animations/animation-manager.h>
#include <regen/utility/filesystem.h>
#include <regen/meshes/texture-mapped-text.h>

//#include <regen/scene/scene-xml.h>
#include <regen/scene/scene-parser.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resources.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/scene-input-xml.h>
using namespace regen::scene;
using namespace std;

#include "scene-display-widget.h"

#define CONFIG_FILE_NAME ".regen-scene-display.cfg"

class ViewNodeProcessor : public NodeProcessor {
public:
  ViewNodeProcessor(ViewNodeList *viewNodes)
  : NodeProcessor("view"),
    viewNodes_(viewNodes)
  {}

  // Override
  void processInput(
      SceneParser *parser,
      SceneInputNode &input,
      const ref_ptr<StateNode> &parent)
  {
    ref_ptr<State> state = ref_ptr<State>::alloc();
    ref_ptr<StateNode> newNode = ref_ptr<StateNode>::alloc(state);
    newNode->set_name(input.getName());
    newNode->set_isHidden(GL_TRUE);
    parent->addChild(newNode);
    parser->putNode(input.getName(), newNode);
    handleChildren(parser,input,newNode);

    ViewNode viewNode;
    viewNode.name = input.getName();
    viewNode.node = newNode;
    viewNodes_->push_back(viewNode);
    REGEN_INFO("View: " << viewNode.name);
  }

protected:
  ViewNodeList *viewNodes_;

  void handleChildren(
      SceneParser *parser,
      SceneInputNode &input,
      const ref_ptr<StateNode> &newNode)
  {
    // Process node children
    const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
    for(list< ref_ptr<SceneInputNode> >::const_iterator
        it=childs.begin(); it!=childs.end(); ++it)
    {
      const ref_ptr<SceneInputNode> &x = *it;
      // First try node processor
      ref_ptr<NodeProcessor> nodeProcessor = parser->getNodeProcessor(x->getCategory());
      if(nodeProcessor.get()!=NULL) {
        nodeProcessor->processInput(parser, *x.get(), newNode);
        continue;
      }
      // Second try state processor
      ref_ptr<StateProcessor> stateProcessor = parser->getStateProcessor(x->getCategory());
      if(stateProcessor.get()!=NULL) {
        stateProcessor->processInput(parser, *x.get(), newNode->state());
        continue;
      }
      REGEN_WARN("No processor registered for '" << x->getDescription() << "'.");
    }
  }
};

/////////////////
////// Camera Event Handler
/////////////////

class EgoCamMotion : public EventHandler
{
public:
  EgoCamMotion(const ref_ptr<EgoCameraManipulator> &m, const GLboolean &buttonPressed)
  : EventHandler(),
    m_(m),
    buttonPressed_(buttonPressed),
    sensitivity_(0.0002f) {}

  void call(EventObject *evObject, EventData *data)
  {
    if(buttonPressed_) {
      Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
      Vec2f delta((float)ev->dx, (float)ev->dy);
      m_->lookLeft(delta.x*sensitivity_);
      m_->lookUp(delta.y*sensitivity_);
    }
  }

  ref_ptr<EgoCameraManipulator> m_;
  const GLboolean &buttonPressed_;
  GLfloat sensitivity_;
};

class EgoCamButton : public EventHandler
{
public:
  EgoCamButton(const ref_ptr<EgoCameraManipulator> &m)
  : EventHandler(), m_(m), buttonPressed_(GL_FALSE) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;
    if(ev->button == 0) buttonPressed_ = ev->pressed;
  }

  ref_ptr<EgoCameraManipulator> m_;
  GLboolean buttonPressed_;
};

class EgoCamKey : public EventHandler
{
public:
  EgoCamKey(const ref_ptr<EgoCameraManipulator> &m)
  : EventHandler(), m_(m) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::KeyEvent *ev = (Application::KeyEvent*)data;
    if(ev->key == Qt::Key_W || ev->key == Qt::Key_Up)         m_->moveForward(!ev->isUp);
    else if(ev->key == Qt::Key_S || ev->key == Qt::Key_Down)  m_->moveBackward(!ev->isUp);
    else if(ev->key == Qt::Key_A || ev->key == Qt::Key_Left)  m_->moveLeft(!ev->isUp);
    else if(ev->key == Qt::Key_D || ev->key == Qt::Key_Right) m_->moveRight(!ev->isUp);
  }

  ref_ptr<EgoCameraManipulator> m_;
};

class LookAtMotion : public EventHandler
{
public:
  LookAtMotion(
      const ref_ptr<LookAtCameraManipulator> &m,
      const GLboolean &buttonPressed)
  : EventHandler(),
    m_(m),
    buttonPressed_(buttonPressed),
    stepX_(0.1f),
    stepY_(0.1f) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
    if(buttonPressed_) {
      m_->set_height(m_->height() + ((float)ev->dy)*stepX_, ev->dt );
      m_->setStepLength( ((float)ev->dx)*stepY_, ev->dt );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  const GLboolean &buttonPressed_;
  GLfloat stepX_;
  GLfloat stepY_;
};

class LookAtButton : public EventHandler
{
public:
  LookAtButton(const ref_ptr<LookAtCameraManipulator> &m)
  : EventHandler(),
    m_(m),
    buttonPressed_(GL_FALSE),
    scrollStep_(0.1f){}

  void call(EventObject *evObject, EventData *data)
  {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;

    if(ev->button == 0) {
      buttonPressed_ = ev->pressed;
      if(ev->pressed) {
        m_->setStepLength( 0.0f );
      }
      } else if (ev->button == 4 && !ev->pressed) {
        m_->set_radius( m_->radius()+scrollStep_ );
      } else if (ev->button == 3 && !ev->pressed) {
        m_->set_radius( m_->radius()-scrollStep_ );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  GLboolean buttonPressed_;
  GLfloat scrollStep_;
};

/////////////////
////// FPS widget update
/////////////////

class UpdateFPS : public Animation
{
public:
  UpdateFPS(const ref_ptr<TextureMappedText> &widget)
  : Animation(GL_TRUE,GL_FALSE),
    widget_(widget),
    frameCounter_(0),
    fps_(0),
    sumDtMiliseconds_(0.0f)
  {}

  void glAnimate(RenderState *rs, GLdouble dt) {
    frameCounter_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (GLint) (frameCounter_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      frameCounter_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      widget_->set_value(ss.str());
    }
  }

private:
  ref_ptr<TextureMappedText> widget_;
  GLuint frameCounter_;
  GLint fps_;
  GLdouble sumDtMiliseconds_;
};

/////////////////
////// Scene Loader Animation
/////////////////

class SceneLoaderAnimation : public Animation
{
public:
  SceneLoaderAnimation(SceneDisplayWidget *widget, const string &sceneFile)
  : Animation(GL_TRUE,GL_FALSE),
    widget_(widget), sceneFile_(sceneFile)
  {
  }
  void glAnimate(RenderState *rs, GLdouble dt) {
    widget_->loadSceneGraphicsThread(sceneFile_);
  }
  SceneDisplayWidget *widget_;
  const string sceneFile_;
};

/////////////////
////// Animation Range Setter
/////////////////

class AnimationRangeUpdater : public EventHandler
{
public:
  AnimationRangeUpdater(
      const ref_ptr<NodeAnimation> &anim,
      const vector<AnimRange> &animRanges)
  : EventHandler(),
    anim_(anim),
    animRanges_(animRanges) {}

  void call(EventObject *ev, EventData *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;
    int index = rand() % animRanges_.size();
    anim->setAnimationIndexActive(0, animRanges_[index].range);
  }

protected:
  ref_ptr<NodeAnimation> anim_;
  vector<AnimRange> animRanges_;
};

/////////////////
/////////////////

SceneDisplayWidget::SceneDisplayWidget(QtApplication *app)
: QMainWindow(), inputDialog_(NULL), inputWidget_(NULL), app_(app)
{
  setMouseTracking(true);

  ui_.setupUi(this);
  ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0,0,1,1);
  resize(1000,700);
  readConfig();
}
void SceneDisplayWidget::init()
{
  if(activeFile_.empty()) {
    openFile();
  } else {
    loadScene(activeFile_);
  }
}

SceneDisplayWidget::~SceneDisplayWidget()
{
  if(inputDialog_ != NULL) {
    delete inputDialog_;
    delete inputWidget_;
  }
}

void SceneDisplayWidget::resetFile() {
  activeFile_ = "";
  openFile();
}
void SceneDisplayWidget::readConfig() {
  // just read in the fluid file for now
  boost::filesystem::path p(userDirectory());
  p /= CONFIG_FILE_NAME;
  if(!boost::filesystem::exists(p)) return;
  ifstream cfgFile;
  cfgFile.open(p.c_str());
  cfgFile >> activeFile_;
  cfgFile.close();
}

void SceneDisplayWidget::writeConfig() {
  // just write out the fluid file for now
  boost::filesystem::path p(userDirectory());
  p /= CONFIG_FILE_NAME;
  ofstream cfgFile;
  cfgFile.open(p.c_str());
  cfgFile << activeFile_ << endl;
  cfgFile.close();
}

void SceneDisplayWidget::nextView() {
  if(viewNodes_.empty()) return;
  ViewNode &active0 = *activeView_;
  active0.node->set_isHidden(GL_TRUE);

  activeView_++;
  if(activeView_ == viewNodes_.end()) {
    activeView_ = viewNodes_.begin();
  }

  ViewNode &active1 = *activeView_;
  active1.node->set_isHidden(GL_FALSE);
  app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));
}
void SceneDisplayWidget::previousView() {
  if(viewNodes_.empty()) return;
  ViewNode &active0 = *activeView_;
  active0.node->set_isHidden(GL_TRUE);

  if(activeView_ == viewNodes_.begin()) {
    activeView_ = viewNodes_.end();
  }
  activeView_--;

  ViewNode &active1 = *activeView_;
  active1.node->set_isHidden(GL_FALSE);
  app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));
}

void SceneDisplayWidget::toggleInputsDialog() {
  if(inputDialog_ == NULL) {
    inputDialog_ = new QDialog(this);
    inputDialog_->setWindowTitle("ShaderInput Editor");
    inputDialog_->resize(600,500);

    QGridLayout *gridLayout = new QGridLayout(inputDialog_);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

    inputWidget_ = new ShaderInputWidget(inputDialog_);
    gridLayout->addWidget(inputWidget_);
  }
  if(inputDialog_->isVisible()) {
    inputDialog_->hide();
  }
  else {
    inputWidget_->setNode(app_->renderTree());
    inputDialog_->show();
  }
}

void SceneDisplayWidget::openFile() {
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.setFilter("XML Files (*.xml);;All files (*.*)");
  dialog.setViewMode(QFileDialog::Detail);
  dialog.selectFile(QString(activeFile_.c_str()));

  if(!dialog.exec()) {
    if(activeFile_.empty()) {
      REGEN_WARN("no texture updater file selected, exiting.");
      exit(0);
    }
    return;
  }

  QStringList fileNames = dialog.selectedFiles();
  activeFile_ = fileNames.first().toStdString();
  writeConfig();

  loadScene(activeFile_);
}

void SceneDisplayWidget::updateSize() {
}

void SceneDisplayWidget::loadScene(const string &sceneFile) {
  if(inputDialog_!=NULL && inputDialog_->isVisible()) {
    inputDialog_->hide();
  }
  loadAnim_ = ref_ptr<SceneLoaderAnimation>::alloc(this, sceneFile);
}

void SceneDisplayWidget::loadSceneGraphicsThread(const string &sceneFile) {
  REGEN_INFO("Loading XML scene at " << sceneFile << ".");

  if(camKeyHandler_.get()) app_->disconnect(camKeyHandler_);
  if(camMotionHandler_.get()) app_->disconnect(camMotionHandler_);
  if(camButtonHandler_.get()) app_->disconnect(camButtonHandler_);
  AnimationManager::get().pause(GL_TRUE);

  viewNodes_.clear();
  manipulator_ = ref_ptr<EgoCameraManipulator>();
  camKeyHandler_ = ref_ptr<EventHandler>();
  camMotionHandler_ = ref_ptr<EventHandler>();
  camButtonHandler_ = ref_ptr<EventHandler>();
  physics_ = ref_ptr<BulletPhysics>();
  nodeAnimations_.clear();

  list< ref_ptr<EventHandler> > eventHandler_;
  for(list< ref_ptr<EventHandler> >::iterator
      it=eventHandler_.begin(); it!=eventHandler_.end(); ++it)
  {
    app_->disconnect(*it);
  }
  app_->clear();

  ref_ptr<RootNode> tree = app_->renderTree();

  ref_ptr<SceneInputXML> xmlInput = ref_ptr<SceneInputXML>::alloc(sceneFile);
  scene::SceneParser sceneParser(app_,xmlInput);
  sceneParser.setNodeProcessor(ref_ptr<ViewNodeProcessor>::alloc(&viewNodes_));
  sceneParser.processNode(tree, "root", "node");
  physics_ = sceneParser.getPhysics();
  eventHandler_ = sceneParser.getEventHandler();

  if(xmlInput->getRoot()->getFirstChild("node","initialize").get()!=NULL) {
    ref_ptr<StateNode> initializeNode = ref_ptr<StateNode>::alloc();
    sceneParser.processNode(initializeNode, "initialize", "node");
    initializeNode->traverse(RenderState::get());
  }

  ref_ptr<SceneInputNode> root = sceneParser.getRoot();
  ref_ptr<SceneInputNode> guiConfigNode = root->getFirstChild("gui-config");
  if(guiConfigNode.get()==NULL) { guiConfigNode = root; }

  const string cameraID =
      guiConfigNode->getValue<string>("camera","main-camera");
  const string assetID =
      guiConfigNode->getValue<string>("animation-asset","animation-asset");

  // Add camera manipulator for named camera
  //ref_ptr<Camera> cam = xmlLoader.getCamera(cameraID);
  ref_ptr<Camera> cam =
      sceneParser.getResources()->getCamera(&sceneParser,cameraID);
  if(cam.get()!=NULL) {
    if(guiConfigNode->getValue<bool>("ego-camera",true)) {
      ref_ptr<EgoCameraManipulator> egoManipulator =
          ref_ptr<EgoCameraManipulator>::alloc(cam);
      egoManipulator->set_moveAmount(
          guiConfigNode->getValue<GLfloat>("ego-speed",0.01f));

      ref_ptr<EgoCamKey> keyCallable = ref_ptr<EgoCamKey>::alloc(egoManipulator);
      ref_ptr<EgoCamButton> buttonCallable = ref_ptr<EgoCamButton>::alloc(egoManipulator);
      ref_ptr<EgoCamMotion> motionCallable =
          ref_ptr<EgoCamMotion>::alloc(egoManipulator, buttonCallable->buttonPressed_);
      motionCallable->sensitivity_ =
          guiConfigNode->getValue<GLfloat>("ego-sensitivity",0.005f);

      camKeyHandler_ = keyCallable;
      camButtonHandler_ = buttonCallable;
      camMotionHandler_ = motionCallable;
      manipulator_ = egoManipulator;

      app_->connect(Application::KEY_EVENT, camKeyHandler_);
      app_->connect(Application::BUTTON_EVENT, camButtonHandler_);
      app_->connect(Application::MOUSE_MOTION_EVENT, camMotionHandler_);
    }
    else if(guiConfigNode->getValue<bool>("look-at-camera",true)) {
      GLuint interval =
          guiConfigNode->getValue<GLuint>("look-at-interval",10);
      ref_ptr<LookAtCameraManipulator> manipulator =
          ref_ptr<LookAtCameraManipulator>::alloc(cam,interval);
      manipulator->set_height(
          guiConfigNode->getValue<GLfloat>("look-at-height",0.0f));
      manipulator->set_lookAt(
          guiConfigNode->getValue<Vec3f>("look-at",Vec3f(0.0f)));
      manipulator->set_radius(
          guiConfigNode->getValue<GLfloat>("look-at-radius",5.0f));
      manipulator->set_degree(
          guiConfigNode->getValue<GLfloat>("look-at-degree",0.0f));
      manipulator->setStepLength(
          guiConfigNode->getValue<GLfloat>("look-at-step",0.0f));

      ref_ptr<LookAtButton> buttonCallable = ref_ptr<LookAtButton>::alloc(manipulator);
      ref_ptr<LookAtMotion> motionCallable = ref_ptr<LookAtMotion>::alloc(
          manipulator, buttonCallable->buttonPressed_);
      buttonCallable->scrollStep_ =
          guiConfigNode->getValue<GLfloat>("look-at-scroll-step",2.0f);
      motionCallable->stepX_ =
          guiConfigNode->getValue<GLfloat>("look-at-step-x",0.02f);
      motionCallable->stepY_ =
          guiConfigNode->getValue<GLfloat>("look-at-step-y",0.001f);

      camButtonHandler_ = buttonCallable;
      camMotionHandler_ = motionCallable;
      manipulator_ = manipulator;

      app_->connect(Application::BUTTON_EVENT, buttonCallable);
      app_->connect(Application::MOUSE_MOTION_EVENT, motionCallable);
    }
  }

  // Update view...
  if(viewNodes_.size()>0) {
    activeView_ = viewNodes_.end();
    activeView_--;
    ViewNode &active = *activeView_;
    active.node->set_isHidden(GL_FALSE);
    app_->toplevelWidget()->setWindowTitle(QString(active.name.c_str()));
  }

  // Update text of FPS widget
  ref_ptr<MeshVector> fpsWidget =
      sceneParser.getResources()->getMesh(&sceneParser,"fps-widget");
  if(fpsWidget.get()!=NULL && !fpsWidget->empty()) {
    ref_ptr<TextureMappedText> text =
        ref_ptr<TextureMappedText>::upCast(*fpsWidget->begin());
    if(text.get()!=NULL) {
      fbsWidgetUpdater_ = ref_ptr<UpdateFPS>::alloc(text);
      REGEN_INFO("FPS widget found.");
    } else {
      fbsWidgetUpdater_ = ref_ptr<Animation>();
      REGEN_INFO("Unable to find FPS widget.");
    }
  } else {
    fbsWidgetUpdater_ = ref_ptr<Animation>();
    REGEN_INFO("Unable to find FPS widget.");
  }

  // handle animations
  //ref_ptr<AssetImporter> animAsset = xmlLoader.getAsset(assetID);
  ref_ptr<AssetImporter> animAsset =
      sceneParser.getResources()->getAsset(&sceneParser,assetID);
  if(animAsset.get()) {
    vector<AnimRange> ranges = sceneParser.getAnimationRanges(assetID);
    nodeAnimations_ = animAsset->getNodeAnimations();

    if(!nodeAnimations_.empty() && !ranges.empty()) {
      for(GLuint i=0; i<nodeAnimations_.size(); ++i) {
        const ref_ptr<NodeAnimation> &anim = nodeAnimations_[i];
        ref_ptr<EventHandler> animStopped = ref_ptr<AnimationRangeUpdater>::alloc(anim,ranges);
        anim->connect(Animation::ANIMATION_STOPPED, animStopped);
        anim->startAnimation();

        EventData evData;
        evData.eventID = Animation::ANIMATION_STOPPED;
        animStopped->call(anim.get(), &evData);
      }
    }
  }

  loadAnim_ = ref_ptr<Animation>();
  AnimationManager::get().resume();
  REGEN_INFO("XML Scene Loaded.");
}

void SceneDisplayWidget::resizeEvent(QResizeEvent *event) {
  updateSize();
}
