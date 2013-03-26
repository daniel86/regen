/*
 * generic-data-window.h
 *
 *  Created on: 16.03.2013
 *      Author: daniel
 */

#ifndef GENERIC_DATA_WINDOW_H_
#define GENERIC_DATA_WINDOW_H_

#include <QtGui/QMainWindow>

#include <regen/gl-types/shader-input.h>
#include <applications/qt/shader-input-editor.h>

namespace regen {
/**
 * \brief Allows editing ShaderInput values.
 */
class ShaderInputWidget : public QWidget
{
Q_OBJECT

public:
  ShaderInputWidget(QWidget *parent = 0);
  ~ShaderInputWidget();

  /**
   * Add generic data to editor, allowing the user to manipulate the data.
   * @param treePath path in tree widget.
   * @param in the data
   * @param minBound per component minimum
   * @param maxBound per component maximum
   * @param precision per component precision
   * @param description brief description
   */
  void add(
      const string &treePath,
      const ref_ptr<ShaderInput> &in,
      const Vec4f &minBound,
      const Vec4f &maxBound,
      const Vec4i &precision,
      const string &description);

public slots:
  void setXValue(int);
  void setYValue(int);
  void setZValue(int);
  void setWValue(int);
  void resetValue();
  void activateValue(QTreeWidgetItem*,QTreeWidgetItem*);

protected:
  Ui_shaderInputEditor ui_;
  QTreeWidgetItem *selectedItem_;
  ShaderInput *selectedInput_;

  map<ShaderInput*,byte*> initialValue_;
  map<ShaderInput*,Vec4f> maxBounds_;
  map<ShaderInput*,Vec4f> minBounds_;
  map<ShaderInput*,Vec4i> precisions_;
  map<ShaderInput*,string> description_;
  map<QTreeWidgetItem*, ref_ptr<ShaderInput> > inputs_;

  void setValue(GLint sliderValue, GLint index);
};
}

#endif /* GENERIC_DATA_WINDOW_H_ */
