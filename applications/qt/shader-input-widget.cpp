
#include <QCloseEvent>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/states/fbo-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/light-pass.h>
#include <stack>
#include <QPropertyAnimation>

#include "shader-input-widget.h"
#include "regen/animations/animation-manager.h"
#include "regen/sky/sky.h"
#include "VectorWidget.h"
#include "ColorWidget.h"
#include "TextureWidget.h"

using namespace regen;

ShaderInputWidget::ShaderInputWidget(QtApplication *app, QWidget *parent)
		: QWidget(parent), app_(app),
		  selectedItem_(nullptr),
		  ignoreValueChanges_(GL_FALSE) {
	ui_.setupUi(this);
	QList<int> sizes;
	sizes << 25 << 75;
	ui_.splitter->setSizes(sizes);
	ui_.parameterWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

ShaderInputWidget::~ShaderInputWidget() {
	for (auto &it: initialValue_) {
		delete[]it.second;
	}
	initialValue_.clear();
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

void ShaderInputWidget::resetParameter(ShaderInput *input) {
	if (initialValue_.count(input) == 0) {
		REGEN_WARN("No initial value set.");
		return;
	}
	byte *initialValue = initialValue_[input];
	input->writeVertex(0, initialValue);
	// TODO: Only update the changed value.
	activateValue(selectedItem_, selectedItem_);
}

void ShaderInputWidget::setNode(const ref_ptr<StateNode> &node) {
	ui_.treeWidget->clear();

	auto *item = new QTreeWidgetItem;
	item->setText(0, "Scene Graph");
	item->setExpanded(true);
	ui_.treeWidget->addTopLevelItem(item);

	rootNode_ = node;
	handleNode(node, item);
	item->setExpanded(true);

	auto *animWidget = new QTreeWidgetItem(item);
	animWidget->setText(0, "animations");
	animWidget->setExpanded(true);
	int index = 0;
	std::set<Animation *> allAnimations;
	for (auto &anim: AnimationManager::get().synchronizedAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim: AnimationManager::get().unsynchronizedAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim: AnimationManager::get().graphicsAnimations()) {
		allAnimations.insert(anim);
	}
	for (auto &anim: allAnimations) {
		if (!isValidState(anim->animationState().get())) {
			continue;
		}
		auto *animItem = new QTreeWidgetItem(animWidget);
		// use index as name for now
		std::string animName;
		if (anim->hasAnimationName()) {
			animName = anim->animationName();
		} else {
			animName = REGEN_STRING("animation-" << (index++));
		}
		animItem->setText(0, QString::fromStdString(animName));
		animItem->setExpanded(true);
		animations_[animItem] = anim;

		auto *skyAnim = dynamic_cast<const Sky *>(anim);
		if (skyAnim != nullptr) {
			for (auto &layer: skyAnim->layer()) {
				if (!isValidState(layer->updateState().get())) {
					continue;
				}
				auto *layerItem = new QTreeWidgetItem(animItem);
				layerItem->setText(0, layer->name().c_str());
				nodes_[layerItem] = layer;
			}
		}
	}
}

bool ShaderInputWidget::handleNode(
		const ref_ptr<StateNode> &node,
		QTreeWidgetItem *parent) {
	bool isEmpty = !isValidState(node->state().get());

	QTreeWidgetItem *x = parent;
	GLuint level = 0u;
	while (x != nullptr) {
		level += 1u;
		x = x->parent();
	}

	for (const auto &it: node->childs()) {
		auto *child = new QTreeWidgetItem(parent);
		child->setText(0, QString::fromStdString(it->name()));
		child->setExpanded(level < 4);
		if (handleNode(it, child)) {
			isEmpty = false;
		} else {
			delete child;
		}
	}

	nodes_[parent] = node;

	return !isEmpty;
}

void ShaderInputWidget::activateValue(QTreeWidgetItem *selected, QTreeWidgetItem *_) {
	ignoreValueChanges_ = GL_TRUE;
	selectedItem_ = selected;

	while (ui_.parameterLayout->count() > 0) {
		auto x = ui_.parameterLayout->takeAt(0);
		delete x->widget();
		delete x;
	}

	if (nodes_.count(selected) > 0) {
		auto selectedNode = nodes_[selected];
		ui_.nameValue->setText(selectedNode->name().c_str());
		handleState(selectedNode, selectedNode->state(), selectedItem_);
	}
	else if (animations_.count(selected) > 0) {
		auto selectedAnim = animations_[selected];
		ui_.nameValue->setText(selectedAnim->animationName().c_str());
		handleState(rootNode_, selectedAnim->animationState(), selectedItem_);
	}
	else {
		REGEN_WARN("No item found for selected node.");
	}

	ignoreValueChanges_ = GL_FALSE;
}

bool ShaderInputWidget::handleState(
		const ref_ptr<StateNode> &node,
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
		ref_ptr<InputContainer> container = hasInput->inputContainer();
		const ShaderInputList &inputs = container->inputs();
		for (const auto &namedInput: inputs) {
			if (namedInput.in_->numVertices() > 1) continue;
			if (addParameter(node, namedInput, parent)) isEmpty = false;
		}
	}

	auto *texState = dynamic_cast<TextureState *>(state.get());
	if (texState != nullptr) {
		if (addParameter(node, NamedShaderInput(texState->texture()), parent)) {
			isEmpty = false;
		}
	}

	auto *lightPass = dynamic_cast<LightPass *>(state.get());
	if (lightPass != nullptr) {
		for (auto &light: lightPass->lights()) {
			if (handleState(node, light.light, parent)) {
				isEmpty = false;
			}
			for (auto &input: light.inputs) {
				if (addParameter(node, NamedShaderInput(input), parent)) {
					isEmpty = false;
				}
			}
		}
	}

	for (const auto &it: state->joined()) {
		if (handleState(node, it, parent)) {
			isEmpty = false;
		}
	}

	if (isEmpty) {
		return false;
	} else {
		return true;
	}
}

template<class WidgetType>
QWidget *createParameterWidget(QtApplication *app, QWidget *parent,
		const ref_ptr<StateNode> &node,
		const ref_ptr<ShaderInput> &input) {
	auto *widget = new WidgetType({app, node, input}, parent);
	widget->show();
	return widget;
}

bool ShaderInputWidget::isValidNode(const StateNode *n) {
	std::stack<const StateNode *> nodes;
	nodes.push(n);
	while (!nodes.empty()) {
		const StateNode *node = nodes.top();
		nodes.pop();
		if (isValidState(node->state().get())) {
			return true;
		}
		for (const auto &it: node->childs()) {
			nodes.push(it.get());
		}
	}
	return false;
}

bool ShaderInputWidget::isValidState(const State *rootState) {
	std::stack<const State *> states;
	states.push(rootState);
	while (!states.empty()) {
		const State *state = states.top();
		states.pop();
		if (dynamic_cast<const FBOState *>(state) ||
			dynamic_cast<const ModelTransformation *>(state) ||
			dynamic_cast<const Camera *>(state)) {
			continue;
		}
		auto *hasInput = dynamic_cast<const HasInput *>(state);
		if (hasInput != nullptr) {
			ref_ptr<InputContainer> container = hasInput->inputContainer();
			const ShaderInputList &inputs = container->inputs();
			for (const auto &namedInput: inputs) {
				if (namedInput.in_->numVertices() > 1) continue;
				if (isValidParameter(namedInput.in_.get())) return true;
			}
		}
		auto *lightPass = dynamic_cast<const LightPass *>(state);
		if (lightPass != nullptr) {
			for (auto &light: lightPass->lights()) {
				for (auto &input: light.inputs) {
					if (isValidParameter(input.get())) return true;
				}
			}
		}
		auto *texState = dynamic_cast<const TextureState *>(state);
		if (texState != nullptr) {
			return true;
		}
		for (const auto &it: state->joined()) {
			states.push(it.get());
		}
	}
	return false;
}

bool ShaderInputWidget::isValidParameter(const ShaderInput *input) {
	if (input->valsPerElement() > 4) return false;

	if (input->isBufferBlock()) {
		auto *block = dynamic_cast<const UBO *>(input);
		if (block && block->name() == "GlobalUniforms") {
			return false;
		}
		bool hasInputs = false;
		if (block) {
			for (auto &uniform: block->blockInputs()) {
				hasInputs = isValidParameter(uniform.in_.get()) || hasInputs;
			}
		}
		return hasInputs;
	}

	// TODO: maybe rather use "editable" flag?
	if (input->name() == "viewport") return false;
	if (input->name() == "inverseViewport") return false;

	return true;
}

bool ShaderInputWidget::addParameter(
		const ref_ptr<StateNode> &node,
		const NamedShaderInput &namedInput,
		QTreeWidgetItem *parent) {
	const ref_ptr<ShaderInput> in = namedInput.in_;
	if (in->valsPerElement() > 4) return false;

	if (in->isBufferBlock()) {
		auto *block = dynamic_cast<UBO *>(in.get());
		if (block && block->name() == "GlobalUniforms") {
			return false;
		}
		bool hasInputs = false;
		if (block) {
			for (auto &uniform: block->blockInputs()) {
				hasInputs = addParameter(node, uniform, parent) || hasInputs;
			}
		}
		return hasInputs;
	}

	// TODO: maybe rather use "editable" flag?
	if (in->name() == "viewport") return false;
	if (in->name() == "inverseViewport") return false;

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

    QWidget *parameterWidget = nullptr;
    QString icon;
    auto &schema = in->schema();
    switch (schema.semantics()) {
        case InputSchema::COLOR:
            parameterWidget = createParameterWidget<ColorWidget>(app_, ui_.parameterWidget, node, namedInput.in_);
            icon = "\uf1fb"; // paint brush
            break;
        case InputSchema::TEXTURE:
            parameterWidget = createParameterWidget<TextureWidget>(app_, ui_.parameterWidget, node, namedInput.in_);
            icon = "\uf03e"; // image
            break;
        case InputSchema::UNKNOWN: {
            auto *tex = dynamic_cast<Texture *>(namedInput.in_.get());
            if (tex) {
                parameterWidget = createParameterWidget<TextureWidget>(app_, ui_.parameterWidget, node, namedInput.in_);
                icon = "\uf03e"; // image
                break;
            }
        }
        default:
            parameterWidget = createParameterWidget<VectorWidget>(app_, ui_.parameterWidget, node, namedInput.in_);
            icon = "\uf201"; // line chart
            break;
    }
    addParameterWidget_(parameterWidget, namedInput, icon);

	return true;
}

static QPushButton *createButton(const QString &text, QWidget *parent) {
	auto *button = new QPushButton(text, parent);
	QFont font = button->font();
	font.setFamily("FontAwesome");
	button->setFont(font);
	button->setFlat(true);
	button->setStyleSheet(
			"QPushButton {"
			"    border: none;"
			"    border-radius: 8px;"
			"}"
			"QPushButton:hover {"
			"    background-color: #3e8e91;"
			"}"
	);
	int buttonSize = font.pointSize() * 3;
	button->setFixedSize(buttonSize, buttonSize);
	return button;
}

void changeButtonIconColor(QPushButton *button, const QColor &color) {
	QString qss = QString(
			"QPushButton {"
			"    color: %1;"
			"    background-color: transparent;"
			"    border: none;"
			"    border-radius: 8px;"
			"}"
			"QPushButton:hover { color: %2; background-color: #3e8e91; }"
	).arg(color.name()).arg(color.darker().name());
	button->setStyleSheet(qss);
}

void handleScalarParameter_(QWidget *parameterWidget, const NamedShaderInput &namedInput, QHBoxLayout *titleBar) {
	auto *scalarWidget = dynamic_cast<VectorWidget *>(parameterWidget);
	// create a slider for X component, it will be synchronized with the other widgets
	auto *slider = scalarWidget->createSlider(0);
	// make some size constraints to make it look good in the title bar
	slider->setMinimumWidth(140);
	slider->setMaximumWidth(140);
	slider->setMinimumHeight(20);
	// add the slider to the title bar
	titleBar->addWidget(slider);
}

void ShaderInputWidget::handleTextureParameter_(QWidget *parameterWidget, const NamedShaderInput &namedInput, QHBoxLayout *titleBar) {
	auto *textureWidget = dynamic_cast<TextureWidget *>(parameterWidget);
	// create a button that opens a file dialog
	if (textureWidget->hasTextureFile()) {
		auto *fileButton = createButton("\uf07b", parameterWidget);
		titleBar->addWidget(fileButton);
		connect(fileButton, &QPushButton::clicked, textureWidget, [textureWidget]() {
			textureWidget->openFile();
		});
	}
}

static void addSeparator(QHBoxLayout *titleBar) {
	auto *separator = new QFrame();
	separator->setFrameShape(QFrame::VLine);
	separator->setFrameShadow(QFrame::Plain);
	separator->setLineWidth(2);
	separator->setMaximumHeight(20);
	separator->setMinimumWidth(12);
	separator->setStyleSheet("color: #d0d0d0;");
	titleBar->addWidget(separator);
}

void ShaderInputWidget::addParameterWidget_(QWidget *parameterWidget, const NamedShaderInput &namedInput, const QString &icon) {
    bool isTexture = dynamic_cast<Texture *>(namedInput.in_.get()) != nullptr;

    auto *titleBar = new QHBoxLayout();
    titleBar->setContentsMargins(6, 0, 0, 0);
    titleBar->setSpacing(0);

	auto *iconLabel = new QLabel(ui_.parameterWidget);
	// Set the FontAwesome icon
	iconLabel->setText(icon);
	QFont iconFont = iconLabel->font();
	iconFont.setFamily("FontAwesome");
	iconFont.setPointSize(10); // Set the font size to scale the icon
	iconLabel->setFont(iconFont);
	iconLabel->setFixedSize(36, 36);
	iconLabel->setMargin(6);
	titleBar->addWidget(iconLabel);
    addSeparator(titleBar);

    // Create the name label
    auto *nameLabel = new QLabel(QString::fromStdString(namedInput.in_->name()), ui_.parameterWidget);
    QFont labelFont = nameLabel->font();
    labelFont.setPointSize(8); // Set smaller font size
    labelFont.setBold(true); // Set bold font
    nameLabel->setMargin(6);
    nameLabel->setFont(labelFont);
    nameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    nameLabel->setContentsMargins(0, 0, 0, 0);
    titleBar->addWidget(nameLabel);

    // Add a horizontal spacer
    titleBar->addStretch();

    // Create the button bar
    auto &schema = namedInput.in_->schema();
    switch (schema.semantics()) {
        case InputSchema::COLOR: {
            auto *colorWidget = dynamic_cast<ColorWidget *>(parameterWidget);
            auto *colorPickerButton = createButton("\uF0C8", ui_.parameterWidget);
            titleBar->addWidget(colorPickerButton);
            connect(colorWidget, &ColorWidget::colorChanged, this, [colorPickerButton](const QColor &color) {
                changeButtonIconColor(colorPickerButton, color);
            });
            connect(colorPickerButton, &QPushButton::clicked, this, [colorWidget]() {
                colorWidget->pickColor();
            });
            colorWidget->initializeColor();
            break;
        }
        case InputSchema::ALPHA:
        case InputSchema::SCALAR:
            handleScalarParameter_(parameterWidget, namedInput, titleBar);
            break;
        case InputSchema::TEXTURE:
            if (isTexture) {
                handleTextureParameter_(parameterWidget, namedInput, titleBar);
            } else {
                REGEN_WARN("Invalid texture input '" << namedInput.in_->name() << "'.");
            }
            break;
        case InputSchema::UNKNOWN:
        case InputSchema::VECTOR:
            if (isTexture) {
                handleTextureParameter_(parameterWidget, namedInput, titleBar);
            } else if (namedInput.in_->valsPerElement() == 1) {
                handleScalarParameter_(parameterWidget, namedInput, titleBar);
            }
            break;
        default:
            break;
    }
    addSeparator(titleBar);

    // Add a reset button to the title bar
    auto *resetButton = createButton("\uf0e2", ui_.parameterWidget);
    titleBar->addWidget(resetButton);

    // Add a toggle button to the title bar
    auto *toggleButton = createButton("\uf0da", ui_.parameterWidget); // Down arrow icon
    titleBar->addWidget(toggleButton);

    // Set the initial state to unexpanded
    parameterWidget->setVisible(false);
    parameterWidget->setMaximumHeight(0);

    auto *titleBarWidget = new QWidget(ui_.parameterWidget);
    titleBarWidget->setLayout(titleBar);

    // Set size policy to prevent expansion
    nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    parameterWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    parameterWidget->setContentsMargins(0, 0, 0, 0);

    // Add the row to the parameter layout
    ui_.parameterLayout->addWidget(titleBarWidget, 0, Qt::AlignTop);
    ui_.parameterLayout->addWidget(parameterWidget, 0, Qt::AlignTop);

    // Add a spacer widget to separate the parameter widgets
    auto *spacer = new QFrame(ui_.parameterWidget);
    spacer->setFrameShape(QFrame::HLine);
    spacer->setFrameShadow(QFrame::Sunken);
    spacer->setFixedHeight(12);
    ui_.parameterLayout->addWidget(spacer);

    // Connect the reset button to the resetParameter function
    connect(resetButton, &QPushButton::clicked, this, [this, namedInput]() {
        resetParameter(namedInput.in_.get());
    });

    // Connect the toggle button to a slot
    connect(toggleButton, &QPushButton::clicked, this, [parameterWidget, toggleButton]() {
        bool isVisible = parameterWidget->isVisible();
        toggleButton->setText(isVisible ? "\uf0da" : "\uf0d7"); // Down arrow / Up arrow

        auto *animation = new QPropertyAnimation(parameterWidget, "maximumHeight");
        animation->setDuration(100); // Faster duration in milliseconds
        animation->setStartValue(isVisible ? parameterWidget->sizeHint().height() : 0);
        animation->setEndValue(isVisible ? 0 : parameterWidget->sizeHint().height());

        if (isVisible) {
            connect(animation, &QPropertyAnimation::finished, parameterWidget, [parameterWidget]() {
                parameterWidget->setVisible(false);
            });
        } else {
            parameterWidget->setVisible(true);
        }
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    });
}
