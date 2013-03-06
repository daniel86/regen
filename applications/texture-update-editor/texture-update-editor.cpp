
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
using namespace std;

#include <boost/filesystem.hpp>

#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Hor_Slider.H>

#include <ogle/utility/string-util.h>
#include <ogle/external/glsw/glsw.h>
#include <ogle/external/rapidxml/rapidxml.hpp>
#include <ogle/states/state-node.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/textures/texture-updater.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/blit-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/textures/texture-loader.h>
#include <ogle/states/shader-configurer.h>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>
#include <ogle/config.h>

using namespace ogle;

#define CONFIG_FILE_NAME ".ogle-texture-update-editor.cfg"
#define STATUS_TIME 3.0

// defines style of syntax highlighting
#define INVALID_STYLE   'A'
#define COMMENT_STYLE   'B'
#define META_TAG_STYLE  'C'
#define DATA_TAG_STYLE  'D'
#define ATTR_STYLE      'E'
#define VAL_STYLE       'F'
static Fl_Text_Display::Style_Table_Entry XMLStyle[] = {
  { FL_RED,             FL_TIMES,          FL_NORMAL_SIZE }, // A
  { FL_BLUE,            FL_TIMES_ITALIC,   FL_NORMAL_SIZE }, // B
  { FL_DARK_MAGENTA,    FL_TIMES,          FL_NORMAL_SIZE }, // C
  { FL_DARK_CYAN,       FL_TIMES_BOLD,     FL_NORMAL_SIZE }, // D
  { FL_DARK_BLUE,       FL_TIMES,          FL_NORMAL_SIZE }, // E
  { FL_MAGENTA,         FL_TIMES,          FL_NORMAL_SIZE }  // F
};
#define isCommentStart(txt,start,end) hasPrefix(txt,"<!--",start,end)
#define isCommentEnd(txt,start,end)   hasPrefix(txt,"-->",start,end)
#define isMetaTagStart(txt,start,end) hasPrefix(txt,"<?",start,end)
#define isMetaTagEnd(txt,start,end)   hasPrefix(txt,"?>",start,end)
#define isDataTagStart(txt,start,end) hasPrefix(txt,"<",start,end)
#define isDataTagEnd(txt,start,end)   hasPrefix(txt,">",start,end)
#define isValueStart(txt,start,end)   hasPrefix(txt,"=\"",start,end)

// contains data used for buttons in button bar
struct ButtonInfo {
  const char *iconName;
  ulong shortcut;
  const char *tooltip;
  void (*callback)(Fl_Widget*,void*);
};

// updates a widget using the fps from ogle
class UpdateFPSFltk : public Animation
{
public:
  UpdateFPSFltk(Fl_Widget *widget)
  : Animation(),
    widget_(widget),
    numFrames_(0),
    sumDtMiliseconds_(0.0f) {}

  virtual void animate(GLdouble) {}
  virtual void glAnimate(RenderState *rs, GLdouble dt)
  {
    numFrames_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (int) (numFrames_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      numFrames_ = 0;
      label_ = FORMAT_STRING(fps_ << " FPS");
      widget_->label(label_.c_str());
    }
  }
  virtual GLboolean useGLAnimation() const {
    return GL_TRUE;
  }
  virtual GLboolean useAnimation() const {
    return GL_FALSE;
  }

  Fl_Widget *widget_;
  string label_;
  unsigned int numFrames_;
  int fps_;
  double sumDtMiliseconds_;
};

static bool hasPrefix(
    const char *text,
    const string &prefix,
    int start, int end)
{
  if((int)prefix.size() > (end-start)) { return false; }
  const char *prefixStr = prefix.c_str();
  for(GLuint i=0u; i<prefix.size(); ++i) {
    if(prefixStr[i] != text[start+i]) {
      return false;
    }
  }
  return true;
}

class FluidEditor : public OGLEFltkApplication
{
public:
  FluidEditor(const ref_ptr<RootNode> &renderTree, int &argc, char** argv)
  : OGLEFltkApplication(renderTree, argc, argv, 1070, 600),
    editorWidget_(NULL),
    textbuf_(NULL),
    isDragInitialSplat_(GL_FALSE),
    modified_(GL_FALSE),
    isFileLoading_(GL_FALSE),
    isMouseDragging_(GL_FALSE)
  {
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    readConfig();

    if(textureUpdaterFile_.empty() || !boost::filesystem::exists(textureUpdaterFile_)) {
      // unable to find a valid path
      char *chosenFile = fl_file_chooser("Please choose a fluid file.", "*.xml", "");
      if(chosenFile==NULL) exit(0);
      textureUpdaterFile_ = string(chosenFile);
    }
    if(iconPath_.empty() || !boost::filesystem::exists(iconPath_)) {
      // unable to find a valid path
      char *chosenDir = fl_dir_chooser("Please choose the directory where icons are located.", "");
      if(chosenDir==NULL) exit(0);
      iconPath_ = string(chosenDir);
    }

    texState_ = ref_ptr<TextureState>::manage(new TextureState);
    texState_->setMapTo(MAP_TO_COLOR);

    INFO_LOG("Texture-Updater File: " << textureUpdaterFile_);
    INFO_LOG("Icon Path: " << iconPath_);
  }

