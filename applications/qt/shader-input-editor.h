/********************************************************************************
** Form generated from reading UI file 'shader-input-editorKB5044.ui'
**
** Created: Tue Mar 26 12:35:31 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SHADER_2D_INPUT_2D_EDITORKB5044_H
#define SHADER_2D_INPUT_2D_EDITORKB5044_H

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
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_shaderInputEditor
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
    QLabel *yLabel;
    QLabel *zLabel;
    QLabel *wLabel;
    QLabel *descriptionLabel;
    QVBoxLayout *xValueLayout;
    QLabel *xValueLabel;
    QSlider *xValue;
    QVBoxLayout *yValueLayout;
    QLabel *yValueLabel;
    QSlider *yValue;
    QVBoxLayout *zValueLayout;
    QLabel *zValueLabel;
    QSlider *zValue;
    QVBoxLayout *wValueLayout;
    QLabel *wValueLabel;
    QSlider *wValue;

    void setupUi(QMainWindow *shaderInputEditor)
    {
        if (shaderInputEditor->objectName().isEmpty())
            shaderInputEditor->setObjectName(QString::fromUtf8("shaderInputEditor"));
        shaderInputEditor->resize(503, 389);
        QFont font;
        font.setBold(false);
        font.setWeight(50);
        font.setKerning(true);
        shaderInputEditor->setFont(font);
        centralwidget = new QWidget(shaderInputEditor);
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
        treeWidget->setMaximumSize(QSize(16777215, 16777215));
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
        QFont font1;
        font1.setBold(true);
        font1.setUnderline(false);
        font1.setWeight(75);
        nameLabel->setFont(font1);

        valueLayout->setWidget(0, QFormLayout::LabelRole, nameLabel);

        nameValue = new QLabel(formLayoutWidget);
        nameValue->setObjectName(QString::fromUtf8("nameValue"));
        nameValue->setMaximumSize(QSize(150, 16777215));

        valueLayout->setWidget(0, QFormLayout::FieldRole, nameValue);

        typeLabel = new QLabel(formLayoutWidget);
        typeLabel->setObjectName(QString::fromUtf8("typeLabel"));
        typeLabel->setMaximumSize(QSize(80, 16777215));
        QFont font2;
        font2.setBold(true);
        font2.setWeight(75);
        typeLabel->setFont(font2);

        valueLayout->setWidget(1, QFormLayout::LabelRole, typeLabel);

        typeValue = new QLabel(formLayoutWidget);
        typeValue->setObjectName(QString::fromUtf8("typeValue"));
        typeValue->setMaximumSize(QSize(150, 16777215));

        valueLayout->setWidget(1, QFormLayout::FieldRole, typeValue);

        xLabel = new QLabel(formLayoutWidget);
        xLabel->setObjectName(QString::fromUtf8("xLabel"));
        xLabel->setMaximumSize(QSize(80, 16777215));
        xLabel->setFont(font2);

        valueLayout->setWidget(2, QFormLayout::LabelRole, xLabel);

        yLabel = new QLabel(formLayoutWidget);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));
        yLabel->setMaximumSize(QSize(80, 16777215));
        yLabel->setFont(font2);

        valueLayout->setWidget(3, QFormLayout::LabelRole, yLabel);

        zLabel = new QLabel(formLayoutWidget);
        zLabel->setObjectName(QString::fromUtf8("zLabel"));
        zLabel->setMaximumSize(QSize(80, 16777215));
        zLabel->setFont(font2);

        valueLayout->setWidget(4, QFormLayout::LabelRole, zLabel);

        wLabel = new QLabel(formLayoutWidget);
        wLabel->setObjectName(QString::fromUtf8("wLabel"));
        wLabel->setMaximumSize(QSize(80, 16777215));
        wLabel->setFont(font2);

        valueLayout->setWidget(5, QFormLayout::LabelRole, wLabel);

        descriptionLabel = new QLabel(formLayoutWidget);
        descriptionLabel->setObjectName(QString::fromUtf8("descriptionLabel"));
        sizePolicy.setHeightForWidth(descriptionLabel->sizePolicy().hasHeightForWidth());
        descriptionLabel->setSizePolicy(sizePolicy);
        descriptionLabel->setMaximumSize(QSize(250, 16777215));
        QFont font3;
        font3.setItalic(true);
        descriptionLabel->setFont(font3);
        descriptionLabel->setWordWrap(true);

        valueLayout->setWidget(6, QFormLayout::SpanningRole, descriptionLabel);

        xValueLayout = new QVBoxLayout();
        xValueLayout->setSpacing(0);
        xValueLayout->setObjectName(QString::fromUtf8("xValueLayout"));
        xValueLabel = new QLabel(formLayoutWidget);
        xValueLabel->setObjectName(QString::fromUtf8("xValueLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(xValueLabel->sizePolicy().hasHeightForWidth());
        xValueLabel->setSizePolicy(sizePolicy1);
        QFont font4;
        font4.setPointSize(8);
        font4.setBold(false);
        font4.setWeight(50);
        xValueLabel->setFont(font4);
        xValueLabel->setFrameShape(QFrame::NoFrame);
        xValueLabel->setFrameShadow(QFrame::Plain);
        xValueLabel->setLineWidth(0);
        xValueLabel->setAlignment(Qt::AlignCenter);

        xValueLayout->addWidget(xValueLabel);

        xValue = new QSlider(formLayoutWidget);
        xValue->setObjectName(QString::fromUtf8("xValue"));
        xValue->setMaximum(99999);
        xValue->setOrientation(Qt::Horizontal);

        xValueLayout->addWidget(xValue);


        valueLayout->setLayout(2, QFormLayout::FieldRole, xValueLayout);

        yValueLayout = new QVBoxLayout();
        yValueLayout->setSpacing(0);
        yValueLayout->setObjectName(QString::fromUtf8("yValueLayout"));
        yValueLabel = new QLabel(formLayoutWidget);
        yValueLabel->setObjectName(QString::fromUtf8("yValueLabel"));
        QFont font5;
        font5.setPointSize(8);
        yValueLabel->setFont(font5);
        yValueLabel->setLineWidth(0);
        yValueLabel->setAlignment(Qt::AlignCenter);

        yValueLayout->addWidget(yValueLabel);

        yValue = new QSlider(formLayoutWidget);
        yValue->setObjectName(QString::fromUtf8("yValue"));
        yValue->setMaximum(99999);
        yValue->setOrientation(Qt::Horizontal);

        yValueLayout->addWidget(yValue);


        valueLayout->setLayout(3, QFormLayout::FieldRole, yValueLayout);

        zValueLayout = new QVBoxLayout();
        zValueLayout->setSpacing(0);
        zValueLayout->setObjectName(QString::fromUtf8("zValueLayout"));
        zValueLabel = new QLabel(formLayoutWidget);
        zValueLabel->setObjectName(QString::fromUtf8("zValueLabel"));
        zValueLabel->setFont(font5);
        zValueLabel->setAlignment(Qt::AlignCenter);

        zValueLayout->addWidget(zValueLabel);

        zValue = new QSlider(formLayoutWidget);
        zValue->setObjectName(QString::fromUtf8("zValue"));
        zValue->setMaximum(99999);
        zValue->setOrientation(Qt::Horizontal);

        zValueLayout->addWidget(zValue);


        valueLayout->setLayout(4, QFormLayout::FieldRole, zValueLayout);

        wValueLayout = new QVBoxLayout();
        wValueLayout->setSpacing(0);
        wValueLayout->setObjectName(QString::fromUtf8("wValueLayout"));
        wValueLabel = new QLabel(formLayoutWidget);
        wValueLabel->setObjectName(QString::fromUtf8("wValueLabel"));
        wValueLabel->setFont(font5);
        wValueLabel->setAlignment(Qt::AlignCenter);

        wValueLayout->addWidget(wValueLabel);

        wValue = new QSlider(formLayoutWidget);
        wValue->setObjectName(QString::fromUtf8("wValue"));
        wValue->setMaximum(99999);
        wValue->setOrientation(Qt::Horizontal);

        wValueLayout->addWidget(wValue);


        valueLayout->setLayout(5, QFormLayout::FieldRole, wValueLayout);

        splitter->addWidget(formLayoutWidget);

        mainLayout->addWidget(splitter);


        gridLayout_2->addLayout(mainLayout, 0, 0, 1, 1);

        shaderInputEditor->setCentralWidget(centralwidget);

        retranslateUi(shaderInputEditor);
        QObject::connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), shaderInputEditor, SLOT(activateValue(QTreeWidgetItem*,QTreeWidgetItem*)));
        QObject::connect(xValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setXValue(int)));
        QObject::connect(yValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setYValue(int)));
        QObject::connect(zValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setZValue(int)));
        QObject::connect(wValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setWValue(int)));

        QMetaObject::connectSlotsByName(shaderInputEditor);
    } // setupUi

    void retranslateUi(QMainWindow *shaderInputEditor)
    {
        shaderInputEditor->setWindowTitle(QApplication::translate("shaderInputEditor", "Shader Input Editor", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("shaderInputEditor", "column", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("shaderInputEditor", "name", 0, QApplication::UnicodeUTF8));
        nameValue->setText(QApplication::translate("shaderInputEditor", "...", 0, QApplication::UnicodeUTF8));
        typeLabel->setText(QApplication::translate("shaderInputEditor", "type", 0, QApplication::UnicodeUTF8));
        typeValue->setText(QApplication::translate("shaderInputEditor", "...", 0, QApplication::UnicodeUTF8));
        xLabel->setText(QApplication::translate("shaderInputEditor", "x", 0, QApplication::UnicodeUTF8));
        yLabel->setText(QApplication::translate("shaderInputEditor", "y", 0, QApplication::UnicodeUTF8));
        zLabel->setText(QApplication::translate("shaderInputEditor", "z", 0, QApplication::UnicodeUTF8));
        wLabel->setText(QApplication::translate("shaderInputEditor", "w", 0, QApplication::UnicodeUTF8));
        descriptionLabel->setText(QApplication::translate("shaderInputEditor", "...", 0, QApplication::UnicodeUTF8));
        xValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        yValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        zValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        wValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class shaderInputEditor: public Ui_shaderInputEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SHADER_2D_INPUT_2D_EDITORKB5044_H
