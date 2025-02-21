
#ifndef SHADER_INPUT_WIDGET_H
#define SHADER_INPUT_WIDGET_H

#include <QtWidgets/QMainWindow>

#include <regen/gl-types/shader-input.h>
#include <regen/animations/animation.h>
#include <regen/states/state-node.h>
#include <QTreeWidgetItem>
#include "qt/ui_shader-input-editor.h"

namespace regen {
/**
 * \brief Allows editing ShaderInput values.
 */
	class ShaderInputWidget : public QWidget {
	Q_OBJECT

	public:
		explicit ShaderInputWidget(QWidget *parent = nullptr);

		~ShaderInputWidget() override;

		void setNode(const ref_ptr<StateNode> &node);

	public slots:

		void resetValue();

		void valueUpdated();

		void maxUpdated();

		void activateValue(QTreeWidgetItem *, QTreeWidgetItem *);

	protected:
		Ui_shaderInputEditor ui_;
		QTreeWidgetItem *selectedItem_;
		ShaderInput *selectedInput_;
		GLboolean ignoreValueChanges_;

		std::map<ShaderInput *, byte *> initialValue_;
		std::map<ShaderInput *, GLuint> initialValueStamp_;
		std::map<ShaderInput *, GLuint> valueStamp_;

		std::map<QTreeWidgetItem *, ref_ptr<ShaderInput> > inputs_;

		bool handleState(
				const ref_ptr<State> &state,
				QTreeWidgetItem *parent);

		bool handleNode(
				const StateNode *node,
				QTreeWidgetItem *parent);

		bool handleInput(
				const NamedShaderInput &input,
				QTreeWidgetItem *parent);

		void updateInitialValue(ShaderInput *x);
	};
}

#endif /* SHADER_INPUT_WIDGET_H */