  const string& textureUpdaterFile() const { return textureUpdaterFile_; }
  const string& textureUpdaterFileName() const { return textureUpdaterFileName_; }
  void set_textureUpdaterFile(const string &textureUpdaterFile) {
    textureUpdaterFile_ = textureUpdaterFile;
    if(textbuf_!=NULL) {
      // update editor if it was not constructed yet
      isFileLoading_ = GL_TRUE;
      textbuf_->loadfile(textureUpdaterFile_.c_str());
      isFileLoading_ = GL_FALSE;
    }
  }

  void set_modified(GLboolean modified) { modified_ = modified; }
  GLboolean modified() const { return modified_; }

  GLboolean isFileLoading() const { return isFileLoading_; }

  Fl_Box* statusWidget() { return statusWidget_; }
  Fl_Text_Editor* editorWidget() { return editorWidget_; }
  Fl_Text_Buffer* textbuf() { return textbuf_; }
  Fl_Text_Buffer* stylebuf() { return stylebuf_; }

  void updateWindowTitle() {
    boost::filesystem::path p(textureUpdaterFile_);
    textureUpdaterFileName_ = string(p.filename().c_str());
    if (modified_) {
      set_windowTitle(textureUpdaterFileName_ + "(modified) - OGLE Texture-Update Editor");
    } else {
      set_windowTitle(textureUpdaterFileName_ + " - OGLE Texture-Update Editor");
    }
  }

  virtual void exitMainLoop(int errorCode) {
    // save before exiting
    if(modified_ && askSave()==0) { return; }
    OGLEFltkApplication::exitMainLoop(errorCode);
  }

  //////////////////////////////////
  /////// CONFIG FILE IO ///////////
  //////////////////////////////////

  void readConfig() {
    // just read in the fluid file for now
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    if(!boost::filesystem::exists(p)) return;
    ifstream cfgFile;
    cfgFile.open(p.c_str());
    cfgFile >> textureUpdaterFile_;
    cfgFile >> iconPath_;
    cfgFile.close();
  }
  void writeConfig() {
    // just write out the fluid file for now
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    ofstream cfgFile;
    cfgFile.open(p.c_str());
    cfgFile << textureUpdaterFile_ << endl;
    cfgFile << iconPath_ << endl;
    cfgFile.close();
  }

  //////////////////////////////////
  /////// FLUID FILE IO ////////////
  //////////////////////////////////

  void saveFile(const char *filePath) {
    // save current editor content
    ofstream fluidFileStream;
    fluidFileStream.open(filePath);
    fluidFileStream << textbuf_->text();
    fluidFileStream.close();
  }
  void saveFileReload(const char *filePath) {
    // save current editor content and reload simulation
    saveFile(filePath);
    textureUpdaterFile_ = filePath;
    if(loadTextureUpdater(GL_TRUE)) {
      statusPush(FORMAT_STRING(textureUpdaterFileName_ << " saved"));
    }
  }
  int askSave() {
    int r = fl_choice("The current file has not been saved.\n"
                      "Would you like to save it now?",
                      "Cancel", "Save", "Discard");
    if (r == 0) { return 1; } // discard
    if (r == 1) { saveFile(textureUpdaterFile_.c_str()); } // ok
    return 0;
  }

  //////////////////////////////////
  /////// STATUS MESSAGES //////////
  //////////////////////////////////

  static void statusTimeout_(void *data) {
    ((FluidEditor*)data)->statusPop();
  }
  void statusPush(const string &status) {
    // add status text, will be displayed until timeout called
    bool wasEmpty = statusQueue_.empty();
    statusQueue_.push(status);
    if(wasEmpty) {
      // no timeout running, start one
      Fl::add_timeout(STATUS_TIME,statusTimeout_,this);
      statusWidget_->label(statusQueue_.front().c_str());
    }
  }
  void statusPop() {
    // remove active status text
    statusQueue_.pop();
    if(statusQueue_.empty()) {
      statusWidget_->label("");
    } else {
      // another status left, start new timeout
      Fl::add_timeout(STATUS_TIME,statusTimeout_,this);
      statusWidget_->label(statusQueue_.front().c_str());
    }
  }

