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
class ShaderInputWindow : public QMainWindow
{
Q_OBJECT

public:
  ShaderInputWindow(QWidget *parent = 0);

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

signals:
  void windowClosed();

public slots:
  void setXValue(int);
  void setYValue(int);
  void setZValue(int);
  void setWValue(int);
  void activateValue(QTreeWidgetItem*,QTreeWidgetItem*);

protected:
  Ui_shaderInputEditor ui_;
  QTreeWidgetItem *selectedItem_;
  ShaderInput *selectedInput_;

  map<ShaderInput*,Vec4f> maxBounds_;
  map<ShaderInput*,Vec4f> minBounds_;
  map<ShaderInput*,Vec4i> precisions_;
  map<ShaderInput*,string> description_;
  map<QTreeWidgetItem*, ref_ptr<ShaderInput> > inputs_;

  void setValue(GLint sliderValue, GLint index);
  virtual void closeEvent(QCloseEvent*);
};
}

#endif /* GENERIC_DATA_WINDOW_H_ */
