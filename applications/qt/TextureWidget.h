#ifndef REGEN_TEXTURE_WIDGET_H
#define REGEN_TEXTURE_WIDGET_H

#include <qt/ui_ColorWidget.h>
#include <regen/textures/texture.h>
#include <QDateTime>
#include "qt-application.h"
#include "RegenWidget.h"

namespace regen {
	/**
	 * \brief Widget for displaying and editing textures.
	 */
	class TextureWidget : public RegenWidget {
	public:
		explicit TextureWidget(const RegenWidgetData &data, QWidget *parent = nullptr);

		~TextureWidget() override;

		void openFile();

		bool hasTextureFile() const;

	protected:
		ref_ptr<State> widgetState_;
		QWidget *textureWidget_;

		void hideEvent(QHideEvent *event) override;

		void showEvent(QShowEvent *event) override;
	};

} // knowrob

#endif //REGEN_TEXTURE_WIDGET_H
