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

#include <ogle/animations/animation-manager.h>
#include <ogle/utility/xml.h>

#include "texture-updater-widget.h"

#define CONFIG_FILE_NAME ".ogle-texture-updater.cfg"

TextureUpdaterWidget::TextureUpdaterWidget(QtApplication *app)
: QMainWindow(), app_(app)
{
  setMouseTracking(true);

  texture_ = ref_ptr<TextureState>::manage(new TextureState);
  texture_->set_mapTo(TextureState::MAP_TO_COLOR);

  ui_.setupUi(this);
  ui_.glWidgetLayout->addWidget(&app_->glWidget(), 0,0,1,1);
  readConfig();
}
TextureUpdaterWidget::~TextureUpdaterWidget()
{
}

void TextureUpdaterWidget::readConfig()
{
  // just read in the fluid file for now
  boost::filesystem::path p(getenv("HOME"));
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
  boost::filesystem::path p(getenv("HOME"));
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
      WARN_LOG("no texture updater file selected.");
      exit(0);
      return;
    }

    QStringList fileNames = dialog.selectedFiles();
    xmlFile = fileNames.first().toStdString();
  }

  if(texUpdater_.get()==NULL) {
    texUpdater_ = ref_ptr<TextureUpdater>::manage(new TextureUpdater);
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(texUpdater_));
  }

  try {
    texUpdater_->operator >>(xmlFile);
  }
  catch(XMLLoader::Error &e) {
    WARN_LOG("Failed to parse XML file. " << e.what());
    openFile();
    return;
  }
  textureUpdaterFile_ = xmlFile;
  texture_->set_texture(texUpdater_->outputTexture());
  updateSize();
}

void TextureUpdaterWidget::resizeEvent(QResizeEvent * event)
{ updateSize(); }

const ref_ptr<TextureState>& TextureUpdaterWidget::texture() const
{ return texture_; }
