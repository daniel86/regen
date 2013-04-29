/*
 * application.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <GL/glew.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <regen/external/glsw/glsw.h>
#include <regen/config.h>

#include "application.h"
using namespace regen;

GLuint Application::BUTTON_EVENT =
    EventObject::registerEvent("button-event");
GLuint Application::KEY_EVENT =
    EventObject::registerEvent("key-event");
GLuint Application::MOUSE_MOTION_EVENT =
    EventObject::registerEvent("mouse-motion-event");
GLuint Application::MOUSE_LEAVE_EVENT =
    EventObject::registerEvent("mouse-leave-event");
GLuint Application::RESIZE_EVENT =
    EventObject::registerEvent("resize-event");

Application::Application(int &argc, char** argv)
: EventObject(),
  renderTree_(ref_ptr<RootNode>::manage(new RootNode)),
  isGLInitialized_(GL_FALSE)
{
  windowViewport_ = ref_ptr<ShaderInput2i>::manage(new ShaderInput2i("windowViewport"));
  windowViewport_->setUniformData(Vec2i(2,2));

  mousePosition_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("mousePosition"));
  mousePosition_->setUniformData(Vec2f(0.0f));

  isMouseEntered_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("mouseEntered"));
  isMouseEntered_->setUniformData(0);

  lastMotionTime_ = boost::posix_time::ptime(
      boost::posix_time::microsec_clock::local_time());
  lastDisplayTime_ = lastMotionTime_;
  lastUpdateTime_ = lastMotionTime_;

  requiredExt_.push_back("GL_VERSION_3_3");
  requiredExt_.push_back("GL_ARB_copy_buffer");
  requiredExt_.push_back("GL_ARB_framebuffer_object");
  requiredExt_.push_back("GL_ARB_instanced_arrays");
  requiredExt_.push_back("GL_ARB_texture_float");
  requiredExt_.push_back("GL_ARB_texture_multisample");
  requiredExt_.push_back("GL_ARB_viewport_array");
  requiredExt_.push_back("GL_ARB_uniform_buffer_object");
  requiredExt_.push_back("GL_ARB_vertex_array_object");
  requiredExt_.push_back("GL_ARB_map_buffer_range");
  requiredExt_.push_back("GL_ARB_texture_buffer_range");
  requiredExt_.push_back("GL_EXT_geometry_shader4");
  requiredExt_.push_back("GL_EXT_texture_filter_anisotropic");

  optionalExt_.push_back("GL_ARB_seamless_cube_map");
  optionalExt_.push_back("GL_ARB_tessellation_shader");

  srand(time(0));
}

GLboolean Application::addShaderPath(const boost::filesystem::path &path)
{
  if(!boost::filesystem::exists(path)) return GL_FALSE;
  GLboolean hasShaderFiles = GL_FALSE;
  GLboolean hasChildShaderFiles = GL_FALSE;

  boost::filesystem::directory_iterator it(path), eod;
  BOOST_FOREACH(const boost::filesystem::path &child, make_pair(it, eod))
  {
    if(is_directory(child)) {
      // check if sub directories contain glsl files
      hasChildShaderFiles |= addShaderPath(child);
    }
    else if(!hasShaderFiles && is_regular_file(child)) {
      // check if directory contains glsl files
      boost::filesystem::path ext = child.extension();
      string exts = ext.string();
      hasShaderFiles = (exts.compare(".glsl")==0);
    }
  }

  if(hasShaderFiles) {
    string includePath = path.string();

#ifdef UNIX
    // GLSW seems to want a terminal '/' on unix
    if(*includePath.rbegin()!='/') {
      includePath += "/";
    }
#endif
#ifdef WIN32
    if(*includePath.rbegin()!='/' && *includePath.rbegin()!='\\') {
      includePath += "\\";
    }
#endif

    glswAddPath(includePath.c_str(), ".glsl");
  }

  return hasShaderFiles || hasChildShaderFiles;
}

GLboolean Application::setupShaderLoading()
{
  glswInit();

  // try src directory first, might be more up to date then installation
  boost::filesystem::path srcPath(REGEN_SOURCE_DIR);
  srcPath /= REGEN_PROJECT_NAME;
  if(addShaderPath(srcPath)) {
    DEBUG_LOG("Loading shader from: " << srcPath);
    return GL_TRUE;
  }

  // if nothing found in src dir, try insatll directory
  boost::filesystem::path installPath(REGEN_INSTALL_PREFIX);
  installPath /= "share";
  installPath /= REGEN_PROJECT_NAME;
  installPath /= "shader";
  if(addShaderPath(installPath)) {
    DEBUG_LOG("Loading shader from: " << installPath);
    return GL_TRUE;
  }

  return GL_FALSE;
}

void Application::setupLogging()
{
  Logging::addLogger( new FileLogger(Logging::INFO, "regen-info.log") );
  Logging::addLogger( new FileLogger(Logging::DEBUG, "regen-debug.log") );
  Logging::addLogger( new FileLogger(Logging::WARN, "regen-error.log") );
  Logging::addLogger( new FileLogger(Logging::ERROR, "regen-error.log") );
  Logging::addLogger( new FileLogger(Logging::FATAL, "regen-error.log") );
  Logging::addLogger( new CoutLogger(Logging::INFO ) );
  Logging::addLogger( new CoutLogger(Logging::DEBUG) );
  Logging::addLogger( new CerrLogger(Logging::FATAL) );
  Logging::addLogger( new CerrLogger(Logging::ERROR) );
  Logging::addLogger( new CerrLogger(Logging::WARN) );
  Logging::set_verbosity(Logging::V);
}

void Application::addRequiredExtension(const string &ext)
{ requiredExt_.push_back(ext); }
void Application::addOptionalExtension(const string &ext)
{ optionalExt_.push_back(ext); }

void Application::mouseEnter()
{
  isMouseEntered_->setVertex1i(0,1);
  MouseLeaveEvent *event = new MouseLeaveEvent;
  event->entered = GL_TRUE;
  queueEmit(MOUSE_LEAVE_EVENT, event);
}
void Application::mouseLeave()
{
  isMouseEntered_->setVertex1i(0,0);
  MouseLeaveEvent *event = new MouseLeaveEvent;
  event->entered = GL_FALSE;
  queueEmit(MOUSE_LEAVE_EVENT, event);
}
const ref_ptr<ShaderInput1i> Application::isMouseEntered() const
{
  return isMouseEntered_;
}

void Application::mouseMove(const Vec2i &pos)
{
  boost::posix_time::ptime time(
      boost::posix_time::microsec_clock::local_time());
  GLint dx = pos.x - mousePosition_->getVertex2f(0).x;
  GLint dy = pos.y - mousePosition_->getVertex2f(0).y;
  mousePosition_->setVertex2f(0, Vec2f(pos.x,pos.y));

  MouseMotionEvent *event = new MouseMotionEvent;
  event->dt = ((GLdouble)(time - lastMotionTime_).total_microseconds())/1000.0;
  event->dx = dx;
  event->dy = dy;
  queueEmit(MOUSE_MOTION_EVENT, event);

  lastMotionTime_ = time;
}

void Application::mouseButton(const ButtonEvent &ev)
{
  mousePosition_->setVertex2f(0, Vec2f(ev.x,ev.y));

  ButtonEvent *event = new ButtonEvent;
  event->button = ev.button;
  event->x = ev.x;
  event->y = ev.y;
  event->pressed = ev.pressed;
  event->isDoubleClick = ev.isDoubleClick;
  queueEmit(BUTTON_EVENT, event);
}

void Application::keyUp(const KeyEvent &ev)
{
  KeyEvent *event = new KeyEvent;
  event->isUp = GL_TRUE;
  event->key = ev.key;
  event->x = ev.x;
  event->y = ev.y;
  queueEmit(KEY_EVENT, event);
}

void Application::keyDown(const KeyEvent &ev)
{
  KeyEvent *event = new KeyEvent;
  event->isUp = GL_FALSE;
  event->key = ev.key;
  event->x = ev.x;
  event->y = ev.y;
  queueEmit(KEY_EVENT, event);
}

void Application::resizeGL(const Vec2i &size)
{
  windowViewport_->setVertex2i(0, size);
  queueEmit(RESIZE_EVENT);
}

void Application::initGL()
{
  //glewExperimental=GL_TRUE;
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    cerr << "Error: " << glewGetErrorString(err) << endl;
    exit(1);
  }
  // glewInit may calls glGetString(GL_EXTENSIONS),
  // then invalid enum error is generated.
  GL_ERROR_LOG();

  DEBUG_LOG("VENDOR: " << glGetString(GL_VENDOR));
  DEBUG_LOG("RENDERER: " << glGetString(GL_RENDERER));
  DEBUG_LOG("VERSION: " << glGetString(GL_VERSION));

  // check for required and optional extensions
  for(list<string>::iterator it=requiredExt_.begin(); it!=requiredExt_.end(); ++it)
  {
    if(!glewIsSupported(it->c_str())) {
      ERROR_LOG((*it) << " unsupported.");
      exit(0);
    }
    else
    { DEBUG_LOG((*it) << " supported."); }
  }
  for(list<string>::iterator it=optionalExt_.begin(); it!=optionalExt_.end(); ++it)
  {
    if(!glewIsSupported(it->c_str()))
    { DEBUG_LOG((*it) << " unsupported."); }
    else
    { DEBUG_LOG((*it) << " supported."); }
  }

#define DEBUG_GLi(dname, pname) { \
    GLint val; \
    glGetIntegerv(pname, &val); \
    DEBUG_LOG(dname << ": " << val); }
  DEBUG_GLi("MAX_3D_TEXTURE_SIZE", GL_MAX_3D_TEXTURE_SIZE);
  DEBUG_GLi("MAX_ARRAY_TEXTURE_LAYERS", GL_MAX_ARRAY_TEXTURE_LAYERS);
  DEBUG_GLi("MAX_COLOR_ATTACHMENTS", GL_MAX_COLOR_ATTACHMENTS);
  DEBUG_GLi("MAX_CUBE_MAP_TEXTURE_SIZE", GL_MAX_CUBE_MAP_TEXTURE_SIZE);
  DEBUG_GLi("MAX_DRAW_BUFFERS", GL_MAX_DRAW_BUFFERS);
#ifdef GL_MAX_FRAMEBUFFER_HEIGHT
  DEBUG_GLi("MAX_FRAMEBUFFER_HEIGHT", GL_MAX_FRAMEBUFFER_HEIGHT);
#endif
#ifdef GL_MAX_FRAMEBUFFER_WIDTH
  DEBUG_GLi("MAX_FRAMEBUFFER_WIDTH", GL_MAX_FRAMEBUFFER_WIDTH);
#endif
#ifdef GL_MAX_FRAMEBUFFER_LAYERS
  DEBUG_GLi("MAX_FRAMEBUFFER_LAYERS", GL_MAX_FRAMEBUFFER_LAYERS);
#endif
  DEBUG_GLi("MAX_TEXTURE_IMAGE_UNITS", GL_MAX_TEXTURE_IMAGE_UNITS);
  DEBUG_GLi("MAX_TEXTURE_SIZE", GL_MAX_TEXTURE_SIZE);
  DEBUG_GLi("MAX_TEXTURE_BUFFER_SIZE", GL_MAX_TEXTURE_BUFFER_SIZE);
#ifdef GL_MAX_UNIFORM_LOCATIONS
  DEBUG_GLi("MAX_UNIFORM_LOCATIONS", GL_MAX_UNIFORM_LOCATIONS);
#endif
  DEBUG_GLi("MAX_UNIFORM_BLOCK_SIZE", GL_MAX_UNIFORM_BLOCK_SIZE);
  DEBUG_GLi("MAX_VERTEX_ATTRIBS", GL_MAX_VERTEX_ATTRIBS);
  DEBUG_GLi("MAX_VIEWPORTS", GL_MAX_VIEWPORTS);
  DEBUG_GLi("TEXTURE_BUFFER_OFFSET_ALIGNMENT", GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT);
#undef DEBUG_GLi

  if(setupShaderLoading()==GL_FALSE) {
    ERROR_LOG("Unable to locate shader files.");
    exit(1);
  }

  renderTree_->init();
  renderTree_->state()->joinShaderInput(windowViewport_);
  renderTree_->state()->joinShaderInput(mousePosition_);
  renderTree_->state()->joinShaderInput(isMouseEntered_);
  renderState_ = RenderState::get();
  isGLInitialized_ = GL_TRUE;
}

void Application::drawGL()
{
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = (GLdouble)(t-lastDisplayTime_).total_milliseconds();
  lastDisplayTime_ = t;
  renderTree_->render(dt);
}

void Application::updateGL()
{
  boost::posix_time::ptime t(
      boost::posix_time::microsec_clock::local_time());
  GLdouble dt = (GLdouble)(t-lastUpdateTime_).total_milliseconds();
  lastUpdateTime_ = t;
  renderTree_->postRender(dt);
}

GLboolean Application::isGLInitialized() const
{ return isGLInitialized_; }

const ref_ptr<RootNode>& Application::renderTree() const
{ return renderTree_; }

const ref_ptr<ShaderInput2i>& Application::windowViewport() const
{ return windowViewport_; }
const ref_ptr<ShaderInput2f>& Application::mousePosition() const
{ return mousePosition_; }
