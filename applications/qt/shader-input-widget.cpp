
#include <QCloseEvent>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/states/fbo-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/texture-state.h>
#include <regen/states/material-state.h>
#include <regen/states/model-transformation.h>
#include <regen/states/camera.h>
#include <regen/states/light-pass.h>
#include <regen/states/direct-shading.h>

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
  SetValueCallback(map<ShaderInput*,GLuint> *valueStamps)
  : Animation(GL_TRUE,GL_FALSE), valueStamps_(valueStamps) {}

  void push(ShaderInput *in, byte *changedData)
  {
    lock();
    if(!isRunning()) startAnimation();
    values_.push(ChangedValue(in,changedData));
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
    ChangedValue(ShaderInput *_in, byte *_changedData)
    : in(_in), changedData(_changedData) {}
    ShaderInput *in;
    byte *changedData;
  };
  Stack<ChangedValue> values_;
  map<ShaderInput*,GLuint> *valueStamps_;

  void setValue(const ChangedValue &v)
  {
    v.in->setUniformDataUntyped(v.changedData);
    (*valueStamps_)[v.in] = v.in->stamp();
    delete []v.changedData;
  }
};

ShaderInputWidget::ShaderInputWidget(QWidget *parent)
: QWidget(parent)
{
  ui_.setupUi(this);

  selectedItem_ = NULL;
  selectedInput_ = NULL;
  ignoreValueChanges_ = GL_FALSE;

  setValueCallback_ = ref_ptr<SetValueCallback>::alloc(&valueStamp_);
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

void ShaderInputWidget::setNode(const ref_ptr<StateNode> &node)
{
  ui_.treeWidget->clear();

  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText(0, "Scene Graph");
  ui_.treeWidget->addTopLevelItem(item);

  handleNode(node,item);
  item->setExpanded(true);
}

bool ShaderInputWidget::handleNode(
    const ref_ptr<StateNode> &node,
    QTreeWidgetItem *parent)
{
  bool isEmpty = true;

  if(handleState(node->state(), parent)) isEmpty = false;

  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    QTreeWidgetItem *child = new QTreeWidgetItem(parent);
    child->setText(0, QString::fromStdString((*it)->name()));
    if(handleNode(*it, child)) {
      isEmpty = false;
    }
    else {
      delete child;
    }
  }

  return !isEmpty;
}

bool ShaderInputWidget::handleState(
    const ref_ptr<State> &state,
    QTreeWidgetItem *parent)
{
  bool isEmpty = true;

  QTreeWidgetItem *child = new QTreeWidgetItem(parent);
  if(dynamic_cast<FBOState*>(state.get())) {
    child->setText(0, "FBOState");
  }
  else if(dynamic_cast<ShaderState*>(state.get())) {
    child->setText(0, "ShaderState");
  }
  else if(dynamic_cast<TextureState*>(state.get())) {
    child->setText(0, "TextureState");
  }
  else if(dynamic_cast<Material*>(state.get())) {
    child->setText(0, "Material");
  }
  else if(dynamic_cast<Camera*>(state.get())) {
    child->setText(0, "Camera");
  }
  else if(dynamic_cast<ModelTransformation*>(state.get())) {
    child->setText(0, "ModelTransformation");
  }
  else if(dynamic_cast<StateSequence*>(state.get())) {
    child->setText(0, "StateSequence");
  }
  else if(dynamic_cast<LightPass*>(state.get())) {
    child->setText(0, "LightPass");
  }
  else if(dynamic_cast<DirectShading*>(state.get())) {
    child->setText(0, "DirectShading");
  }
  else {
    child->setText(0, "State");
  }

  for(list< ref_ptr<State> >::const_iterator
      it=state->joined().begin(); it!=state->joined().end(); ++it)
  {
    if(handleState(*it, child)) isEmpty = false;
  }

  HasInput *hasInput = dynamic_cast<HasInput*>(state.get());
  if(hasInput != NULL) {
    ref_ptr<ShaderInputContainer> container = hasInput->inputContainer();
    const ShaderInputList &inputs = container->inputs();
    for(ShaderInputList::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      const NamedShaderInput &namedInput = *it;
      if(namedInput.in_->numVertices()>1) continue;
      if(handleInput(namedInput,child)) isEmpty = false;
    }
  }

  if(isEmpty) {
    delete child;
    return false;
  }
  else {
    return true;
  }
}

bool ShaderInputWidget::handleInput(
    const NamedShaderInput &namedInput,
    QTreeWidgetItem *parent)
{
  const ref_ptr<ShaderInput> in = namedInput.in_;
  if(in->elementCount()>1) return false;
  if(in->valsPerElement()>4) return false;

  if(initialValue_.count(in.get())>0) {
    byte *lastValue = initialValue_[in.get()];
    delete []lastValue;
  }
  byte *initialValue = new byte[in->inputSize()];
  memcpy(initialValue, in->clientData(), in->inputSize());
  initialValue_[in.get()] = initialValue;
  initialValueStamp_[in.get()] = in->stamp();
  valueStamp_[in.get()] = 0;

  QTreeWidgetItem *inputItem = new QTreeWidgetItem(parent);
  inputItem->setText(0, QString::fromStdString(namedInput.name_));
  inputs_[inputItem] = in;

  if(selectedInput_ == NULL) {
    activateValue(inputItem,NULL);
  }

  return true;
}

