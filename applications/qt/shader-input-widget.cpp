/*
 * generic-data-window.cpp
 *
 *  Created on: 16.03.2013
 *      Author: daniel
 */

#include <QCloseEvent>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>

#include "shader-input-widget.h"
using namespace regen;

static const string __typeString(GLenum dataType) {
  switch(dataType) {
  case GL_FLOAT: return "float";
  case GL_INT: return "int";
  case GL_UNSIGNED_INT: return "unsigned int";
  default: return REGEN_STRING(dataType);
  }
}

class SetValueCallback : public Animation
{
public:
  SetValueCallback() : Animation(GL_TRUE,GL_FALSE) {}

  void push(ShaderInput *in, GLuint index, const GLfloat &value)
  {
    lock();
    if(!isRunning()) startAnimation();
    values_.push(ChangedValue(in,index,value));
    unlock();
  }

  void glAnimate(RenderState *rs, GLdouble dt)
  {
    while(!values_.isEmpty())
    {
      lock();
      setValue(values_.top());
      values_.pop();
      unlock();
    }

    lock();
    // stop anim if value stack is empty right now
    if(values_.isEmpty())
    { stopAnimation(); }
    unlock();
  }

private:
  struct ChangedValue {
    ChangedValue(ShaderInput *_in, GLuint _index, const GLfloat &_value)
    : in(_in), index(_index), value(_value) {}
    ShaderInput *in;
    GLuint index;
    GLfloat value;
  };
  Stack<ChangedValue> values_;

  void setValue(const ChangedValue &v)
  {
    switch(v.in->dataType()) {
    case GL_FLOAT: {
      ((GLfloat*)v.in->dataPtr())[v.index] = v.value;
      break;
    }
    case GL_INT: {
      ((GLint*)v.in->dataPtr())[v.index] = (GLint)v.value;
      break;
    }
    case GL_UNSIGNED_INT: {
      ((GLuint*)v.in->dataPtr())[v.index] = (GLuint)v.value;
      break;
    }
    default:
      REGEN_WARN("unknown data type " << v.in->dataType());
      break;
    }
    v.in->nextStamp();
  }
};

ShaderInputWidget::ShaderInputWidget(QWidget *parent)
: QWidget(parent)
{
  ui_.setupUi(this);

  selectedItem_ = NULL;
  selectedInput_ = NULL;
  ignoreValueChanges_ = GL_FALSE;

  setValueCallback_ = ref_ptr<SetValueCallback>::alloc();
}
ShaderInputWidget::~ShaderInputWidget()
{
  for(map<ShaderInput*,byte*>::iterator
      it=initialValue_.begin(); it!=initialValue_.end(); ++it)
  {
    delete []it->second;
  }
  initialValue_.clear();
}

void ShaderInputWidget::add(
    const string &treePath,
    const ref_ptr<ShaderInput> &in,
    const Vec4f &minBound,
    const Vec4f &maxBound,
    const Vec4i &precision,
    const string &description)
{
  if(initialValue_.count(in.get())>0) {
    byte *lastValue = initialValue_[in.get()];
    delete []lastValue;
  }
  byte *initialValue = new byte[in->inputSize()];
  memcpy(initialValue, in->data(), in->inputSize()*sizeof(byte));
  initialValue_[in.get()] = initialValue;

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
    ignoreValueChanges_ = GL_TRUE;
    activateValue(childItem,NULL);
    ignoreValueChanges_ = GL_FALSE;
  }
}