  //////////////////////////////////
  /////// FLUID LOADING ////////////
  //////////////////////////////////

  GLboolean loadTextureUpdater(GLboolean executeInitialOperations=GL_TRUE)
  {
    // try parsing the fluid
    TextureUpdater *newUpdater = new TextureUpdater;

    try {
      ifstream inputfile(textureUpdaterFile_.c_str());
      inputfile >> (*newUpdater);
    }
    catch (rapidxml::parse_error e) {
      statusPush("Parsing the texture-updater file failed.");
      ERROR_LOG("parsing the texture-updater file failed. " << e.what());
      delete newUpdater;
      return GL_FALSE;
    }
    if(newUpdater==NULL) {
      statusPush("Creating the texture-updater failed.");
      ERROR_LOG("parsing the texture-updater file failed.");
      delete newUpdater;
      return GL_FALSE;
    }

    // clean up last loaded fluid
    if(textureUpdater_.get()!=NULL) {
      AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(textureUpdater_));
    }
    textureUpdater_ = ref_ptr<TextureUpdater>::manage(newUpdater);

    list<TextureUpdateOperation*> &operations = textureUpdater_->operations();

    splats_.clear();
    initialSplats_.clear();
    mouseZoomOperations_.clear();
    zoomValue_ = 1.0;
    for(list<TextureUpdateOperation*>::iterator
        it=operations.begin(); it!=operations.end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(op->fsName() == "fluid.splat.circle" || op->fsName() == "fluid.splat.rect") {
        splats_.push_back(op);
      }
      // find zoomable operations
      Shader *shader = op->shader();
      if(shader!=NULL && shader->isUniform("mouseZoom")) {
        ref_ptr<ShaderInput> in = shader->input("mouseZoom");
        stringstream ss("1.0");
        *(in.get()) << ss;
        shader->setInput(in);
        mouseZoomOperations_.push_back(op);
      }
      // find draggable operations
      if(shader!=NULL && shader->isUniform("mouseOffset")) {
        ref_ptr<ShaderInput> in = shader->input("mouseOffset");
        stringstream ss("0.0,0.0");
        *(in.get()) << ss;
        shader->setInput(in);
        mouseDragOperations_.push_back(op);
      }
    }
    for(list<TextureUpdateOperation*>::iterator
        it=textureUpdater_->initialOperations().begin();
        it!=textureUpdater_->initialOperations().end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(op->fsName() == "fluid.splat.circle" || op->fsName() == "fluid.splat.rect") {
        initialSplats_.push_back(op);
      }
    }

    // add output texture to scene object
    SimpleRenderTarget *outputBuffer = textureUpdater_->getBuffer("output");
    if(outputBuffer == NULL) {
      statusPush("No 'output' buffer defined.");
    } else {
      outputTexture_ = outputBuffer->texture();
      texState_->set_texture(outputTexture_);
    }

    // execute the initial operations
    if(executeInitialOperations) {
      textureUpdater_->executeOperations(
          renderTree_->renderState().get(),
          textureUpdater_->initialOperations());
    }