void ShaderInputWidget::updateInitialValue(ShaderInput *x)
{
  GLuint stamp = x->stamp();
  if(stamp!=valueStamp_[x] &&
     stamp!=initialValueStamp_[x])
  {
    // last time value was not changed from widget
    // update initial data
    byte *initialValue = new byte[x->inputSize()];
    memcpy(initialValue, x->clientData(), x->inputSize()*sizeof(byte));

    byte *oldInitial = initialValue_[x];
    delete []oldInitial;
    initialValue_[x] = initialValue;
    initialValueStamp_[x] = stamp;
  }
}

//////////////////////////////
//////// Slots
//////////////////////////////

template<class T> byte* createData(
    ShaderInput *in,
    QLineEdit **valueWidgets,
    GLuint count)
{
  T *typedData = new T[count];
  for(GLuint i=0u; i<count; ++i) {
    QLineEdit *widget = valueWidgets[i];
    stringstream ss(widget->text().toStdString());
    ss >> typedData[i];
  }
  return (byte*)typedData;
}

void ShaderInputWidget::valueUpdated()
{
  if(ignoreValueChanges_) return;
  if(selectedInput_==NULL) {
    REGEN_WARN("valueUpdated() called but no ShaderInput selected.");
    return;
  }

  QLineEdit* valueWidgets[4] =
  { ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit };

  GLuint count = selectedInput_->valsPerElement();
  if(count>4) {
    REGEN_WARN("More then 4 components unsupported.");
    return;
  }

  byte *changedData = NULL;
  switch(selectedInput_->dataType()) {
  case GL_FLOAT:
    changedData = createData<GLfloat>(selectedInput_, valueWidgets, count);
    break;
  case GL_INT:
    changedData = createData<GLint>(selectedInput_, valueWidgets, count);
    break;
  case GL_UNSIGNED_INT:
    changedData = createData<GLuint>(selectedInput_, valueWidgets, count);
    break;
  default:
    REGEN_WARN("Unknown data type " << selectedInput_->dataType());
    break;
  }

  ((SetValueCallback*)setValueCallback_.get())->push(selectedInput_, changedData);
}

void ShaderInputWidget::resetValue()
{
  if(initialValue_.count(selectedInput_)==0) {
    REGEN_WARN("No initial value set.");
    return;
  }
  updateInitialValue(selectedInput_);
  byte* initialValue = initialValue_[selectedInput_];
  selectedInput_->setUniformDataUntyped(initialValue);
  activateValue(selectedItem_,selectedItem_);
}

void ShaderInputWidget::activateValue(QTreeWidgetItem *selected, QTreeWidgetItem *_)
{
  if(inputs_.count(selected)==0) {
    return; // Not a ShaderInput item.
  }
  ignoreValueChanges_ = GL_TRUE;
  GLuint i;

  // remember selection
  selectedItem_ = selected;
  selectedInput_ = inputs_[selectedItem_].get();
  REGEN_INFO("activateValue " << selectedInput_->name());

  ui_.nameValue->setText(selectedInput_->name().c_str());
  ui_.typeValue->setText(__typeString(selectedInput_->dataType()).c_str());

  QLabel* labelWidgets[4] =
  { ui_.xLabel, ui_.yLabel, ui_.zLabel, ui_.wLabel };
  QLineEdit* valueWidgets[4] =
  { ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit };

  // hide component widgets
  for(i=0; i<4; ++i) {
    labelWidgets[i]->hide();
    valueWidgets[i]->hide();
  }

  GLuint count = selectedInput_->valsPerElement();
  if(count>4) {
    REGEN_WARN("More then 4 components unsupported.");
    return;
  }
  byte *value = selectedInput_->clientDataPtr();

  // show and set active components
  for(i=0; i<count; ++i) {
    labelWidgets[i]->show();
    valueWidgets[i]->show();

    string v;
    switch(selectedInput_->dataType()) {
    case GL_FLOAT: {
      v = REGEN_STRING( ((GLfloat*)value)[i] );
      break;
    }
    case GL_INT: {
      v = REGEN_STRING( ((GLint*)value)[i] );
      break;
    }
    case GL_UNSIGNED_INT: {
      v = REGEN_STRING( ((GLuint*)value)[i] );
      break;
    }
    default:
      REGEN_WARN("Unknown data type " << selectedInput_->dataType());
      break;
    }
    valueWidgets[i]->setText(QString::fromStdString(v));
  }

  ignoreValueChanges_ = GL_FALSE;
}
