/*
 * generic-data-editor.h
 *
 *  Created on: 15.03.2013
 *      Author: daniel
 */

#ifndef GENERIC_DATA_EDITOR_H_
#define GENERIC_DATA_EDITOR_H_

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFormLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QSlider>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_genericDataEditor
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_2;
    QVBoxLayout *mainLayout;
    QSplitter *splitter;
    QTreeWidget *treeWidget;
    QWidget *formLayoutWidget;
    QFormLayout *valueLayout;
    QLabel *nameLabel;
    QLabel *nameValue;
    QLabel *typeLabel;
    QLabel *typeValue;
    QLabel *xLabel;
    QSlider *xValue;
    QLabel *yLabel;
    QSlider *yValue;
    QLabel *zLabel;
    QSlider *zValue;
    QLabel *wLabel;
    QSlider *wValue;
    QLabel *descriptionLabel;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *genericDataEditor)
    {
        if (genericDataEditor->objectName().isEmpty())
            genericDataEditor->setObjectName(QString::fromUtf8("genericDataEditor"));
        genericDataEditor->resize(546, 400);
        centralwidget = new QWidget(genericDataEditor);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout_2 = new QGridLayout(centralwidget);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        mainLayout = new QVBoxLayout();
        mainLayout->setObjectName(QString::fromUtf8("mainLayout"));
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy);
        splitter->setOrientation(Qt::Horizontal);
        treeWidget = new QTreeWidget(splitter);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        splitter->addWidget(treeWidget);
        treeWidget->header()->setVisible(false);
        formLayoutWidget = new QWidget(splitter);
        formLayoutWidget->setObjectName(QString::fromUtf8("formLayoutWidget"));
        valueLayout = new QFormLayout(formLayoutWidget);
        valueLayout->setObjectName(QString::fromUtf8("valueLayout"));
        valueLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        valueLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
        valueLayout->setLabelAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        valueLayout->setFormAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        valueLayout->setHorizontalSpacing(14);
        valueLayout->setContentsMargins(4, 2, 4, 2);
        nameLabel = new QLabel(formLayoutWidget);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));
        nameLabel->setMaximumSize(QSize(80, 16777215));
        QFont font;
        font.setBold(true);
        font.setUnderline(false);
        font.setWeight(75);
        nameLabel->setFont(font);

        valueLayout->setWidget(0, QFormLayout::LabelRole, nameLabel);

        nameValue = new QLabel(formLayoutWidget);
        nameValue->setObjectName(QString::fromUtf8("nameValue"));
        nameValue->setMaximumSize(QSize(150, 16777215));

        valueLayout->setWidget(0, QFormLayout::FieldRole, nameValue);

        typeLabel = new QLabel(formLayoutWidget);
        typeLabel->setObjectName(QString::fromUtf8("typeLabel"));
        typeLabel->setMaximumSize(QSize(80, 16777215));
        QFont font1;
        font1.setBold(true);
        font1.setWeight(75);
        typeLabel->setFont(font1);

        valueLayout->setWidget(1, QFormLayout::LabelRole, typeLabel);

        typeValue = new QLabel(formLayoutWidget);
        typeValue->setObjectName(QString::fromUtf8("typeValue"));
        typeValue->setMaximumSize(QSize(150, 16777215));

        valueLayout->setWidget(1, QFormLayout::FieldRole, typeValue);

        xLabel = new QLabel(formLayoutWidget);
        xLabel->setObjectName(QString::fromUtf8("xLabel"));
        xLabel->setMaximumSize(QSize(80, 16777215));
        xLabel->setFont(font1);

        valueLayout->setWidget(2, QFormLayout::LabelRole, xLabel);

        xValue = new QSlider(formLayoutWidget);
        xValue->setObjectName(QString::fromUtf8("xValue"));
        xValue->setMaximumSize(QSize(150, 16777215));
        xValue->setOrientation(Qt::Horizontal);

        valueLayout->setWidget(2, QFormLayout::FieldRole, xValue);

        yLabel = new QLabel(formLayoutWidget);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));
        yLabel->setMaximumSize(QSize(80, 16777215));
        yLabel->setFont(font1);

        valueLayout->setWidget(3, QFormLayout::LabelRole, yLabel);

        yValue = new QSlider(formLayoutWidget);
        yValue->setObjectName(QString::fromUtf8("yValue"));
        yValue->setMaximumSize(QSize(150, 16777215));
        yValue->setOrientation(Qt::Horizontal);

        valueLayout->setWidget(3, QFormLayout::FieldRole, yValue);

        zLabel = new QLabel(formLayoutWidget);
        zLabel->setObjectName(QString::fromUtf8("zLabel"));
        zLabel->setMaximumSize(QSize(80, 16777215));
        zLabel->setFont(font1);

        valueLayout->setWidget(4, QFormLayout::LabelRole, zLabel);

        zValue = new QSlider(formLayoutWidget);
        zValue->setObjectName(QString::fromUtf8("zValue"));
        zValue->setMaximumSize(QSize(150, 16777215));
        zValue->setOrientation(Qt::Horizontal);

        valueLayout->setWidget(4, QFormLayout::FieldRole, zValue);

        wLabel = new QLabel(formLayoutWidget);
        wLabel->setObjectName(QString::fromUtf8("wLabel"));
        wLabel->setMaximumSize(QSize(80, 16777215));
        wLabel->setFont(font1);

        valueLayout->setWidget(5, QFormLayout::LabelRole, wLabel);

        wValue = new QSlider(formLayoutWidget);
        wValue->setObjectName(QString::fromUtf8("wValue"));
        wValue->setMaximumSize(QSize(150, 16777215));
        wValue->setOrientation(Qt::Horizontal);

        valueLayout->setWidget(5, QFormLayout::FieldRole, wValue);

        descriptionLabel = new QLabel(formLayoutWidget);
        descriptionLabel->setObjectName(QString::fromUtf8("label"));
        descriptionLabel->setWordWrap(false);

        valueLayout->setWidget(6, QFormLayout::SpanningRole, descriptionLabel);

        splitter->addWidget(formLayoutWidget);

        mainLayout->addWidget(splitter);


        gridLayout_2->addLayout(mainLayout, 0, 0, 1, 1);

        genericDataEditor->setCentralWidget(centralwidget);
        statusBar = new QStatusBar(genericDataEditor);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        genericDataEditor->setStatusBar(statusBar);

        retranslateUi(genericDataEditor);
        QObject::connect(xValue, SIGNAL(valueChanged(int)), genericDataEditor, SLOT(setXValue(int)));
        QObject::connect(yValue, SIGNAL(valueChanged(int)), genericDataEditor, SLOT(setYValue(int)));
        QObject::connect(zValue, SIGNAL(valueChanged(int)), genericDataEditor, SLOT(setZValue(int)));
        QObject::connect(wValue, SIGNAL(valueChanged(int)), genericDataEditor, SLOT(setWValue(int)));
        QObject::connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), genericDataEditor, SLOT(activateValue(QTreeWidgetItem*,QTreeWidgetItem*)));

        QMetaObject::connectSlotsByName(genericDataEditor);
    } // setupUi

    void retranslateUi(QMainWindow *genericDataEditor)
    {
        genericDataEditor->setWindowTitle(QApplication::translate("genericDataEditor", "MainWindow", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("genericDataEditor", "column", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("genericDataEditor", "name", 0, QApplication::UnicodeUTF8));
        nameValue->setText(QApplication::translate("genericDataEditor", "...", 0, QApplication::UnicodeUTF8));
        typeLabel->setText(QApplication::translate("genericDataEditor", "type", 0, QApplication::UnicodeUTF8));
        typeValue->setText(QApplication::translate("genericDataEditor", "...", 0, QApplication::UnicodeUTF8));
        xLabel->setText(QApplication::translate("genericDataEditor", "x", 0, QApplication::UnicodeUTF8));
        yLabel->setText(QApplication::translate("genericDataEditor", "y", 0, QApplication::UnicodeUTF8));
        zLabel->setText(QApplication::translate("genericDataEditor", "z", 0, QApplication::UnicodeUTF8));
        wLabel->setText(QApplication::translate("genericDataEditor", "w", 0, QApplication::UnicodeUTF8));
        descriptionLabel->setText(QApplication::translate("genericDataEditor", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class genericDataEditor: public Ui_genericDataEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif /* GENERIC_DATA_EDITOR_H_ */
