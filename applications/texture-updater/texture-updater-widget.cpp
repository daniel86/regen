/*
 * texture-updater-widget.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <iostream>
using namespace std;

#include <QtGui/QFileDialog>
#include <QtCore/QString>

#include <regen/animations/animation-manager.h>
#include <regen/utility/xml.h>
#include <regen/utility/filesystem.h>

#include "texture-updater-widget.h"

#define CONFIG_FILE_NAME ".regen-texture-updater.cfg"

class LoadUpdaterAnimation : public Animation
{
public:
  LoadUpdaterAnimation(
      ref_ptr<TextureUpdater> *updater,
      const ref_ptr<TextureState> &tex)
  : Animation(GL_TRUE,GL_FALSE), updater_(updater), tex_(tex)
  {
    stopAnimation();
  }

  void loadFile(const string &xmlFile)
  {
    xmlFile_ = xmlFile;
    startAnimation();
  }

  void glAnimate(RenderState *rs, GLdouble dt)
  {
    ref_ptr<TextureUpdater> nextUpdater;
    if(updater_->get()==NULL) {
      nextUpdater = ref_ptr<TextureUpdater>::alloc();
    } else {
      nextUpdater = *updater_;
    }
    try {
      nextUpdater->operator >>(xmlFile_);
    }
    catch(rapidxml::parse_error &e) {
      REGEN_WARN("Failed to parse XML file: " << e.what() << ".");
      stopAnimation();
      return;
    }
    catch(xml::Error &e) {
      REGEN_WARN("Failed to parse XML file: " << e.what() << ".");
      stopAnimation();
      return;
    }
    *updater_ = nextUpdater;
    tex_->set_texture(nextUpdater->outputTexture());

    stopAnimation();
  }

protected:
  ref_ptr<TextureUpdater> *updater_;
  ref_ptr<TextureState> tex_;
  string xmlFile_;
};

TextureUpdaterWidget::TextureUpdaterWidget(QtApplication *app)
: QMainWindow(), app_(app)
{
  setMouseTracking(true);

  texture_ = ref_ptr<TextureState>::alloc();
  texture_->set_mapTo(TextureState::MAP_TO_COLOR);

  updaterLoader_ = ref_ptr<LoadUpdaterAnimation>::alloc(&texUpdater_, texture_);

  ui_.setupUi(this);
  ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0,0,1,1);
  readConfig();
}
TextureUpdaterWidget::~TextureUpdaterWidget()
{
}

void TextureUpdaterWidget::resetFile()
{
  textureUpdaterFile_ = "";
  openFile();
}

void TextureUpdaterWidget::readConfig()
{
  // just read in the fluid file for now
  boost::filesystem::path p(userDirectory());
  p /= CONFIG_FILE_NAME;
  if(!boost::filesystem::exists(p)) return;
  ifstream cfgFile;
  cfgFile.open(p.c_str());
  cfgFile >> textureUpdaterFile_;
  cfgFile.close();
}

void TextureUpdaterWidget::writeConfig()
{
  // just write out the fluid file for now
  boost::filesystem::path p(userDirectory());
  p /= CONFIG_FILE_NAME;
  ofstream cfgFile;
  cfgFile.open(p.c_str());
  cfgFile << textureUpdaterFile_ << endl;
  cfgFile.close();
}

void TextureUpdaterWidget::updateSize()
{
  ref_ptr<Texture> tex = texture_->texture();
  if(tex.get()==NULL) return;

  GLfloat widgetRatio = ui_.blackBackground->width()/(GLfloat)ui_.blackBackground->height();
  GLfloat texRatio = tex->width()/(GLfloat)tex->height();
  GLint w,h;
  if(widgetRatio>texRatio) {
    w = (GLint)(ui_.blackBackground->height()*texRatio);
    h = ui_.blackBackground->height();
  }
  else {
    w = ui_.blackBackground->width();
    h = (GLint)(ui_.blackBackground->width()/texRatio);
  }
  if(w%2 != 0) { w-=1; }
  if(h%2 != 0) { h-=1; }
  ui_.glWidget->setMinimumSize(QSize(max(2,w),max(2,h)));
}

void TextureUpdaterWidget::openFile()
{
  string xmlFile = textureUpdaterFile_;
  if(xmlFile.empty() || texUpdater_.get()) {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setFilter("XML Files (*.xml);;All files (*.*)");
    dialog.setViewMode(QFileDialog::Detail);
    dialog.selectFile(QString(textureUpdaterFile_.c_str()));

    if(!dialog.exec()) {
      REGEN_WARN("no texture updater file selected.");
      exit(0);
      return;
    }

    QStringList fileNames = dialog.selectedFiles();
    xmlFile = fileNames.first().toStdString();
  }
  LoadUpdaterAnimation *loader = (LoadUpdaterAnimation*)updaterLoader_.get();
  loader->loadFile(xmlFile);
  if(!texture_->texture().get()) {
    loader->glAnimate(RenderState::get(),0.0);
    updateSize();
  }
  textureUpdaterFile_ = xmlFile;
}

void TextureUpdaterWidget::resizeEvent(QResizeEvent * event)
{
  updateSize();
}

const ref_ptr<TextureState>& TextureUpdaterWidget::texture() const
{ return texture_; }
