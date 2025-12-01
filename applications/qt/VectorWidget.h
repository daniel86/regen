#ifndef REGEN_VECTOR_WIDGET_H
#define REGEN_VECTOR_WIDGET_H

#include "qt/ui_VectorWidget.h"
#include "qt-application.h"
#include "RegenWidget.h"

namespace regen {
	/**
	 * \brief Widget for editing vector values.
	 */
	class VectorWidget : public RegenWidget {
	Q_OBJECT

	public:
		explicit VectorWidget(const RegenWidgetData &data, QWidget *parent = nullptr);

		QSlider *createSlider(int vectorIndex, bool isExternal = true);

		void initializeValue();

	public slots:

		void valueUpdated();

	protected:
		Ui_vectorWidget ui_;
		std::array<std::vector<QSlider *>, 4> externalSlider_;
		bool ignoreValueChanges_ = false;
	};
} // regen

#endif //REGEN_VECTOR_WIDGET_H
