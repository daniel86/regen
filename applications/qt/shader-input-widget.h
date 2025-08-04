
#ifndef SHADER_INPUT_WIDGET_H
#define SHADER_INPUT_WIDGET_H

#include <QtWidgets/QMainWindow>

#include <regen/regen.h>
#include "regen/glsl/shader-input.h"
#include <regen/animations/animation.h>
#include <regen/states/state-node.h>
#include <QTreeWidgetItem>
#include "qt-application.h"
#include "qt/ui_shader-input-editor.h"

namespace regen {
	/**
	 * \brief Allows editing ShaderInput values.
	 */
	class ShaderInputWidget : public QWidget {
	Q_OBJECT

	public:
		explicit ShaderInputWidget(QtApplication *app, QWidget *parent = nullptr);

		~ShaderInputWidget() override;

		void setNode(const ref_ptr<StateNode> &node);

	public slots:

		void activateValue(QTreeWidgetItem *, QTreeWidgetItem *);

	protected:
		QtApplication *app_;
		Ui_shaderInputEditor ui_;
		QTreeWidgetItem *selectedItem_;
		GLboolean ignoreValueChanges_;

		std::map<ShaderInput *, byte *> initialValue_;
		std::map<ShaderInput *, GLuint> initialValueStamp_;
		std::map<ShaderInput *, GLuint> valueStamp_;

		ref_ptr<StateNode> rootNode_;
		std::map<QTreeWidgetItem *, ref_ptr<StateNode>> nodes_;
		std::map<QTreeWidgetItem *, const Animation*> animations_;

		void updateInitialValue(ShaderInput *x);

		bool handleState(
				const ref_ptr<StateNode> &node,
				const ref_ptr<State> &state,
				QTreeWidgetItem *parent);

		bool handleNode(
				const ref_ptr<StateNode> &node,
				QTreeWidgetItem *parent);

		bool addParameter(
				const ref_ptr<StateNode> &node,
				const NamedShaderInput &input,
				QTreeWidgetItem *parent);

		void addParameterWidget_(QWidget *parameterWidget, const NamedShaderInput &namedInput, const QString &icon);

		void handleTextureParameter_(QWidget *parameterWidget, const NamedShaderInput &namedInput, QHBoxLayout *titleBar);

		void resetParameter(ShaderInput *);

		bool isValidNode(const StateNode *node);

		bool isValidState(const State *rootState);

		bool isValidParameter(const ShaderInput *input);
	};
}

#endif /* SHADER_INPUT_WIDGET_H */