    // finally remove old and add new animation for updating the fluid
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(textureUpdater_));

    updateWindowTitle();

    return GL_TRUE;
  }


  //////////////////////////////////
  /////// XML Syntax Highlighting //
  //////////////////////////////////

  void parseStyle(
      int start, int end,
      char *text,
      char *style,
      char lastStyle)
  {
    if(start>=end) return;

    int nextStart = start+1;
    char nextStyle = lastStyle;

    style[start] = lastStyle;

    switch(lastStyle) {
    // something not surrounded by tags
    case INVALID_STYLE:
      if(isCommentStart(text,start,end)) {
        style[start] = COMMENT_STYLE;
        nextStyle = COMMENT_STYLE;
      }
      else if(isMetaTagStart(text,start,end)) {
        style[start] = META_TAG_STYLE;
        nextStyle = META_TAG_STYLE;
      }
      else if(isDataTagStart(text,start,end)) {
        style[start] = DATA_TAG_STYLE;
        nextStyle = DATA_TAG_STYLE;
      }
      break;
    // "<!--" active
    case COMMENT_STYLE:
      if(isCommentEnd(text,start,end)) {
        style[start+1] = COMMENT_STYLE;
        style[start+2] = COMMENT_STYLE;
        nextStart = start+3;
        nextStyle = INVALID_STYLE;
      }
      break;
    // "<?" active
    case META_TAG_STYLE:
      if(isMetaTagEnd(text,start,end)) {
        style[start+1] = META_TAG_STYLE;
        nextStart = start+2;
        nextStyle = INVALID_STYLE;
      }
      break;
    // "<" active
    case DATA_TAG_STYLE:
      if(isDataTagEnd(text,start,end)) {
        nextStyle = INVALID_STYLE;
      }
      else if(text[start] == ' ') {
        nextStyle = ATTR_STYLE;
      }
      break;
    // attribute name X=".."
    case ATTR_STYLE:
      if(isDataTagEnd(text,start,end)) {
        style[start] = DATA_TAG_STYLE;
        nextStyle = INVALID_STYLE;
      }
      else if(isValueStart(text,start,end)) {
        style[start+1] = VAL_STYLE;
        nextStart = start+2;
        nextStyle = VAL_STYLE;
      }
      break;
    // attribute value ".."
    case VAL_STYLE:
      if(text[start] == '"') {
        nextStyle = DATA_TAG_STYLE;
      }
      break;
    }
    parseStyle(nextStart,end,text,style,nextStyle);
  }

  //////////////////////////////////
  /////// WIDGETS //////////////////
  //////////////////////////////////

  virtual void createWidgets(Fl_Pack *parent) {
    int hPackWidth = fltkWidth_;
    int hPackHeight = fltkHeight_;
    int hPackY = 0;
    int editorWidth = (int)(0.44*hPackWidth);
    int editorHeight = hPackHeight;
    int ogleWidth = hPackWidth-editorWidth;
    int ogleHeight = hPackHeight;

    parent->remove(fltkWindow_);

    // packing editor and view horizontal
    Fl_Tile *tile = new Fl_Tile(0,0,hPackWidth,hPackHeight);
    //hPack->type(Fl_Pack::HORIZONTAL);
    tile->begin();

    // pack editor and view
    int hPackX = 0;

    createLeftTile(hPackX,hPackY,editorWidth,editorHeight);
    hPackX += editorWidth;

    // we want to get a resize event so ogle gets a smaller size
    tile->add(fltkWindow_);
    fltkWindow_->size(ogleWidth,ogleHeight);
    fltkWindow_->position(hPackX,hPackY);
    //createRightTile(hPackX,hPackY,ogleWidth,ogleHeight);
    hPackX += ogleWidth;

    tile->end();
    // resize all child widgets
    //hPack->resizable(hPack);
    parent->resizable(tile);
    parent->add(tile);
    tile->resize(0,0,hPackWidth,hPackHeight);
  }

  void createLeftTile(int x, int y, int w, int h) {
    int buttonBarHeight = 36;
    int buttonBarWidth = w;
    int statusBarHeight = 24;
    int statusBarWidth = w;
    int editorHeight = h-buttonBarHeight-statusBarHeight;
    int editorWidth = w;
    int vPackX = 0;

    // packing editor widgets vertical
    Fl_Pack *vPack = new Fl_Pack(x,y,w,h);
    vPack->type(Fl_Pack::VERTICAL);
    vPack->begin(); {
      int vPackY = 0;

      Fl_Box *hline0 = new Fl_Box(vPackX,vPackY,editorWidth,1);
      hline0->box(FL_BORDER_BOX);
      editorHeight -= 1;
      vPackY += 1;

      createButtonBarWidget(vPackX,vPackY,buttonBarWidth,buttonBarHeight);
      vPackY += buttonBarHeight;

      Fl_Box *hline = new Fl_Box(vPackX,vPackY,editorWidth,1);
      hline->box(FL_BORDER_BOX);
      editorHeight -= 1;
      vPackY += 1;

      leftTileNotebook_ = new Fl_Tabs(vPackX,vPackY,editorWidth,editorHeight);
      int tabX = 0;
      int tabY = 24+38;
      int tabW = editorWidth;
      int tabH = editorHeight-24;
      leftTileNotebook_->begin(); {
        // first tab
        createTextEditorWidget(tabX,tabY,tabW,tabH);

        vPackY += editorHeight;

        leftTileNotebook_->resizable(editorWidget_);
      } leftTileNotebook_->end();

      createStatusBarWidget(vPackX,vPackY,statusBarWidth,statusBarHeight);
      vPackY += statusBarHeight;

      vPack->resizable(leftTileNotebook_);
    } vPack->end();
  }

  void createButtonBarWidget(int x, int y, int w, int h) {
    static ButtonInfo buttonInfo[] = {
          { "document-open", 'o', "Open a fluid file", openCallback_ },
          { "document-save", 's', "Save the editor content", saveCallback_ },
          { "document-save-as", FL_SHIFT+'s', "Save the editor content", saveAsCallback_ },
        { "separator", 0, 0, 0 },
          { "edit-cut", 'x', "Cut selected text", cutCallback_ },
          { "edit-copy", 'c', "Copy selected text", copyCallback_ },
          { "edit-paste", 'v', "Paste text", pasteCallback_ },
        { "separator", 0, 0, 0 },
          { "exit", 'q', "Exit this application", exitCallback_ }
    };

    Fl_Pack *pack = new Fl_Pack(x,y,w,h);
    pack->type(Fl_Pack::HORIZONTAL);
    pack->begin();

    int buttonX = 0;
    for(GLuint i=0u; i<sizeof(buttonInfo)/sizeof(ButtonInfo); ++i)
    {
      ButtonInfo &inf = buttonInfo[i];
      if(inf.callback==0) {
        Fl_Box *button = new Fl_Box(buttonX,0,1,h);
        button->box(FL_BORDER_BOX);
        buttonX += 1;
      }
      else {
        boost::filesystem::path imageFile(iconPath_);
        imageFile /= FORMAT_STRING(inf.iconName<<".png");
        Fl_Button *button = new Fl_Button(buttonX,0,h,h);
        Fl_Image *image = new Fl_PNG_Image(imageFile.c_str());
        button->image(image);
        button->box(FL_NO_BOX);
        //button->down_box(FL_NO_BOX);
        button->down_color(FL_DARK_BLUE);
        button->tooltip(inf.tooltip);
        button->shortcut(FL_COMMAND+inf.shortcut);
        button->callback(inf.callback, this);
        buttonX += h;
      }
    }

    pack->end();
  }

  void createTextEditorWidget(int x, int y, int w, int h) {
    textbuf_ = new Fl_Text_Buffer;
    stylebuf_ = new Fl_Text_Buffer;
    editorWidget_ = new Fl_Text_Editor(x,y,w,h);
    editorWidget_->label("XML Editor");
    editorWidget_->buffer(textbuf_);
    // associate style buffer with text
    editorWidget_->highlight_data(
        stylebuf_,
        XMLStyle,
        sizeof(XMLStyle)/sizeof(XMLStyle[0]),
        'A', // unfinished char
        styleUnfinishedCallback_,
        this);
    // update style buffer if text buffer changed
    textbuf_->add_modify_callback(styleUpdateCallback, this);
    if(!textureUpdaterFile_.empty()) {
      textbuf_->loadfile(textureUpdaterFile_.c_str());
    }
    // get notification if text changes
    textbuf_->add_modify_callback(textbufModifyCallback_, this);
  }

  void createStatusBarWidget(int x, int y, int w, int h) {
    Fl_Pack *pack = new Fl_Pack(x,y,w,h);
    pack->type(Fl_Pack::HORIZONTAL);
    pack->begin();

    int statusWidth = w*2/3;
    statusWidget_ = new Fl_Box(x,y,statusWidth,h);
    statusWidget_->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
    statusWidget_->label("");

    fpsWidget_ = new Fl_Box(x+statusWidth,y,w-statusWidth,h);
    fpsWidget_->align(FL_ALIGN_INSIDE|FL_ALIGN_RIGHT);
    fpsWidget_->label("0 FPS");
    // update FPS
    updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPSFltk(fpsWidget_));
    AnimationManager::get().addAnimation(updateFPS_);

    pack->end();
    pack->resizable(pack);
  }

  void createRightTile(int x, int y, int w, int h) {
    //fltkWindow_ = new GLWindow(this,x,y,w,h);
  }

  Vec2i getGridPosition2D(const Vec2f &winPos_)
  {
    Vec2f winPos(winPos_);
    Vec2f winSize(fltkWindow_->w(), fltkWindow_->h());
    winPos.y = winSize.y-winPos.y;

    // currently the view height always fits the
    // window height. So x is not correct if w!=h
    float sizeDiff = fltkWindow_->w()-fltkWindow_->h();
    winPos.x -= 0.5f*sizeDiff;
    winSize.x -= sizeDiff;

    Vec2f gridSize(
        outputTexture_->width(),
        outputTexture_->height());
    Vec2f posNormalized(
        winPos.x/winSize.x,
        winPos.y/winSize.y);
    Vec2f gridPosf = posNormalized*gridSize;
    Vec2i gridPosi((GLint)gridPosf.x, (GLint)gridPosf.y);

    if(gridPosi.x>gridSize.x) { gridPosi.x = gridSize.x; }
    else if(gridPosi.x<0.0f) { gridPosi.x = 0.0; }
    if(gridPosi.y>gridSize.y) { gridPosi.y = gridSize.y; }
    else if(gridPosi.y<0.0f) { gridPosi.y = 0.0; }

    return gridPosi;
  }

  virtual void mouseMove(GLint x, GLint y)
  {
    GLfloat dx = (GLfloat)lastMouseX_-(GLfloat)x;
    GLfloat dy = (GLfloat)lastMouseY_-(GLfloat)y;
    OGLEFltkApplication::mouseMove(x,y);

    if(outputTexture_.get()==NULL) {
      return;
    }

    mouseGridPos_ = getGridPosition2D(Vec2f(x,y));

    if(isMouseDragging_) {
      // update mouseOffset uniform
      for(list<TextureUpdateOperation*>::iterator
          it=mouseDragOperations_.begin(); it!=mouseDragOperations_.end(); ++it)
      {
        Shader *shader = (*it)->shader();
        if(shader!=NULL && shader->hasUniformData("mouseOffset")) {
          ShaderInput2f *in = ((ShaderInput2f*)shader->input("mouseOffset").get());
          const Vec2f &val = in->getVertex2f(0);
          Vec2f dm(dx/(GLfloat)fltkWindow_->w(),-dy/(GLfloat)fltkWindow_->h());
          in->setVertex2f(0, val + dm*(zoomValue_*2.0));
        }
      }
    }
    else if(!dragSplats_.empty()) {
      // update splatPoint uniform and splatValue for obstacle velocity
      for(list<TextureUpdateOperation*>::iterator
          it=dragSplats_.begin(); it!=dragSplats_.end(); ++it)
      {
        TextureUpdateOperation *op = *it;
        Shader *shader = op->shader();
        if(!shader->hasUniformData("splatPoint")) { continue; }

        ref_ptr<ShaderInput> inValue = shader->input("splatPoint");
        Vec2f pos = inValue->getVertex2f(0);
        Vec2f lastPos = pos;
        pos.x = (GLfloat)mouseGridPos_.x;
        pos.y = (GLfloat)mouseGridPos_.y;
        inValue->setVertex2f(0, pos);
        if(op->outputBuffer()->name()=="obstacles" && shader->hasUniformData("splatValue")) {
          ref_ptr<ShaderInput> inValue = shader->input("splatValue");
          if(inValue->valsPerElement()>=3) {
            Vec3f velocity = inValue->getVertex3f(0);
            Vec2f ds = (pos-lastPos)*4.0;
            velocity.y = ds.x; // x-velocity
            velocity.z = ds.y; // y-velocity
            inValue->setVertex3f(0, velocity);
          }
        }
        if(isDragInitialSplat_) {
          op->outputBuffer()->bind();
          op->outputBuffer()->clear(Vec4f(0.0f),1);
        }
      }

      if(isDragInitialSplat_) {
        textureUpdater_->executeOperations(
            renderTree_->renderState().get(),
            textureUpdater_->initialOperations());
      }
    }
  }

  GLboolean isDragObstacle(TextureUpdateOperation *op, const Vec2f &mousePos)
  {
    Shader *shader = op->shader();
    if(!shader->hasUniformData("splatPoint")) { return GL_FALSE; }
    const Vec2f &pos = shader->input("splatPoint")->getVertex2f(0);

    if(shader->hasUniformData("splatRadius")) {
      // circle
      const GLfloat &radius = shader->input("splatRadius")->getVertex1f(0);
      if(radius > (pos-mousePos).length()) { return GL_TRUE; }
    } else if(shader->hasUniformData("splatSize")) {
      // rect
      const Vec2f &size = shader->input("splatSize")->getVertex2f(0);
      Vec2f distance(abs(pos.x-mousePos.x), abs(pos.y-mousePos.y));
      if(distance.x*2.0<size.x && distance.y*2.0<size.y) { return GL_TRUE; }
    }
    return GL_FALSE;
  }
  void findDragObstacle(const Vec2i &mousePosi)
  {
    Vec2f mousePos(mousePosi.x, mousePosi.y);

    isDragInitialSplat_ = GL_FALSE;

    for(list<TextureUpdateOperation*>::iterator
        it=initialSplats_.begin(); it!=initialSplats_.end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(isDragObstacle(op,mousePos)) {
        dragSplats_.push_back(op);
        isDragInitialSplat_ = GL_TRUE;
      }
    }
    for(list<TextureUpdateOperation*>::iterator
        it=splats_.begin(); it!=splats_.end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(isDragObstacle(op,mousePos)) {
        dragSplats_.push_back(op);
      }
    }
  }

  virtual void mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y, GLboolean isDoubleClick)
  {
    OGLEFltkApplication::mouseButton(button,pressed,x,y,isDoubleClick);

    if(outputTexture_.get()==NULL) {
      return;
    }

    if(button==0) {
      Vec2i gridPos = getGridPosition2D(Vec2f(x,y));
      if(pressed) {
        findDragObstacle(gridPos);
      } else {
        // make sure velocity set to 0
        mouseMove(x,y);
        mouseMove(x,y);
        dragSplats_.clear();
      }
      if(dragSplats_.empty()) {
        isMouseDragging_ = pressed;
      }
    } else if(button==1) {

    } else if(button==3||button==4) {
      // update mouseZoom uniform
      for(list<TextureUpdateOperation*>::iterator
          it=mouseZoomOperations_.begin(); it!=mouseZoomOperations_.end(); ++it)
      {
        Shader *shader = (*it)->shader();
        if(shader!=NULL && shader->hasUniformData("mouseZoom")) {
          ShaderInput1f *in = ((ShaderInput1f*)shader->input("mouseZoom").get());
          in->setVertex1f(0, in->getVertex1f(0)*(button==3 ? 0.95 : 1.0/0.95));
          zoomValue_ = in->getVertex1f(0);
        }

      }
    }
  }

  //////////////////////////////////
  /////// FLTK EVENTS //////////////
  //////////////////////////////////

  static void openCallback_(Fl_Widget*, void *data) {
    // open icon clicked
    FluidEditor *app = (FluidEditor*)data;
    const string &currentFile = app->textureUpdaterFile();
    char *newfile = fl_file_chooser("Open File?", "*.xml", currentFile.c_str());
    if (newfile != NULL) {
      if(app->modified()) {
        if(app->askSave()==0) {
          // the user discarded
          return;
        }
      }
      app->set_textureUpdaterFile(string(newfile));
      app->set_modified(GL_FALSE);
      if(app->loadTextureUpdater()) {
        app->statusPush(FORMAT_STRING(app->textureUpdaterFileName() << " opened"));
      }
    }
  }
  static void saveCallback_(Fl_Widget*, void *data) {
    // save icon clicked
    FluidEditor *app = (FluidEditor*)data;
    app->set_modified(GL_FALSE);
    app->saveFileReload( app->textureUpdaterFile().c_str() );
  }
  static void saveAsCallback_(Fl_Widget*, void *data) {
    // save as icon clicked
    FluidEditor *app = (FluidEditor*)data;
    const string &currentFile = app->textureUpdaterFile();
    char *newfile = fl_file_chooser("Save File As?", "*.xml", currentFile.c_str());
    if (newfile != NULL) {
      app->set_modified(GL_FALSE);
      app->saveFileReload( newfile );
    }
  }
  static void exitCallback_(Fl_Widget*, void *data) {
    // exit icon clicked
    FluidEditor *app = (FluidEditor*)data;
    app->writeConfig();
    app->exitMainLoop(0);
  }

  static void cutCallback_(Fl_Widget*, void *data) {
    // cut icon clicked
    Fl_Text_Editor::kf_cut(0, ((FluidEditor*)data)->editorWidget());
  }
  static void copyCallback_(Fl_Widget*, void *data) {
    // copy icon clicked
    Fl_Text_Editor::kf_copy(0, ((FluidEditor*)data)->editorWidget());
  }
  static void pasteCallback_(Fl_Widget*, void *data) {
    // paste icon clicked
    Fl_Text_Editor::kf_paste(0, ((FluidEditor*)data)->editorWidget());
  }

  static void styleUnfinishedCallback_(int, void*){}

  static void textbufModifyCallback_(
      int pos,
      int nInserted,
      int nDeleted,
      int nRestyled,
      const char* deletedText,
      void *data)
  {
    // editor content changed, set app to modified
    FluidEditor *app = (FluidEditor*)data;
    if ((nInserted || nDeleted) && !app->isFileLoading()) {
      if(!app->modified()) {
        app->set_modified(GL_TRUE);
        app->updateWindowTitle();
      }
    }
  }

  static void styleUpdateCallback(
      int pos,
      int nInserted,
      int nDeleted,
      int nRestyled,
      const char* deletedText,
      void *data)
  {
    // editor content changed, update the style buffer for syntax highlighting
    FluidEditor *app = (FluidEditor*)data;
    Fl_Text_Buffer *stylebuf = app->stylebuf();
    Fl_Text_Buffer *textbuf = app->textbuf();

    // If this is just a selection change, just unselect the style buffer...
    if (nInserted == 0 && nDeleted == 0) {
      stylebuf->unselect();
      return;
    }

    // Track changes in the text buffer...
    if (nInserted > 0) {
      // Insert characters into the style buffer...
      char *style = new char[nInserted + 1];
      memset(style, INVALID_STYLE, nInserted);
      style[nInserted] = '\0';

      stylebuf->replace(pos, pos + nInserted, style);
      delete[] style;
    } else {
      // Just delete characters in the style buffer...
      stylebuf->remove(pos, pos + nDeleted);
    }

    // this is inefficient because the complete text file
    // is parsed every time something is inserted or deleted.
    char *text  = textbuf->text();
    int size = textbuf->length();
    char *style = new char[size];
    app->parseStyle(0, size, text, style, INVALID_STYLE);
    stylebuf->replace(0, size, style);
    app->editorWidget()->redisplay_range(0, size);
    free(text);
    delete[] style;
  }

  const ref_ptr<TextureState>& texState() const
  {
    return texState_;
  }

