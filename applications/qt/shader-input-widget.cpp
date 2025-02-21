
#include <QCloseEvent>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/states/fbo-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/light-pass.h>

#include "shader-input-widget.h"
#include "regen/animations/animation-manager.h"

using namespace regen;

static std::string regen_typeString(GLenum dataType) {
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

ShaderInputWidget::ShaderInputWidget(QWidget *parent)
		: QWidget(parent) {
	ui_.setupUi(this);

	selectedItem_ = nullptr;
	selectedInput_ = nullptr;
	ignoreValueChanges_ = GL_FALSE;
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
	std::set<Animation *> allAnimations;
	for (auto &anim : AnimationManager::get().synchronizedAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim : AnimationManager::get().unsynchronizedAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim : AnimationManager::get().graphicsAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim : allAnimations) {
		auto *animItem = new QTreeWidgetItem(anims);
		// use index as name for now
		std::string animName;
		if (anim->hasAnimationName()) {
			animName = anim->animationName();
		} else {
			animName = REGEN_STRING("animation-" << (index++));
		}
		animItem->setText(0, QString::fromStdString(animName));
		handleState(anim->animationState(), animItem);
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
		for (const auto & namedInput : inputs) {
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
			for (auto &input : light.inputs) {
				if (handleInput(NamedShaderInput(input), parent)) {
					isEmpty = false;
				}
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
	if (in->valsPerElement() > 4) return false;

	if (in->isUniformBlock()) {
		auto *block = dynamic_cast<UniformBlock *>(in.get());
		for (auto &uniform : block->uniforms()) {
			handleInput(uniform, parent);
		}
		return true;
	}

	if (initialValue_.count(in.get()) > 0) {
		byte *lastValue = initialValue_[in.get()];
		delete[]lastValue;
	}
	auto clientData = in->mapClientDataRaw(ShaderData::READ);
	byte *initialValue = new byte[in->elementSize()];
	memcpy(initialValue, clientData.r, in->elementSize());
	clientData.unmap();
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
		auto clientData = x->mapClientDataRaw(ShaderData::READ);
		byte *initialValue = new byte[x->elementSize()];
		memcpy(initialValue, clientData.r, x->elementSize());

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

	selectedInput_->writeVertex(0, changedData);
	delete[] changedData;
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
	byte *initialValue = initialValue_[selectedInput_];
	selectedInput_->writeVertex(0, initialValue);
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
	ui_.typeValue->setText(regen_typeString(selectedInput_->dataType()).c_str());

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
	auto mapped = selectedInput_->mapClientDataRaw(ShaderData::READ);
	const byte *value = mapped.r;

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
