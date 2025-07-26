#include "VectorWidget.h"

using namespace regen;

static GLfloat readInputValue(const ref_ptr<ShaderInput> &input, const byte *value, GLuint i) {
	switch (input->baseType()) {
		case GL_FLOAT: {
			return ((GLfloat *) value)[i];
		}
		case GL_INT: {
			return static_cast<GLfloat>((((GLint *) value)[i]));
		}
		case GL_UNSIGNED_INT: {
			return static_cast<GLfloat>((((GLuint *) value)[i]));
		}
		default:
			REGEN_WARN("Unknown data type " << input->baseType());
			break;
	}
	return 0.0f;
}

VectorWidget::VectorWidget(const RegenWidgetData &data, QWidget *parent)
		: RegenWidget(data, parent) {
	GLuint count = input_->valsPerElement();
	GLuint i;
	if (count > 4) {
		throw std::runtime_error("More than 4 components unsupported.");
	}
	auto mapped = input_->mapClientDataRaw(BUFFER_GPU_READ);
	const byte *value = mapped.r;
	ignoreValueChanges_ = true;

	ui_.setupUi(this);
	QLabel *labelWidgets[4] =
			{ui_.xLabel, ui_.yLabel, ui_.zLabel, ui_.wLabel};
	QSlider *valueWidgets[4] =
			{ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
	QLineEdit *valueTexts[4] =
			{ui_.xValueText, ui_.yValueText, ui_.zValueText, ui_.wValueText};
	QWidget *valueContainers[4] =
			{ui_.xContainer, ui_.yContainer, ui_.zContainer, ui_.wContainer};

	// hide component widgets
	for (i = 0; i < 4; ++i) {
		labelWidgets[i]->hide();
		valueWidgets[i]->hide();
		valueTexts[i]->hide();
		valueContainers[i]->hide();
	}

	for (i = 0; i < count; ++i) {
		auto limits = input_->schema().limits(i);
		if (limits.has_value()) {
			valueWidgets[i]->setMinimum(static_cast<int>(limits->first * 1000.0f));
			valueWidgets[i]->setMaximum(static_cast<int>(limits->second * 1000.0f));
		} else {
			GLfloat x = readInputValue(input_, value, i);
			valueWidgets[i]->setMinimum(0);
			valueWidgets[i]->setMaximum(static_cast<int>(std::max(1.0, fabs(x) * 2.0) * 1000.0f));
		}
	}

	// show and set active components
	for (i = 0; i < count; ++i) {
		GLfloat x = readInputValue(input_, value, i);
		valueWidgets[i]->setValue(static_cast<int>(x * 1000.0f));
		valueTexts[i]->setText(QString::number(x));

		labelWidgets[i]->show();
		valueWidgets[i]->show();
		valueTexts[i]->show();
		valueContainers[i]->show();
	}
	ignoreValueChanges_ = false;
}

QSlider *VectorWidget::createSlider(int componentIndex, bool isExternal) {
	auto mapped = input_->mapClientDataRaw(BUFFER_GPU_READ);
	const byte *value = mapped.r;
	auto x = readInputValue(input_, value, componentIndex);
	// create a slider for component with given index, it will be synchronized with the other widgets
	auto *slider = new QSlider(Qt::Horizontal, this);
	slider->setSingleStep(1);
	slider->setPageStep(10);
	auto limits = input_->schema().limits(componentIndex);
	if (limits.has_value()) {
		slider->setMinimum(static_cast<int>(limits->first * 1000.0f));
		slider->setMaximum(static_cast<int>(limits->second * 1000.0f));
	} else {
		slider->setMinimum(0);
		slider->setMaximum(static_cast<int>(std::max(1.0, fabs(x) * 2.0) * 1000.0f));
	}
	// set the value of the slider
	slider->setValue(static_cast<int>(x * 1000.0f));
	connect(slider, &QSlider::valueChanged, this, [this, slider, componentIndex]() {
		// set the value of ui_.xValueEdit
		QSlider *valueWidgets[4] = {ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
		valueWidgets[componentIndex]->setValue(slider->value());
	});
	if (isExternal) externalSlider_[componentIndex].push_back(slider);
	return slider;
}

template<class T>
byte *createData(
		const ref_ptr<ShaderInput> &in,
		QSlider **valueWidgets,
		QLineEdit **valueTexts,
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

void VectorWidget::valueUpdated() {
	if (ignoreValueChanges_) return;

	QSlider *valueWidgets[4] =
			{ui_.xValueEdit, ui_.yValueEdit, ui_.zValueEdit, ui_.wValueEdit};
	QLineEdit *valueTexts[4] =
			{ui_.xValueText, ui_.yValueText, ui_.zValueText, ui_.wValueText};

	GLuint count = input_->valsPerElement();
	if (count > 4) {
		REGEN_WARN("More then 4 components unsupported.");
		return;
	}

	byte *changedData = nullptr;
	switch (input_->baseType()) {
		case GL_FLOAT:
			changedData = createData<GLfloat>(input_, valueWidgets, valueTexts, count);
			break;
		case GL_INT:
			changedData = createData<GLint>(input_, valueWidgets, valueTexts, count);
			break;
		case GL_UNSIGNED_INT:
			changedData = createData<GLuint>(input_, valueWidgets, valueTexts, count);
			break;
		default:
			REGEN_WARN("Unknown data type " << input_->baseType());
			break;
	}

	input_->writeVertex(0, changedData);
	delete[] changedData;

	// set value of external sliders
	for (GLuint i = 0; i < count; ++i) {
		for (auto *slider: externalSlider_[i]) {
			slider->setValue(valueWidgets[i]->value());
		}
	}
}
