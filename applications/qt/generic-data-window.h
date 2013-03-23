/*
 * generic-data-window.h
 *
 *  Created on: 16.03.2013
 *      Author: daniel
 */

#ifndef GENERIC_DATA_WINDOW_H_
#define GENERIC_DATA_WINDOW_H_

#include <QtGui/QMainWindow>

#include <ogle/gl-types/shader-input.h>
#include <applications/qt/generic-data-editor.h>

namespace ogle {
/**
 * \brief Allows editing ShaderInput values.
 */
class GenericDataWindow : public QMainWindow
{
Q_OBJECT

public:
  GenericDataWindow(QWidget *parent = 0);

  /**
   * Add generic data to editor, allowing the user to manipulate the data.
   * @param treePath path in tree widget.
   * @param in the data
   * @param minBound per component minimum
   * @param maxBound per component maximum
   * @param precision per component precision
   * @param description brief description
   */
  void addGenericData(
      const string &treePath,
      const ref_ptr<ShaderInput> &in,
      const Vec4f &minBound,
      const Vec4f &maxBound,
      const Vec4i &precision,
      const string &description);

signals:
  void windowClosed();

public slots:
  void setXValue(double);
  void setYValue(double);
  void setZValue(double);
  void setWValue(double);
  void activateValue(QTreeWidgetItem*,QTreeWidgetItem*);

protected:
  Ui_genericDataEditor ui_;
  QTreeWidgetItem *selectedItem_;
  ShaderInput *selectedInput_;

  map<ShaderInput*,Vec4f> maxBounds_;
  map<ShaderInput*,Vec4f> minBounds_;
  map<ShaderInput*,Vec4i> precisions_;
  map<ShaderInput*,string> description_;
  map<QTreeWidgetItem*, ref_ptr<ShaderInput> > inputs_;

  void setValue(GLdouble sliderValue, GLint index);
  virtual void closeEvent(QCloseEvent*);
};
}

#endif /* GENERIC_DATA_WINDOW_H_ */
