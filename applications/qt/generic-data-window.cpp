/*
 * generic-data-window.cpp
 *
 *  Created on: 16.03.2013
 *      Author: daniel
 */

#include <QCloseEvent>

#include <regen/utility/string-util.h>

#include "generic-data-window.h"
using namespace regen;

static const string __typeString(GLenum dataType) {
  switch(dataType) {
  case GL_FLOAT: return "float";
  case GL_INT: return "int";
  case GL_UNSIGNED_INT: return "unsigned int";
  default: return FORMAT_STRING(dataType);
  }
}

GenericDataWindow::GenericDataWindow(QWidget *parent)
: QMainWindow(parent)
{
  ui_.setupUi(this);

  QList<int> sizes;
  sizes.push_back(180);
  sizes.push_back(220);
  ui_.splitter->setSizes(sizes);

  selectedItem_ = NULL;
  selectedInput_ = NULL;
}

void GenericDataWindow::addGenericData(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const Vec4i &precision,
    const string &description)
{
  minBounds_[in.get()] = minBound;
  maxBounds_[in.get()] = maxBound;
  precisions_[in.get()] = precision;
  description_[in.get()] = (description.empty() ? "no description specified." : description);

  QStringList splitPath = QString(treePath.c_str()).split(".");
  splitPath.append(in->name().c_str());
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
  if(selectedInput_ == NULL) {
    activateValue(childItem,NULL);
  }
}

void GenericDataWindow::setValue(GLdouble sliderValue, GLint index)
{
  if(selectedInput_ == NULL) return;

  byte *value = selectedInput_->dataPtr();
  GLenum dataType = selectedInput_->dataType();
  switch(dataType) {
  case GL_FLOAT: {
    ((GLfloat*)value)[index] = sliderValue;
    break;
  }
  case GL_INT: {
    ((GLint*)value)[index] = (GLint)sliderValue;
    break;
  }
  case GL_UNSIGNED_INT: {
    ((GLuint*)value)[index] = (GLuint)sliderValue;
    break;
  }
  default:
    WARN_LOG("unknown data type " << dataType);
    break;
  }
  // update timestamp
  selectedInput_->setUniformDataUntyped(value);
}

void GenericDataWindow::closeEvent(QCloseEvent *event)
{
  emit windowClosed();
  event->accept();
}

//////////////////////////////
//////// Slots
//////////////////////////////

void GenericDataWindow::setXValue(double v)
{ setValue(v,0); }
void GenericDataWindow::setYValue(double v)
{ setValue(v,1); }
void GenericDataWindow::setZValue(double v)
{ setValue(v,2); }
void GenericDataWindow::setWValue(double v)
{ setValue(v,3); }

void GenericDataWindow::activateValue(QTreeWidgetItem *selected, QTreeWidgetItem *lastSelected)
{
  if(inputs_.count(selected)==0) return;

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
  QDoubleSpinBox* valueWidgets[4] =
  { ui_.xValue, ui_.yValue, ui_.zValue, ui_.wValue };

  // hide component widgets
  for(int i=0; i<4; ++i) {
    labelWidgets[i]->hide();
    valueWidgets[i]->hide();
  }

  GLuint count = selectedInput_->valsPerElement();
  GLfloat *boundMax = &maxBounds_[selectedInput_].x;
  GLfloat *boundMin = &minBounds_[selectedInput_].x;
  GLint *precision = &precisions_[selectedInput_].x;
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
    valueWidgets[i]->setMinimum(boundMin[i]);
    valueWidgets[i]->setMaximum(boundMax[i]);
    valueWidgets[i]->setDecimals(precision[i]);
    if(precision[i]==0) {
      valueWidgets[i]->setSingleStep(1.0);
    }
    else {
      valueWidgets[i]->setSingleStep( pow(0.1, 1 + precision[i]/2) );
    }
    valueWidgets[i]->setValue(v);
  }
  // hide others
  for(; i<4; ++i) {
    labelWidgets[i]->hide();
    valueWidgets[i]->hide();
  }
}