void ShaderInputWidget::setValue(GLint v, GLint index)
{
  if(selectedInput_ == NULL) return;

  QSlider* valueWidgets[4] =
  { ui_.xValue, ui_.yValue, ui_.zValue, ui_.wValue };
  QLabel* valueLabelWidgets[4] =
  { ui_.xValueLabel, ui_.yValueLabel, ui_.zValueLabel, ui_.wValueLabel };
  string valueLabel="0.0";

  GLfloat sliderMax = (GLfloat)valueWidgets[index]->maximum();
  GLfloat sliderMin = (GLfloat)valueWidgets[index]->minimum();
  GLfloat sliderValue = (GLfloat)v;
  // map slider value to [0,1]
  GLfloat p = (sliderValue-sliderMin)/(sliderMax-sliderMin);
  // and map to shader input value range
  GLfloat *boundMax = &maxBounds_[selectedInput_].x;
  GLfloat *boundMin = &minBounds_[selectedInput_].x;
  GLfloat inputValue = boundMin[index] + p*(boundMax[index] - boundMin[index]);

  if(!ignoreValueChanges_) {
    ((SetValueCallback*)setValueCallback_.get())->push(
        selectedInput_, index, inputValue);
  }

  GLenum dataType = selectedInput_->dataType();
  switch(dataType) {
  case GL_FLOAT: {
    valueLabel = REGEN_STRING(inputValue);
    break;
  }
  case GL_INT: {
    valueLabel = REGEN_STRING((GLint)inputValue);
    break;
  }
  case GL_UNSIGNED_INT: {
    valueLabel = REGEN_STRING((GLuint)inputValue);
    break;
  }
  default:
    REGEN_WARN("unknown data type " << dataType);
    break;
  }

  // update value label
  valueLabelWidgets[index]->setText(valueLabel.c_str());
}

//////////////////////////////
//////// Slots
//////////////////////////////

void ShaderInputWidget::setXValue(int v)
{ setValue(v,0); }
void ShaderInputWidget::setYValue(int v)
{ setValue(v,1); }
void ShaderInputWidget::setZValue(int v)
{ setValue(v,2); }
void ShaderInputWidget::setWValue(int v)
{ setValue(v,3); }

void ShaderInputWidget::resetValue()
{
  if(initialValue_.count(selectedInput_)==0) {
    REGEN_WARN("no initial value set.");
    return;
  }
  byte* initialValue = initialValue_[selectedInput_];
  selectedInput_->setUniformDataUntyped(initialValue);
  activateValue(selectedItem_,selectedItem_);
}

void ShaderInputWidget::activateValue(QTreeWidgetItem *selected, QTreeWidgetItem*)
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
  QSlider* valueWidgets[4] =
  { ui_.xValue, ui_.yValue, ui_.zValue, ui_.wValue };
  QWidget* valueContainer[4] =
  { ui_.xValueWidget, ui_.yValueWidget, ui_.zValueWidget, ui_.wValueWidget };

  // hide component widgets
  for(int i=0; i<4; ++i) {
    labelWidgets[i]->hide();
    valueContainer[i]->hide();
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
    valueContainer[i]->show();
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
      REGEN_WARN("unknown data type " << selectedInput_->dataType());
      break;
    }

    if(selectedInput_->dataType() == GL_FLOAT)
    {
      GLint decimals = REGEN_STRING((int)boundMax[i]).length() + precision[i];
      GLuint max=0, curr=1;
      for(; decimals>0; --decimals) {
        max+=9*curr; curr*=10;
      }
      valueWidgets[i]->setMinimum(0);
      valueWidgets[i]->setMaximum(max);

      // map value to [0,1]
      v = (v-boundMin[i])/(boundMax[i]-boundMin[i]);
      // and map to slider range
      valueWidgets[i]->setValue((GLint)(v*max));
    }
    else
    {
      valueWidgets[i]->setMinimum((int)boundMin[i]);
      valueWidgets[i]->setMaximum((int)boundMax[i]);
      valueWidgets[i]->setValue((int)v);
    }
    /*
    if(precision[i]==0) {
      valueWidgets[i]->setSingleStep(1.0);
    }
    else {
      valueWidgets[i]->setSingleStep( pow(0.1, 1 + precision[i]/2) );
    }
    */
  }
  // hide others
  for(; i<4; ++i) {
    labelWidgets[i]->hide();
    valueContainer[i]->hide();
  }
}