protected:
  // fluid xml file
  string textureUpdaterFile_;
  // fluid file name
  string textureUpdaterFileName_;
  // path where icons can be loaded
  string iconPath_;

  // fluid simulation handle
  ref_ptr<TextureUpdater> textureUpdater_;
  // the output texture of the simulation
  ref_ptr<Texture> outputTexture_;

  list<TextureUpdateOperation*> mouseZoomOperations_;
  list<TextureUpdateOperation*> mouseDragOperations_;
  GLdouble zoomValue_;

  // widget displaying fps from ogle
  Fl_Box *fpsWidget_;
  // timeout updates fps label
  ref_ptr<Animation> updateFPS_;

  // status display widget
  Fl_Box *statusWidget_;
  // queued status messages
  queue<string> statusQueue_;

  Fl_Tabs *leftTileNotebook_;
  // the editor widget
  Fl_Text_Editor *editorWidget_;
  // contains the text the editor displays
  Fl_Text_Buffer *textbuf_;
  // contains syntax highlighting definition
  // Each character from textbuf_ is replaced by the
  // style identifier that matches the syntax.
  Fl_Text_Buffer *stylebuf_;

  Vec2i mouseGridPos_;

  list<TextureUpdateOperation*> dragSplats_;
  list<TextureUpdateOperation*> splats_;
  list<TextureUpdateOperation*> initialSplats_;
  GLboolean isDragInitialSplat_;

  ref_ptr<TextureState> texState_;

  // was the document modified ?
  GLboolean modified_;
  // is a file loading at the moment ?
  GLboolean isFileLoading_;
  GLboolean isMouseDragging_;
};

