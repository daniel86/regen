#include "ColorWidget.h"
#include <QColorDialog>

using namespace regen;

ColorWidget::ColorWidget(const RegenWidgetData &data, QWidget *parent)
		: RegenWidget(data, parent) {
	ui_.setupUi(this);
	auto initialColor = initializeColor();
	updateColor(initialColor);

	// Show or hide the alpha label and slider
	bool hasAlpha = (input_->valsPerElement() == 4);
	ui_.alphaLabel->setVisible(hasAlpha);
	ui_.alphaValue->setVisible(hasAlpha);
	ui_.alphaContainer->setVisible(hasAlpha);
	ui_.alphaValueTxt->setVisible(hasAlpha);
	if (hasAlpha) {
		ui_.alphaValue->setValue(static_cast<int>(initialColor.alphaF() * 1000.0f));
	}
}

QColor ColorWidget::initializeColor() {
	auto mapped = input_->mapClientDataRaw(BUFFER_GPU_READ);
	const byte *value = mapped.r;
	uint32_t count = input_->valsPerElement();
	QColor color;
	if (input_->baseType() == GL_FLOAT) {
		float r = ((float *) value)[0];
		float g = ((float *) value)[1];
		float b = ((float *) value)[2];
		float a = (count == 4) ? ((float *) value)[3] : 1.0f;
		mapped.unmap();
		color.setRgbF(r, g, b, a);
	} else {
		REGEN_WARN("Unsupported data type for color input.");
		return color;
	}

	ui_.htmlValue->setText(color.name());
	QString rgbText = QString("%1, %2, %3")
			.arg(color.redF())
			.arg(color.greenF())
			.arg(color.blueF());
	ui_.rgbValue->setText(rgbText);
	if (count == 4) {
		ui_.alphaValue->setValue(static_cast<int>(color.alphaF() * 1000.0f));
	}

	emit colorChanged(color);
	return color;
}

void ColorWidget::updateColor(const QColor &color) {
	// Update the shader input
	uint32_t count = input_->valsPerElement();
	byte *changedData = new byte[input_->elementSize()];
	if (input_->baseType() == GL_FLOAT) {
		((float *) changedData)[0] = color.redF();
		((float *) changedData)[1] = color.greenF();
		((float *) changedData)[2] = color.blueF();
		if (count == 4) {
			((float *) changedData)[3] = color.alphaF();
		}
	}
	input_->writeVertex(0, changedData);
	delete[] changedData;

	emit colorChanged(color);
}

void ColorWidget::pickColor() {
	// Ensure the selected shader input is a color (3 or 4 components)
	uint32_t count = input_->valsPerElement();
	if (count < 3 || count > 4) {
		REGEN_WARN("pickColor() called but selected input is not a color.");
		return;
	}

	// Get the current color from the shader input
	auto mapped = input_->mapClientDataRaw(BUFFER_GPU_READ);
	const byte *value = mapped.r;
	QColor initialColor;
	if (input_->baseType() == GL_FLOAT) {
		float r = ((float *) value)[0];
		float g = ((float *) value)[1];
		float b = ((float *) value)[2];
		float a = (count == 4) ? ((float *) value)[3] : 1.0f;
		mapped.unmap();
		initialColor.setRgbF(r, g, b, a);
	} else {
		REGEN_WARN("Unsupported data type for color input.");
		return;
	}

	// Show the color dialog
	QColor color = QColorDialog::getColor(initialColor, this, "Select Color");
	if (!color.isValid()) {
		return; // User canceled the dialog
	}

	// Update the HTML and RGB text edits
	ui_.htmlValue->setText(color.name());
	QString rgbText = QString("%1, %2, %3")
			.arg(color.redF())
			.arg(color.greenF())
			.arg(color.blueF());
	ui_.rgbValue->setText(rgbText);
	if (count == 4) {
		ui_.alphaValue->setValue(static_cast<int>(color.alphaF() * 1000.0f));
	}
	updateColor(color);
}

void ColorWidget::alphaChanged() {
	// Ensure the selected shader input is a color (4 components)
	uint32_t count = input_->valsPerElement();
	if (count != 4) {
		REGEN_WARN("alphaChanged() called but selected input is not a color with alpha.");
		return;
	}
	auto sliderValue = ui_.alphaValue->value();

	// Update the alpha value in the shader input
	auto mapped = input_->mapClientDataRaw(BUFFER_GPU_READ);
	const byte *value = mapped.r;
	if (input_->baseType() == GL_FLOAT) {
		((float *) value)[3] = static_cast<float>(sliderValue) / 1000.0f;
	} else {
		REGEN_WARN("Unsupported data type for color input.");
		return;
	}
	mapped.unmap();
	input_->writeVertex(0, value);
}

void ColorWidget::htmlChanged() {
	// Parse the HTML color value
	QString htmlValue = ui_.htmlValue->text();
	QColor color(htmlValue);
	if (!color.isValid()) {
		REGEN_WARN("Invalid HTML color value.");
		return;
	}
	// Update the RGB text edit
	QString rgbText = QString("%1, %2, %3")
			.arg(color.redF())
			.arg(color.greenF())
			.arg(color.blueF());
	ui_.rgbValue->setText(rgbText);
	// Update the color button
	updateColor(color);
}

void ColorWidget::rgbChanged() {
	// Parse the RGB color value
	QString rgbText = ui_.rgbValue->text();
	QStringList rgbList = rgbText.split(", ");
	if (rgbList.size() != 3) {
		REGEN_WARN("Invalid RGB color value.");
		return;
	}
	QColor color;
	color.setRedF(rgbList[0].toFloat());
	color.setGreenF(rgbList[1].toFloat());
	color.setBlueF(rgbList[2].toFloat());
	// Update the HTML text edit
	ui_.htmlValue->setText(color.name());
	// Update the color button
	updateColor(color);
}

void ColorWidget::scaleChanged() {
	// Ensure the selected shader input is a color (3 or 4 components)
	//uint32_t count = input_->valsPerElement();
	//auto sliderValue = ui_.scaleValue->value();
}
