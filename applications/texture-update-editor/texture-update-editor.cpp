/*
 * fluid-test.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include <ogle/animations/texture-updater.h>

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
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/quad.h>
#include <ogle/textures/image-texture.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/states/tesselation-state.h>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>
#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define CONFIG_FILE_NAME ".ogle-fluid-editor.cfg"
#define STATUS_TIME 2.0

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
  virtual void updateGraphics(GLdouble dt)
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
  if(prefix.size() > (end-start)) { return false; }
  const char *prefixStr = prefix.c_str();
  for(int i=0; i<prefix.size(); ++i) {
    if(prefixStr[i] != text[start+i]) {
      return false;
    }
  }
  return true;
}

class FluidEditor : public OGLEFltkApplication
{
  struct MouseSplat {
    FluidEditor *app;
    TextureBuffer *buffer;
    TextureUpdateOperation *operation;
    Fl_Check_Button *enabled;
    Fl_Valuator *radius;
    Fl_Valuator *r;
    Fl_Valuator *g;
    Fl_Valuator *b;
    Fl_Valuator *a;
  };

public:
  FluidEditor(TestRenderTree *renderTree, int &argc, char** argv)
  : OGLEFltkApplication(renderTree, argc, argv, 1070, 600),
    renderTree_(renderTree),
    editorWidget_(NULL),
    textbuf_(NULL),
    modified_(GL_FALSE),
    isFileLoading_(GL_FALSE),
    isMouseSplatting_(GL_FALSE),
    isDragInitialSplat_(GL_FALSE)
  {
    // find fluid path
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    fluidFile_ = readConfig();

    if(fluidFile_.empty() || !boost::filesystem::exists(fluidFile_)) {
      // unable to find a valid path
      char *chosenFile = fl_file_chooser("Please choose a fluid file.", "*.xml", "");
      if(chosenFile==NULL) exit(0);
      fluidFile_ = string(chosenFile);
    }

    // find paths relative to provided fluid file.
    // like this icons and shaders must be placed relative to
    // the fluid file. that's not so nice but i do not care for now.
    boost::filesystem::path fluidFile(fluidFile_);
    boost::filesystem::path &fluidPath = fluidFile.remove_filename();
    if(fluidPath.has_filename()) {
      boost::filesystem::path &parentPath = fluidPath.remove_filename();
      boost::filesystem::path iconPath = parentPath;
      iconPath /= "icons";
      iconPath_ = string(iconPath.c_str());
      if(!boost::filesystem::exists(iconPath)) {
        cerr << "'" << iconPath_ << "' does not exist." << endl;
      }
    } else {
      iconPath_ = string(fluidPath.c_str());
    }

    INFO_LOG("Fluid File: " << fluidFile_);
    INFO_LOG("Icon Path: " << iconPath_);
  }

  const string& fluidFile() const { return fluidFile_; }
  const string& fluidFileName() const { return fluidFileName_; }
  void set_fluidFile(const string &fluidFile) {
    fluidFile_ = fluidFile;
    if(textbuf_!=NULL) {
      // update editor if it was constructed yet
      isFileLoading_ = GL_TRUE;
      textbuf_->loadfile(fluidFile_.c_str());
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
    boost::filesystem::path p(fluidFile_);
    fluidFileName_ = string(p.filename().c_str());
    if (modified_) {
      set_windowTitle(fluidFileName_ + "(modified) - OGLE Fluid Editor");
    } else {
      set_windowTitle(fluidFileName_ + " - OGLE Fluid Editor");
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

  string readConfig() {
    // just read in the fluid file for now
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    if(!boost::filesystem::exists(p)) return "";
    ifstream cfgFile;
    string ret;
    cfgFile.open(p.c_str());
    cfgFile >> ret;
    cfgFile.close();
    return ret;
  }
  void writeConfig() {
    // just write out the fluid file for now
    boost::filesystem::path p(getenv("HOME"));
    p /= CONFIG_FILE_NAME;
    ofstream cfgFile;
    cfgFile.open(p.c_str());
    cfgFile << fluidFile_;
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
    fluidFile_ = filePath;
    if(loadFluid(GL_TRUE)) {
      statusPush(FORMAT_STRING(fluidFileName_ << " saved"));
    }
  }
  int askSave() {
    int r = fl_choice("The current file has not been saved.\n"
                      "Would you like to save it now?",
                      "Cancel", "Save", "Discard");
    if (r == 0) { return 1; } // discard
    if (r == 1) { saveFile(fluidFile_.c_str()); } // ok
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
  /////// OGLE RENDER TREE /////////
  //////////////////////////////////

  virtual void initTree()
  {
    OGLEFltkApplication::initTree();

    // load the fluid
    loadFluid(GL_FALSE);
    statusPush(FORMAT_STRING(fluidFileName_ << " opened"));

    ref_ptr<FBOState> fboState = renderTree_->setRenderToTexture(
        1.0f,1.0f,
        GL_RGB, GL_DEPTH_COMPONENT24,
        GL_TRUE, GL_TRUE,
        Vec4f(0.4f));

    {
      // TODO  FLUID_APP: window width<height and left and right parts are missing.
      // TODO  FLUID_APP: allow loading 3D fluids
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
    // FIXME: must be done after ortho quad added to tree :/
    //     * add ortho quad to vbo without post pass somehow
    fluid_->executeOperations(fluid_->initialOperations());

    renderTree_->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  }

  //////////////////////////////////
  /////// FLUID LOADING ////////////
  //////////////////////////////////

  GLboolean loadFluid(GLboolean executeInitialOperations=GL_TRUE)
  {
    // try parsing the fluid
    TextureUpdater *newFluid = new TextureUpdater;
    newFluid->set_textureQuad(renderTree_->orthoQuad().get());

    try {
      ifstream inputfile(fluidFile_.c_str());
      inputfile >> (*newFluid);
    }
    catch (rapidxml::parse_error e) {
      statusPush("Parsing the fluid file failed.");
      ERROR_LOG("parsing the fluid file failed. " << e.what());
      delete newFluid;
      return GL_FALSE;
    }
    if(newFluid==NULL) {
      statusPush("Creating the fluid failed.");
      ERROR_LOG("parsing the fluid file failed.");
      delete newFluid;
      return GL_FALSE;
    }

    // clean up last loaded fluid
    if(fluidTexture_.get() && material_.get()) {
      material_->removeTexture(fluidTexture_.get());
    }
    if(fluid_.get()!=NULL) {
      AnimationManager::get().removeAnimation(ref_ptr<Animation>::cast(fluid_));
    }
    fluid_ = ref_ptr<TextureUpdater>::manage(newFluid);

    TextureUpdater &foo = *fluid_.get();
    stringstream dummy;
    dummy << foo;
    cout << "XXXX" << endl << dummy.str() << endl << "XXX" << endl;

    list<TextureUpdateOperation*> &operations = fluid_->operations();

    // find advect buffers
    advectTargets_.clear();
    splats_.clear();
    initialSplats_.clear();
    for(list<TextureUpdateOperation*>::iterator
        it=operations.begin(); it!=operations.end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(op->fsName() == "fluid.advect") {
        advectTargets_.push_back(op->outputBuffer());
      }
      else if(op->fsName() == "fluid.splat.circle" || op->fsName() == "fluid.splat.rect") {
        splats_.push_back(op);
      }
    }
    for(list<TextureUpdateOperation*>::iterator
        it=fluid_->initialOperations().begin(); it!=fluid_->initialOperations().end(); ++it)
    {
      TextureUpdateOperation *op = *it;
      if(op->fsName() == "fluid.splat.circle" || op->fsName() == "fluid.splat.rect") {
        initialSplats_.push_back(op);
      }
    }

    // add fluid texture to scene object
    TextureBuffer *fluidBuffer = fluid_->getBuffer("fluid");
    fluidTexture_ = fluidBuffer->texture();
    fluidTexture_->addMapTo(MAP_TO_COLOR);
    if(material_.get()) {
      material_->addTexture(fluidTexture_);
    }

    // execute the initial operations
    if(executeInitialOperations) {
      fluid_->executeOperations(fluid_->initialOperations());
    }

    // finally remove old and add new animation for updating the fluid
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(fluid_));

    updateWindowTitle();
    fillMouseSplatWidget();

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

    // packing editor and view horizontal
    Fl_Tile *tile = new Fl_Tile(0,0,hPackWidth,hPackHeight);
    //hPack->type(Fl_Pack::HORIZONTAL);
    tile->begin();

    // pack editor and view
    int hPackX = 0;

    createLeftTile(hPackX,hPackY,editorWidth,editorHeight);
    hPackX += editorWidth;

    // we want to get a resize event so ogle gets a smaller size
    createRightTile(hPackX,hPackY,ogleWidth,ogleHeight);
    hPackX += ogleWidth;

    tile->end();
    // resize all child widgets
    //hPack->resizable(hPack);
    parent->resizable(tile);
    tile->resize(0,0,hPackWidth,hPackHeight);

    updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPSFltk(fpsWidget_));
    AnimationManager::get().addAnimation(updateFPS_);
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

      Fl_Tabs *tabs = new Fl_Tabs(vPackX,vPackY,editorWidth,editorHeight);
      int tabX = 0;
      int tabY = 24+38;
      int tabW = editorWidth;
      int tabH = editorHeight-24;
      tabs->begin(); {
        // first tab
        createTextEditorWidget(tabX,tabY,tabW,tabH);
        // second tab
        createMouseSplatWidget(tabX,tabY,tabW,tabH);

        vPackY += editorHeight;

        tabs->resizable(editorWidget_);
      } tabs->end();

      createStatusBarWidget(vPackX,vPackY,statusBarWidth,statusBarHeight);
      vPackY += statusBarHeight;

      vPack->resizable(tabs);
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
    for(int i=0; i<sizeof(buttonInfo)/sizeof(ButtonInfo); ++i)
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
    if(!fluidFile_.empty()) {
      textbuf_->loadfile(fluidFile_.c_str());
    }
    // get notification if text changes
    textbuf_->add_modify_callback(textbufModifyCallback_, this);
  }

  void createMouseSplatWidget(int x, int y, int w, int h)
  {
    mouseSplatTab_ = new Fl_Pack(x,y,w,h);
    mouseSplatTab_->label("Mouse Splat");
    mouseSplatTab_->type(Fl_Pack::VERTICAL);
    mouseSplatTab_->spacing(splatSpacing);
    fillMouseSplatWidget();
  }
  static const int splatSpacing = 4;
  static const int splatRowHeight = 28;
  static const int splatFirstColumnWidth = 100;
  Fl_Slider* createSliderRow(
      const char *name,
      int x, int y, int w, int h)
  {
    Fl_Hor_Value_Slider *slider;
    Fl_Pack *pack = new Fl_Pack(x,y,w,h);
    pack->type(Fl_Pack::HORIZONTAL);
    pack->begin(); {
      Fl_Box *box = new Fl_Box(0,0,splatFirstColumnWidth,h);
      box->label(name);
      box->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);

      slider = new Fl_Hor_Value_Slider(splatFirstColumnWidth,0,w-splatFirstColumnWidth,h);
      slider->range(0.0,256.0);
      slider->value(4.0);
      slider->precision(1);

      pack->resizable(slider);

    } pack->end();

    return slider;
  }
  void fillMouseSplatWidget()
  {
    for(list<MouseSplat*> ::iterator
        it=mouseSplats_.begin(); it!=mouseSplats_.end(); ++it)
    {
      delete (*it);
    }
    mouseSplats_.clear();

    mouseSplatTab_->clear();
    mouseSplatTab_->begin(); {
      int y=0;
      int x=0;

      Fl_Box *hline = new Fl_Box(x,y,mouseSplatTab_->w(),1);
      hline->box(FL_NO_BOX);
      y += 1 + splatSpacing;

      Fl_Slider *mouseSplatRadius_ =
          createSliderRow("Radius", x,y,mouseSplatTab_->w(),splatRowHeight);
      mouseSplatRadius_->range(0.0,256.0);
      mouseSplatRadius_->value(4.0);
      mouseSplatRadius_->callback(setMouseSplatRadiusCallback_, this);
      y += splatRowHeight + splatSpacing;

      hline = new Fl_Box(x,y,mouseSplatTab_->w(),1);
      hline->box(FL_BORDER_BOX);
      y += 1 + splatSpacing;

      for(list<TextureBuffer*>::iterator
          it=advectTargets_.begin(); it!=advectTargets_.end(); ++it)
      {
        TextureBuffer *target = *it;
        MouseSplat *mouseSplatTarget = new MouseSplat;

        mouseSplatTarget->app = this;
        mouseSplatTarget->buffer = target;
        mouseSplatTarget->radius = mouseSplatRadius_;
        mouseSplatTarget->enabled = NULL;
        mouseSplatTarget->r = NULL;
        mouseSplatTarget->g = NULL;
        mouseSplatTarget->b = NULL;
        mouseSplatTarget->a = NULL;
        mouseSplatTarget->operation = NULL;

        int count;
        switch (target->texture()->format()) {
        case GL_RED:   count=1; break;
        case GL_RG:    count=2; break;
        case GL_RGB:   count=3; break;
        case GL_RGBA:  count=4; break;
        }

        int frameHeight = (count+1)*splatRowHeight;
        int frameY = 0;
        Fl_Pack *frame = new Fl_Pack(x,y,mouseSplatTab_->w(),frameHeight);
        frame->type(Fl_Pack::VERTICAL);
        frame->begin();
        y += frameHeight + splatSpacing;

        Fl_Check_Button *check = new Fl_Check_Button(0,frameY,mouseSplatTab_->w(),splatRowHeight);
        mouseSplatTarget->enabled = check;
        check->label(target->name().c_str());
        check->callback(updateMouseSplatCallback_, mouseSplatTarget);
        frameY += splatRowHeight;

        Fl_Slider *redSlider =
            createSliderRow("    Red",0,frameY,mouseSplatTab_->w()-20,splatRowHeight);
        mouseSplatTarget->r = redSlider;
        redSlider->callback(updateMouseSplatCallback_,mouseSplatTarget);
        redSlider->range(0.0,100.0);
        redSlider->value(0.0);
        frameY += splatRowHeight;

        if(count>1) {
          Fl_Slider *greenSlider =
              createSliderRow("    Green",0,frameY,mouseSplatTab_->w()-20,splatRowHeight);
          mouseSplatTarget->g = greenSlider;
          greenSlider->callback(updateMouseSplatCallback_,mouseSplatTarget);
          greenSlider->range(0.0,100.0);
          greenSlider->value(0.0);
          frameY += splatRowHeight;
        }
        if(count>2) {
          Fl_Slider *blueSlider =
              createSliderRow("    Blue",0,frameY,mouseSplatTab_->w()-20,splatRowHeight);
          mouseSplatTarget->b = blueSlider;
          blueSlider->callback(updateMouseSplatCallback_,mouseSplatTarget);
          blueSlider->range(0.0,100.0);
          blueSlider->value(0.0);
          frameY += splatRowHeight;
        }
        if(count>3) {
          Fl_Slider *alphaSlider =
              createSliderRow("    Alpha",0,frameY,mouseSplatTab_->w()-20,splatRowHeight);
          mouseSplatTarget->a = alphaSlider;
          alphaSlider->callback(updateMouseSplatCallback_,mouseSplatTarget);
          alphaSlider->range(0.0,100.0);
          alphaSlider->value(0.0);
          frameY += splatRowHeight;
        }


        frame->end();

        hline = new Fl_Box(x,y,mouseSplatTab_->w(),1);
        hline->box(FL_BORDER_BOX);
        y += 1 + splatSpacing;

        mouseSplats_.push_back(mouseSplatTarget);
      }
    } mouseSplatTab_->end();
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

    pack->end();
    pack->resizable(pack);
  }

  void createRightTile(int x, int y, int w, int h) {
    fltkWindow_ = new GLWindow(this,x,y,w,h);
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
        fluidTexture_->width(),
        fluidTexture_->height());
    Vec2f posNormalized(
        winPos.x/winSize.x,
        winPos.y/winSize.y);
    Vec2f gridPosf = posNormalized*gridSize;
    Vec2i gridPosi((GLint)gridPosf.x, (GLint)gridPosf.y);

    if(gridPosi.x>gridSize.x) { gridPosi.x = -1; }
    else if(gridPosi.x<0.0f) { gridPosi.x = -1; }
    if(gridPosi.y>gridSize.y) { gridPosi.y = -1; }
    else if(gridPosi.y<0.0f) { gridPosi.y = -1; }

    return gridPosi;
  }

  virtual void mouseMove(GLuint x, GLuint y)
  {
    OGLEFltkApplication::mouseMove(x,y);

    mouseGridPos_ = getGridPosition2D(Vec2f(x,y));

    if(isMouseSplatting_) {

      for(list<MouseSplat*>::iterator
          it=mouseSplats_.begin(); it!=mouseSplats_.end(); ++it)
      {
        MouseSplat *splat = *it;
        if(splat->operation==NULL) { continue; }
        Shader *shader = splat->operation->shader();

        if(shader->hasUniformData("splatPoint")) {
          ref_ptr<ShaderInput> inValue = shader->input("splatPoint");
          Vec2f &pos = inValue->getVertex2f(0);
          pos.x = (float)mouseGridPos_.x;
          pos.y = (float)mouseGridPos_.y;
        }
      }
    }
    else if(!dragSplats_.empty()) {
      for(list<TextureUpdateOperation*>::iterator
          it=dragSplats_.begin(); it!=dragSplats_.end(); ++it)
      {
        TextureUpdateOperation *op = *it;
        Shader *shader = op->shader();
        if(!shader->hasUniformData("splatPoint")) { continue; }

        ref_ptr<ShaderInput> inValue = shader->input("splatPoint");
        Vec2f &pos = inValue->getVertex2f(0);
        Vec2f lastPos = pos;
        pos.x = (float)mouseGridPos_.x;
        pos.y = (float)mouseGridPos_.y;
        if(op->outputBuffer()->name()=="obstacles" && shader->hasUniformData("splatValue")) {
          ref_ptr<ShaderInput> inValue = shader->input("splatValue");
          if(inValue->valsPerElement()>=3) {
            Vec3f &velocity = inValue->getVertex3f(0);
            Vec2f ds = 4.0*(pos-lastPos);

            velocity.y = ds.x; // x-velocity
            velocity.z = ds.y; // y-velocity
          }
        }
        if(isDragInitialSplat_) {
          op->outputBuffer()->bind();
          op->outputBuffer()->clear(Vec4f(0.0f),1);
        }
      }

      if(isDragInitialSplat_) {
        fluid_->executeOperations(fluid_->initialOperations());
      }
    }
  }

  GLboolean isDragObstacle(TextureUpdateOperation *op, const Vec2f &mousePos)
  {
    Shader *shader = op->shader();
    if(!shader->hasUniformData("splatPoint")) { return GL_FALSE; }
    Vec2f &pos = shader->input("splatPoint")->getVertex2f(0);

    if(shader->hasUniformData("splatRadius")) {
      // circle
      GLfloat &radius = shader->input("splatRadius")->getVertex1f(0);
      if(radius > length(pos-mousePos)) { return GL_TRUE; }
    } else if(shader->hasUniformData("splatSize")) {
      // rect
      Vec2f &size = shader->input("splatSize")->getVertex2f(0);
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

  virtual void mouseButton(GLuint button, GLboolean pressed, GLuint x, GLuint y)
  {
    OGLEFltkApplication::mouseButton(button,pressed,x,y);

    if(button==0) {
      Vec2i gridPos = getGridPosition2D(Vec2f(x,y));
      if(pressed) {
        findDragObstacle(gridPos);
      } else {
        dragSplats_.clear();
      }
      if(dragSplats_.empty()) {
        isMouseSplatting_ = pressed;
      }
    }
  }

  void updateMouseSplatRadius()
  {
    for(list<MouseSplat*>::iterator
        it=mouseSplats_.begin(); it!=mouseSplats_.end(); ++it)
    {
      MouseSplat *splat = *it;
      if(splat->operation==NULL) { continue; }
      Shader *shader = splat->operation->shader();

      if(shader->hasUniformData("splatRadius")) {
        ref_ptr<ShaderInput> inValue = shader->input("splatRadius");
        GLfloat &val = inValue->getVertex1f(0);
        val = splat->radius->value();
      }
    }
  }
  void updateMouseSplat(MouseSplat *splat)
  {
    bool enabled = (splat->enabled->value()==1);

    if(enabled) {
      if(splat->operation==NULL) {
        map<string,string> shaderConfig, operationConfig;
        map<GLenum,string> shaderNames;
        shaderConfig["IGNORE_OBSTACLES"] = "0";
        operationConfig["fs"] = "fluid.splat.circle";
        splat->operation = new TextureUpdateOperation(
            splat->buffer, fluid_->textureQuad(), operationConfig, shaderConfig);
        splat->operation->set_blendMode(BLEND_MODE_ADD);

        Shader *shader = splat->operation->shader();
        ref_ptr<ShaderInput> inverseSize = ref_ptr<ShaderInput>::cast(splat->buffer->inverseSize());
        shader->set_input(inverseSize->name(), inverseSize);

        fluid_->addOperation(splat->operation, GL_FALSE);
      }
      Shader *shader = splat->operation->shader();

      if(shader->hasUniformData("splatRadius")) {
        ref_ptr<ShaderInput> inRadius = shader->input("splatRadius");
        GLfloat &val = inRadius->getVertex1f(0);
        val = splat->radius->value();
      }
      if(shader->hasUniformData("splatValue")) {
        ref_ptr<ShaderInput> inValue = shader->input("splatValue");
        Vec4f &val = inValue->getVertex4f(0);
        val.x = splat->r->value();
        if(splat->r) { val.x = splat->r->value(); }
        if(splat->g) { val.y = splat->g->value(); }
        if(splat->b) { val.z = splat->b->value(); }
        if(splat->a) { val.w = splat->a->value(); }
      }
      if(shader->hasUniformData("splatPoint")) {
        ref_ptr<ShaderInput> inValue = shader->input("splatPoint");
        Vec2f &pos = inValue->getVertex2f(0);
        pos.x = (float)mouseGridPos_.x;
        pos.y = (float)mouseGridPos_.y;
      }
    }
    else {
      if(splat->operation!=NULL) {
        fluid_->removeOperation(splat->operation);
        delete splat->operation;
      }
      splat->operation = NULL;
    }
  }

  //////////////////////////////////
  /////// FLTK EVENTS //////////////
  //////////////////////////////////

  static void updateMouseSplatCallback_(Fl_Widget *w, void *data) {
    MouseSplat *splat = (MouseSplat*)data;
    splat->app->updateMouseSplat(splat);
  }

  static void setMouseSplatRadiusCallback_(Fl_Widget *w, void *data) {
    FluidEditor *app = (FluidEditor*)data;
    app->updateMouseSplatRadius();
  }

  static void openCallback_(Fl_Widget*, void *data) {
    // open icon clicked
    FluidEditor *app = (FluidEditor*)data;
    const string &currentFile = app->fluidFile();
    char *newfile = fl_file_chooser("Open File?", "*.xml", currentFile.c_str());
    if (newfile != NULL) {
      if(app->modified()) {
        if(app->askSave()==0) {
          // the user discarded
          return;
        }
      }
      app->set_fluidFile(string(newfile));
      app->set_modified(GL_FALSE);
      if(app->loadFluid()) {
        app->statusPush(FORMAT_STRING(app->fluidFileName() << " opened"));
      }
    }
  }
  static void saveCallback_(Fl_Widget*, void *data) {
    // save icon clicked
    FluidEditor *app = (FluidEditor*)data;
    app->set_modified(GL_FALSE);
    app->saveFileReload( app->fluidFile().c_str() );
  }
  static void saveAsCallback_(Fl_Widget*, void *data) {
    // save as icon clicked
    FluidEditor *app = (FluidEditor*)data;
    const string &currentFile = app->fluidFile();
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

    // TODO FLUID_APP: this is inefficient because the complete text file
    // is parsed every time something is inserted or deleted.
    // but there are some troubles using a faster approach :/
    char *text  = textbuf->text();
    int size = textbuf->length();
    char *style = new char[size];
    app->parseStyle(0, size, text, style, INVALID_STYLE);
    stylebuf->replace(0, size, style);
    app->editorWidget()->redisplay_range(0, size);
    free(text);
    delete[] style;
  }

protected:
  // ogle render tree
  TestRenderTree *renderTree_;
  // material used for the scene object
  ref_ptr<Material> material_;

  // fluid simulation handle
  ref_ptr<TextureUpdater> fluid_;
  // the output texture of the simulation
  ref_ptr<Texture> fluidTexture_;

  // target buffers for advection step
  // mouse splatting can only splat to these buffers.
  list<TextureBuffer*> advectTargets_;
  list<MouseSplat*> mouseSplats_;

  // fluid xml file
  string fluidFile_;
  // fluid file name
  string fluidFileName_;
  // path where icons can be loaded
  string iconPath_;

  // widget displaying fps from ogle
  Fl_Box *fpsWidget_;
  // timeout updates fps label
  ref_ptr<Animation> updateFPS_;

  // status display widget
  Fl_Box *statusWidget_;
  // queued status messages
  queue<string> statusQueue_;

  // the editor widget
  Fl_Text_Editor *editorWidget_;
  // contains the text the editor displays
  Fl_Text_Buffer *textbuf_;
  // contains syntax highlighting definition
  // Each character from textbuf_ is replaced by the
  // style identifier that matches the syntax.
  Fl_Text_Buffer *stylebuf_;

  Fl_Pack *mouseSplatTab_;
  Vec2i mouseGridPos_;

  list<TextureUpdateOperation*> dragSplats_;
  list<TextureUpdateOperation*> splats_;
  list<TextureUpdateOperation*> initialSplats_;
  GLboolean isDragInitialSplat_;

  // was the document modified ?
  GLboolean modified_;
  // is a file loading at the moment ?
  GLboolean isFileLoading_;
  GLboolean isMouseSplatting_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

  FluidEditor *application = new FluidEditor(renderTree, argc, argv);
  application->show();

  return application->mainLoop();
}