ref_ptr<MeshState> createTextureWidget(
    OGLEApplication *app,
    const ref_ptr<TextureState> &texState,
    const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 0.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f);
  quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
  ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new Rectangle(quadConfig));

  mesh->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderConfigurer.define("USE_NORMALIZED_COORDINATES", "TRUE");
  shaderState->createShader(shaderConfigurer.cfg(), "gui");

  return mesh;
}

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventCallable
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventCallable(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    fboState_->resize(app->glWidth()*wScale_, app->glHeight()*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};

void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->glSizePtr(), attachment));
  app->renderTree()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

int main(int argc, char** argv)
{
  // create and show application window
  ref_ptr<RootNode> tree = ref_ptr<RootNode>::manage(new RootNode);
  ref_ptr<FluidEditor> app = ref_ptr<FluidEditor>::manage(new FluidEditor(tree,argc,argv));
  //app->setWaitForVSync(GL_TRUE);
  app->show();
  // set the render state that is used during tree traversal
  tree->set_renderState(ref_ptr<RenderState>::manage(new RenderState));

  // add a custom path for shader loading
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "texture-update-editor";
  shaderPath /= "shader";
  OGLEApplication::setupGLSWPath(shaderPath);

  // create render target
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(app->glWidth(), app->glHeight(), 1, GL_NONE,GL_NONE,GL_NONE));
  ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA16F, GL_FLOAT);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
  // resize fbo with window
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::manage(
      new FramebufferResizer(fboState,1.0,1.0)));

  // create a root node (that binds the render target)
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));
  app->renderTree()->addChild(sceneRoot);

  // initially load texture
  app->loadTextureUpdater(GL_TRUE);
  // add the video widget to the root node
  createTextureWidget(app.get(), app->texState(), sceneRoot);

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);

  return app->mainLoop();
}
