#ifndef REGEN_COLOR_WIDGET_H
#define REGEN_COLOR_WIDGET_H

#include <qt/ui_ColorWidget.h>
#include "regen/glsl/shader-input.h"
#include "qt-application.h"
#include "RegenWidget.h"

namespace regen {
	/**
	 * \brief Color widget for editing color values.
	 *
	 * This widget allows the user to edit color values in different formats
	 * (HTML, RGB, and alpha) and provides a color picker dialog.
	 */
	class ColorWidget : public RegenWidget {
	Q_OBJECT

	public:
		explicit ColorWidget(const RegenWidgetData &data, QWidget *parent = nullptr);

		QColor initializeColor();

	signals:

		void colorChanged(const QColor &color);

	public slots:

		void pickColor();

		void alphaChanged();

		void htmlChanged();

		void rgbChanged();

		void scaleChanged();

	protected:
		Ui_colorWidget ui_;

		void updateColor(const QColor &color);
	};

} // regen

#endif //REGEN_COLOR_WIDGET_H
