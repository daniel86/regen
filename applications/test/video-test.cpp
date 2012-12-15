
#include <boost/filesystem/path.hpp>
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/av/audio.h>
#include <ogle/textures/video-texture.h>
#include <ogle/animations/animation-manager.h>
#include <FL/Fl_File_Chooser.H>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

// TODO:
//      - video stopped event
//          choose next or random from playlist
//      - playlist
//          random, normal
//          add / remove
//          double click play

class VideoKeyEventHandler : public EventCallable
{
public:
  VideoKeyEventHandler() : EventCallable() {}

  void call(EventObject *ev, void *data)
  {
    OGLEApplication::KeyEvent *keyEv = (OGLEApplication::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->keyValue == FL_Left) {
      vid->seekBackward(10.0);
    }
    else if(keyEv->keyValue == FL_Right) {
      vid->seekForward(10.0);
    }
    else if(keyEv->keyValue == FL_Up) {
      vid->seekForward(60.0);
    }
    else if(keyEv->keyValue == FL_Down) {
      vid->seekBackward(60.0);
    }
    else if(keyEv->key == ' ') {
      vid->togglePlay();
    }
    else if(keyEv->key == 'o') {
      openFile();
    }
    else if(keyEv->key == '+') {
      float gain = vid->audioSource()->gain();
      gain += 0.1;
      vid->audioSource()->set_gain(gain);
    }
    else if(keyEv->key == '-') {
      float gain = vid->audioSource()->gain();
      gain -= 0.1;
      if(gain < 0.0) { gain = 0.0; }
      vid->audioSource()->set_gain(gain);
    }
    else if(keyEv->key == 'n') {

    }
  }
  void openFile() {
    char *chosenFile = fl_file_chooser("Please choose a video file.", "*", "");
    if(chosenFile!=NULL) {
      string filePath = string(chosenFile);
      vid->set_file(filePath);

      boost::filesystem::path bdir(chosenFile);
      app->set_windowTitle(bdir.filename().c_str());

      vid->play();
      app->resize(vid->width(), vid->height());
      app->setKeepAspect();
    }
  }
  OGLEFltkApplication *app;
  ref_ptr<VideoTexture> vid;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
  application->setWaitForVSync(GL_TRUE);
  application->setKeepAspect();
  application->set_windowTitle("Video Player");
  application->show();

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_FALSE,
      GL_FALSE,
      Vec4f(0.0f));

  AudioSystem &as = AudioSystem::get();
  as.set_listenerPosition( Vec3f(0.0) );
  as.set_listenerVelocity( Vec3f(0.0) );
  as.set_listenerOrientation( Vec3f(0.0,0.0,1.0), UP_VECTOR );

  // load video and initially open file
  ref_ptr<VideoTexture> v = ref_ptr<VideoTexture>::manage(new VideoTexture);
  v->set_repeat( true );
  ref_ptr<VideoKeyEventHandler> keyHandler = ref_ptr<VideoKeyEventHandler>::manage(
      new VideoKeyEventHandler);
  keyHandler->vid = v;
  keyHandler->app = application;
  keyHandler->openFile();
  application->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventCallable>::cast(keyHandler));

  // add a GUI element that covers the complete screen
  UnitQuad::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 0.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f, 1.0f, 1.0f);
  quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
  ref_ptr<MeshState> quad =
      ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));
  quad->shaderDefine("USE_NORMALIZED_COORDINATES", "TRUE");
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(v)));
  texState->setMapTo(MAP_TO_COLOR);
  quad->joinStates(ref_ptr<State>::cast(texState));
  renderTree->addGUIElement(quad);

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
