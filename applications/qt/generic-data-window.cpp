/*
 * generic-data-window.cpp
 *
 *  Created on: 16.03.2013
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>

#include "generic-data-window.h"
using namespace ogle;

static const string __typeString(GLenum dataType) {
  switch(dataType) {
  case GL_FLOAT: return "float";
  case GL_INT: return "int";
  case GL_UNSIGNED_INT: return "unsigned int";
  default: return FORMAT_STRING(dataType);
  }
}

GenericDataWindow::GenericDataWindow()
: QMainWindow()
{
  ui_.setupUi(this);

  selectedItem_ = NULL;
  selectedInput_ = NULL;
}

void GenericDataWindow::addGenericData(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const string &description)
{
  minBounds_[in.get()] = minBound;
  maxBounds_[in.get()] = maxBound;
  description_[in.get()] = (description.empty() ? "no description specified." : description);

  QStringList splitPath = QString(treePath.c_str()).split(".");
  // add root folder as top level item if treeWidget doesn't already have it
  QList<QTreeWidgetItem*> toplevel = ui_.treeWidget->findItems(splitPath[0], Qt::MatchFixedString);
  if (toplevel.isEmpty()) {
    QTreeWidgetItem *topLevelItem = new QTreeWidgetItem;
    topLevelItem->setText(0, splitPath[0]);
    ui_.treeWidget->addTopLevelItem(topLevelItem);
    toplevel.append(topLevelItem);
  }
  QTreeWidgetItem *parentItem = toplevel[0];

  // iterate through non-root nodes
  for(int i=1; i<splitPath.size()-1; ++i)
  {
    // iterate through children of parentItem
    bool exists = false;
    for (int j=0; j<parentItem->childCount(); ++j)
    {
      if(splitPath[i] != parentItem->child(j)->text(0)) continue;

      exists = true;
      parentItem = parentItem->child(j);
      break;
    }
    if(!exists) {
      parentItem = new QTreeWidgetItem(parentItem);
      parentItem->setText(0, splitPath[i]);
    }
  }

  QTreeWidgetItem *childItem = new QTreeWidgetItem(parentItem);
  childItem->setText(0, splitPath.last());
  inputs_[childItem] = in;
}

void GenericDataWindow::setValue(int sliderValue,
    int minValue, int maxValue, int index)
{
  assert(selectedInput_ != NULL);

  GLfloat *boundMax = &maxBounds_[selectedInput_].x;
  GLfloat *boundMin = &minBounds_[selectedInput_].x;
  GLfloat p = ((GLfloat) (sliderValue-minValue)) / ((GLfloat) (maxValue-minValue));
  GLfloat v = boundMin[index]*(1.0-p) + boundMax[index]*p;

  byte *value = selectedInput_->dataPtr();
  GLenum dataType = selectedInput_->dataType();
  switch(dataType) {
  case GL_FLOAT: {
    ((GLfloat*)value)[index] = v;
    break;
  }
  case GL_INT: {
    ((GLint*)value)[index] = (GLint)v;
    break;
  }
  case GL_UNSIGNED_INT: {
    ((GLuint*)value)[index] = (GLuint)v;
    break;
  }
  default:
    WARN_LOG("unknown data type " << dataType);
    break;
  }
  // update timestamp
  selectedInput_->setUniformDataUntyped(value);
}

//////////////////////////////
//////// Slots
//////////////////////////////

void GenericDataWindow::setXValue(int v)
{ setValue(v,ui_.xValue->minimum(),ui_.xValue->maximum(),0); }
void GenericDataWindow::setYValue(int v)
{ setValue(v,ui_.yValue->minimum(),ui_.yValue->maximum(),1); }
void GenericDataWindow::setZValue(int v)
{ setValue(v,ui_.zValue->minimum(),ui_.zValue->maximum(),2); }
void GenericDataWindow::setWValue(int v)
{ setValue(v,ui_.wValue->minimum(),ui_.wValue->maximum(),3); }

void GenericDataWindow::currentItemChanged(QTreeWidgetItem *selected, QTreeWidgetItem *lastSelected)
{
  assert(inputs_.count(selected)==1);

  // remember selection
  selectedItem_ = selected;
  selectedInput_ = inputs_[selectedItem_].get();

  ui_.nameValue->setText(selectedInput_->name().c_str());
  ui_.typeValue->setText(__typeString(selectedInput_->dataType()).c_str());
  map<ShaderInput*,string>::iterator it = description_.find(selectedInput_);
  if(it != description_.end()) {
    ui_.descriptionLabel->setText(it->second.c_str());
  }
  else {
    ui_.descriptionLabel->setText("no description specified.");
  }

  QLabel* labelWidgets[4] =
  { ui_.xLabel, ui_.yLabel, ui_.zLabel, ui_.wLabel };
  QSlider* valueWidgets[4] =
  { ui_.xValue, ui_.yValue, ui_.zValue, ui_.wValue };

  // hide component widgets
  for(int i=0; i<4; ++i) {
    labelWidgets[i]->hide();
    valueWidgets[i]->hide();
  }

  GLuint count = selectedInput_->valsPerElement();
  GLfloat *boundMax = &maxBounds_[selectedInput_].x;
  GLfloat *boundMin = &minBounds_[selectedInput_].x;
  byte *value = selectedInput_->dataPtr();

  GLuint i;
  // show and set active components
  for(i=0; i<count; ++i) {
    labelWidgets[i]->show();
    valueWidgets[i]->show();
    GLfloat v=0.0;
    switch(selectedInput_->dataType()) {
    case GL_FLOAT: {
      v = ((GLfloat*)value)[i];
      break;
    }
    case GL_INT: {
      v = (GLfloat) ((GLint*)value)[i];
      break;
    }
    case GL_UNSIGNED_INT: {
      v = (GLfloat) ((GLuint*)value)[i];
      break;
    }
    default:
      WARN_LOG("unknown data type " << selectedInput_->dataType());
      break;
    }
    GLfloat p = (v-boundMin[i])/(boundMax[i]-boundMin[i]);
    valueWidgets[i]->setSliderPosition(
        valueWidgets[i]->minimum()*(1.0-p) + valueWidgets[i]->maximum()*p);
  }
  // hide others
  for(; i<4; ++i) {
    labelWidgets[i]->hide();
    valueWidgets[i]->hide();
  }
}
