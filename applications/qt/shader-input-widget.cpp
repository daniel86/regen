
#include <QCloseEvent>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/states/fbo-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/texture-state.h>
#include <regen/states/material-state.h>
#include <regen/states/model-transformation.h>
#include <regen/camera/camera.h>
#include <regen/states/light-pass.h>
#include <regen/states/direct-shading.h>

#include "shader-input-widget.h"
#include "regen/animations/animation-manager.h"

using namespace regen;
//using namespace std;

static const std::string __typeString(GLenum dataType) {
	switch (dataType) {
		case GL_FLOAT:
			return "float";
		case GL_INT:
			return "int";
		case GL_UNSIGNED_INT:
			return "unsigned int";
		default:
			return REGEN_STRING(dataType);
	}
}

class SetValueCallback : public Animation {
public:
	SetValueCallback(std::map<ShaderInput *, GLuint> *valueStamps)
			: Animation(GL_TRUE, GL_FALSE), valueStamps_(valueStamps) {}

	void push(ShaderInput *in, byte *changedData) {
		lock();
		if (!isRunning()) startAnimation();
		values_.push(ChangedValue(in, changedData));
		unlock();
	}

	void glAnimate(RenderState *rs, GLdouble dt) {
		while (!values_.isEmpty()) {
			lock();
			setValue(values_.top());
			values_.pop();
			unlock();
		}

		lock();
		// stop anim if value stack is empty right now
		if (values_.isEmpty()) { stopAnimation(); }
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
	std::map<ShaderInput *, GLuint> *valueStamps_;

	void setValue(const ChangedValue &v) {
		v.in->setUniformDataUntyped(v.changedData);
		(*valueStamps_)[v.in] = v.in->stamp();
		delete[]v.changedData;
	}
};

ShaderInputWidget::ShaderInputWidget(QWidget *parent)
		: QWidget(parent) {
	ui_.setupUi(this);

	selectedItem_ = NULL;
	selectedInput_ = NULL;
	ignoreValueChanges_ = GL_FALSE;

	setValueCallback_ = ref_ptr<SetValueCallback>::alloc(&valueStamp_);
}

ShaderInputWidget::~ShaderInputWidget() {
	for (auto it = initialValue_.begin(); it != initialValue_.end(); ++it) {
		delete[]it->second;
	}
	initialValue_.clear();
}

void ShaderInputWidget::setNode(const ref_ptr<StateNode> &node) {
	ui_.treeWidget->clear();

	auto *item = new QTreeWidgetItem;
	item->setText(0, "Scene Graph");
	item->setExpanded(true);
	ui_.treeWidget->addTopLevelItem(item);

	handleNode(node, item);
	item->setExpanded(true);

	auto *anims = new QTreeWidgetItem(item);
	anims->setText(0, "animations");
	int index = 0;
	for (auto &anim : AnimationManager::get().glAnimations()) {
		const auto& animShader = anim->shader();
		if (!animShader.get()) continue;
		auto shaderInputs = animShader->inputs();
		auto *animItem = new QTreeWidgetItem(anims);
		// use index as name for now
		std::string animName = REGEN_STRING("anim" << (index++));
		animItem->setText(0, animName.c_str());

		for(const auto& namedInput : shaderInputs) {
			if (namedInput.in_->numVertices() > 1) continue;
			handleInput(namedInput, animItem);
		}
	}
}

bool ShaderInputWidget::handleNode(
		const ref_ptr<StateNode> &node,
		QTreeWidgetItem *parent) {
	bool isEmpty = true;

	if (handleState(node->state(), parent)) isEmpty = false;

	QTreeWidgetItem *x = parent;
	GLuint level = 0u;
	while (x != nullptr) {
		level += 1u;
		x = x->parent();
	}

	for (auto it = node->childs().begin(); it != node->childs().end(); ++it) {
		auto *child = new QTreeWidgetItem(parent);
		child->setText(0, QString::fromStdString((*it)->name()));
		child->setExpanded(level < 5);
		if (handleNode(*it, child)) {
			isEmpty = false;
		} else {
			delete child;
		}
	}

	return !isEmpty;
}

bool ShaderInputWidget::handleState(
		const ref_ptr<State> &state,
		QTreeWidgetItem *parent) {
	bool isEmpty = true;

	if (dynamic_cast<FBOState *>(state.get()) ||
		dynamic_cast<ModelTransformation *>(state.get()) ||
		dynamic_cast<Camera *>(state.get())) {
		return false;
	}

	auto *hasInput = dynamic_cast<HasInput *>(state.get());
	if (hasInput != nullptr) {
		ref_ptr<ShaderInputContainer> container = hasInput->inputContainer();
		const ShaderInputList &inputs = container->inputs();
		for (auto it = inputs.begin(); it != inputs.end(); ++it) {
			const NamedShaderInput &namedInput = *it;
			if (namedInput.in_->numVertices() > 1) continue;
			if (handleInput(namedInput, parent)) isEmpty = false;
		}
	}

	auto *lightPass = dynamic_cast<LightPass *>(state.get());
	if (lightPass != nullptr) {
		for (auto &light : lightPass->lights()) {
			if(handleState(light.light, parent)) {
				isEmpty = false;
			}
		}
	}

	for (auto it = state->joined().begin(); it != state->joined().end(); ++it) {
		if (handleState(*it, parent)) {
			isEmpty = false;
		}
	}

	if (isEmpty) {
		return false;
	} else {
		return true;
	}
}

bool ShaderInputWidget::handleInput(
		const NamedShaderInput &namedInput,
		QTreeWidgetItem *parent) {
	const ref_ptr<ShaderInput> in = namedInput.in_;
	if (in->elementCount() > 1) return false;
	if (in->valsPerElement() > 4) return false;

	if (initialValue_.count(in.get()) > 0) {
		byte *lastValue = initialValue_[in.get()];
		delete[]lastValue;
	}
	byte *initialValue = new byte[in->inputSize()];
	memcpy(initialValue, in->clientData(), in->inputSize());
	initialValue_[in.get()] = initialValue;
	initialValueStamp_[in.get()] = in->stamp();
	valueStamp_[in.get()] = 0;

	auto *inputItem = new QTreeWidgetItem(parent);
	inputItem->setText(0, QString::fromStdString(namedInput.name_));
	inputs_[inputItem] = in;

	if (selectedInput_ == nullptr) {
		activateValue(inputItem, nullptr);
	}

	return true;
}

void ShaderInputWidget::updateInitialValue(ShaderInput *x) {
	GLuint stamp = x->stamp();
	if (stamp != valueStamp_[x] &&
		stamp != initialValueStamp_[x]) {
		// last time value was not changed from widget
		// update initial data
		byte *initialValue = new byte[x->inputSize()];
		memcpy(initialValue, x->clientData(), x->inputSize() * sizeof(byte));

		byte *oldInitial = initialValue_[x];
		delete[]oldInitial;
		initialValue_[x] = initialValue;
		initialValueStamp_[x] = stamp;
	}
}

//////////////////////////////
//////// Slots
//////////////////////////////

template<class T>
byte *createData(
		ShaderInput *in,
		QSlider **valueWidgets,
		QLabel **valueTexts,
		GLuint count) {
	T *typedData = new T[count];
	for (GLuint i = 0u; i < count; ++i) {
		QSlider *widget = valueWidgets[i];
		std::stringstream ss;
		ss << static_cast<float>(widget->value()) / 1000.0f;
		ss >> typedData[i];

		valueTexts[i]->setText(QString::number(typedData[i]));
	}
	return (byte *) typedData;
}

void ShaderInputWidget::valueUpdated() {
	if (ignoreValueChanges_) return;
	if (selectedInput_ == nullptr) {
		REGEN_WARN("valueUpdated() called but no ShaderInput selected.");
		return;
	}

	QSlider *valueWidgets[4] =
			{ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
	QLabel *valueTexts[4] =
			{ui_.xValueText, ui_.yValueText, ui_.zValueText, ui_.wValueText};

	GLuint count = selectedInput_->valsPerElement();
	if (count > 4) {
		REGEN_WARN("More then 4 components unsupported.");
		return;
	}

	byte *changedData = nullptr;
	switch (selectedInput_->dataType()) {
		case GL_FLOAT:
			changedData = createData<GLfloat>(selectedInput_, valueWidgets, valueTexts, count);
			break;
		case GL_INT:
			changedData = createData<GLint>(selectedInput_, valueWidgets, valueTexts, count);
			break;
		case GL_UNSIGNED_INT:
			changedData = createData<GLuint>(selectedInput_, valueWidgets, valueTexts, count);
			break;
		default: REGEN_WARN("Unknown data type " << selectedInput_->dataType());
			break;
	}

	((SetValueCallback *) setValueCallback_.get())->push(selectedInput_, changedData);
}

void ShaderInputWidget::maxUpdated() {
	if (selectedInput_ == nullptr) {
		REGEN_WARN("maxUpdated() called but no ShaderInput selected.");
		return;
	}
	QSlider *valueWidgets[4] =
			{ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
	GLuint count = selectedInput_->valsPerElement();
	if (count > 4) {
		REGEN_WARN("More then 4 components unsupported.");
		return;
	}
	GLfloat maxVal = ui_.maxValue->text().toFloat();
	GLboolean hasNegative = ui_.negativeValueToggle->isChecked();
	for (GLuint i = 0; i < count; ++i) {
		valueWidgets[i]->setMaximum(static_cast<int>(maxVal*1000.0f));
		if (hasNegative) {
			valueWidgets[i]->setMinimum(-static_cast<int>(maxVal*1000.0f));
		} else {
			valueWidgets[i]->setMinimum(0);
		}
	}
}

void ShaderInputWidget::resetValue() {
	if (initialValue_.count(selectedInput_) == 0) {
		REGEN_WARN("No initial value set.");
		return;
	}
	updateInitialValue(selectedInput_);
	byte *initialValue = initialValue_[selectedInput_];
	selectedInput_->setUniformDataUntyped(initialValue);
	activateValue(selectedItem_, selectedItem_);
}

void ShaderInputWidget::activateValue(QTreeWidgetItem *selected, QTreeWidgetItem *_) {
	if (inputs_.count(selected) == 0) {
		return; // Not a ShaderInput item.
	}
	ignoreValueChanges_ = GL_TRUE;
	GLuint i;

	// remember selection
	selectedItem_ = selected;
	selectedInput_ = inputs_[selectedItem_].get();

	ui_.nameValue->setText(selectedInput_->name().c_str());
	ui_.typeValue->setText(__typeString(selectedInput_->dataType()).c_str());

	QLabel *labelWidgets[4] =
			{ui_.xLabel, ui_.yLabel, ui_.zLabel, ui_.wLabel};
	QSlider *valueWidgets[4] =
			{ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
	QLabel *valueTexts[4] =
			{ui_.xValueText, ui_.yValueText, ui_.zValueText, ui_.wValueText};

	// hide component widgets
	for (i = 0; i < 4; ++i) {
		labelWidgets[i]->hide();
		valueWidgets[i]->hide();
		valueTexts[i]->hide();
	}

	GLuint count = selectedInput_->valsPerElement();
	if (count > 4) {
		REGEN_WARN("More then 4 components unsupported.");
		return;
	}
	byte *value = selectedInput_->clientDataPtr();

	GLfloat maxVal = 0.0f;
	GLboolean hasNegative = ui_.negativeValueToggle->isChecked();
	// show and set active components
	for (i = 0; i < count; ++i) {
		labelWidgets[i]->show();
		valueWidgets[i]->show();
		valueTexts[i]->show();
		valueWidgets[i]->setMaximum(999999999);

		GLfloat x = 0.0f;
		switch (selectedInput_->dataType()) {
			case GL_FLOAT: {
				x = ((GLfloat *) value)[i];
				break;
			}
			case GL_INT: {
				x = static_cast<GLfloat>((((GLint *) value)[i]));
				break;
			}
			case GL_UNSIGNED_INT: {
				x = static_cast<GLfloat>((((GLuint *) value)[i]));
				break;
			}
			default: REGEN_WARN("Unknown data type " << selectedInput_->dataType());
				break;
		}
		if (x < 0.0f) hasNegative = GL_TRUE;
		valueWidgets[i]->setValue(static_cast<int>(x*1000.0f));
		valueTexts[i]->setText(QString::number(x));
		if (fabs(x) > maxVal) maxVal = fabs(x);
	}

	maxVal *= 2.0f;
	if (maxVal < 1.0f) maxVal = 1.0f;
	ui_.maxValue->setText(QString::number(maxVal));
	for (i = 0; i < count; ++i) {
		valueWidgets[i]->setMaximum(static_cast<int>(maxVal*1000.0f));
		if (hasNegative) {
			valueWidgets[i]->setMinimum(-static_cast<int>(maxVal*1000.0f));
		} else {
			valueWidgets[i]->setMinimum(0);
		}
	}

	ignoreValueChanges_ = GL_FALSE;
}
