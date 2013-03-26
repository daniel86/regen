/********************************************************************************
** Form generated from reading UI file 'shader-input-editorgO2688.ui'
**
** Created: Tue Mar 26 21:52:11 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SHADER_2D_INPUT_2D_EDITORGO2688_H
#define SHADER_2D_INPUT_2D_EDITORGO2688_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFormLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
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
    QWidget *xValueWidget;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *xValueLayout;
    QLabel *xValueLabel;
    QSlider *xValue;
    QWidget *yValueWidget;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *yValueLayout;
    QLabel *yValueLabel;
    QSlider *yValue;
    QWidget *zValueWidget;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *zValueLayout;
    QLabel *zValueLabel;
    QSlider *zValue;
    QWidget *wValueWidget;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *wValueLayout;
    QLabel *wValueLabel;
    QSlider *wValue;
    QHBoxLayout *resetLayout;
    QSpacerItem *resetSpacer;
    QPushButton *resetButton;
    QLabel *descriptionLabel;

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
        centralwidget->setStyleSheet(QString::fromUtf8("\n"
"/*******************************\n"
" ******** QLabel ***************\n"
" *******************************/\n"
"QLabel#descriptionLabel {\n"
"       color: #010203;\n"
"       background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n"
"                                       stop: 0 #d6d7da, stop: 1 #aaabae);\n"
"       border: 1px solid #8a8b8e;\n"
"       border-radius: 4px;\n"
"}\n"
"\n"
"/*******************************\n"
" ******** QSplitter *************\n"
" *******************************/\n"
"QSplitter::handle:horizontal { width: 6px; }\n"
"QSplitter::handle:vertical { height: 6px; }\n"
"\n"
"/*******************************\n"
" ******** QTreeView ***********\n"
" *******************************/\n"
"QTreeView {\n"
"}\n"
""));
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
        valueLayout->setVerticalSpacing(0);
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
        xLabel->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        valueLayout->setWidget(2, QFormLayout::LabelRole, xLabel);

        yLabel = new QLabel(formLayoutWidget);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));
        yLabel->setMaximumSize(QSize(80, 16777215));
        yLabel->setFont(font2);
        yLabel->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        valueLayout->setWidget(3, QFormLayout::LabelRole, yLabel);

        zLabel = new QLabel(formLayoutWidget);
        zLabel->setObjectName(QString::fromUtf8("zLabel"));
        zLabel->setMaximumSize(QSize(80, 16777215));
        zLabel->setFont(font2);
        zLabel->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        valueLayout->setWidget(4, QFormLayout::LabelRole, zLabel);

        wLabel = new QLabel(formLayoutWidget);
        wLabel->setObjectName(QString::fromUtf8("wLabel"));
        wLabel->setMaximumSize(QSize(80, 16777215));
        wLabel->setFont(font2);
        wLabel->setAlignment(Qt::AlignBottom|Qt::AlignLeading|Qt::AlignLeft);

        valueLayout->setWidget(5, QFormLayout::LabelRole, wLabel);

        xValueWidget = new QWidget(formLayoutWidget);
        xValueWidget->setObjectName(QString::fromUtf8("xValueWidget"));
        horizontalLayout = new QHBoxLayout(xValueWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 3, 0, 3);
        xValueLayout = new QVBoxLayout();
        xValueLayout->setSpacing(0);
        xValueLayout->setObjectName(QString::fromUtf8("xValueLayout"));
        xValueLabel = new QLabel(xValueWidget);
        xValueLabel->setObjectName(QString::fromUtf8("xValueLabel"));
        QFont font3;
        font3.setPointSize(8);
        xValueLabel->setFont(font3);
        xValueLabel->setAlignment(Qt::AlignCenter);

        xValueLayout->addWidget(xValueLabel);

        xValue = new QSlider(xValueWidget);
        xValue->setObjectName(QString::fromUtf8("xValue"));
        xValue->setOrientation(Qt::Horizontal);

        xValueLayout->addWidget(xValue);


        horizontalLayout->addLayout(xValueLayout);


        valueLayout->setWidget(2, QFormLayout::FieldRole, xValueWidget);

        yValueWidget = new QWidget(formLayoutWidget);
        yValueWidget->setObjectName(QString::fromUtf8("yValueWidget"));
        horizontalLayout_2 = new QHBoxLayout(yValueWidget);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 3, 0, 3);
        yValueLayout = new QVBoxLayout();
        yValueLayout->setSpacing(0);
        yValueLayout->setObjectName(QString::fromUtf8("yValueLayout"));
        yValueLabel = new QLabel(yValueWidget);
        yValueLabel->setObjectName(QString::fromUtf8("yValueLabel"));
        yValueLabel->setFont(font3);
        yValueLabel->setAlignment(Qt::AlignCenter);

        yValueLayout->addWidget(yValueLabel);

        yValue = new QSlider(yValueWidget);
        yValue->setObjectName(QString::fromUtf8("yValue"));
        yValue->setOrientation(Qt::Horizontal);

        yValueLayout->addWidget(yValue);


        horizontalLayout_2->addLayout(yValueLayout);


        valueLayout->setWidget(3, QFormLayout::FieldRole, yValueWidget);

        zValueWidget = new QWidget(formLayoutWidget);
        zValueWidget->setObjectName(QString::fromUtf8("zValueWidget"));
        horizontalLayout_3 = new QHBoxLayout(zValueWidget);
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 3, 0, 3);
        zValueLayout = new QVBoxLayout();
        zValueLayout->setSpacing(0);
        zValueLayout->setObjectName(QString::fromUtf8("zValueLayout"));
        zValueLabel = new QLabel(zValueWidget);
        zValueLabel->setObjectName(QString::fromUtf8("zValueLabel"));
        zValueLabel->setFont(font3);
        zValueLabel->setAlignment(Qt::AlignCenter);

        zValueLayout->addWidget(zValueLabel);

        zValue = new QSlider(zValueWidget);
        zValue->setObjectName(QString::fromUtf8("zValue"));
        zValue->setOrientation(Qt::Horizontal);

        zValueLayout->addWidget(zValue);


        horizontalLayout_3->addLayout(zValueLayout);


        valueLayout->setWidget(4, QFormLayout::FieldRole, zValueWidget);

        wValueWidget = new QWidget(formLayoutWidget);
        wValueWidget->setObjectName(QString::fromUtf8("wValueWidget"));
        horizontalLayout_4 = new QHBoxLayout(wValueWidget);
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 3, 0, 3);
        wValueLayout = new QVBoxLayout();
        wValueLayout->setSpacing(0);
        wValueLayout->setObjectName(QString::fromUtf8("wValueLayout"));
        wValueLabel = new QLabel(wValueWidget);
        wValueLabel->setObjectName(QString::fromUtf8("wValueLabel"));
        wValueLabel->setFont(font3);
        wValueLabel->setAlignment(Qt::AlignCenter);

        wValueLayout->addWidget(wValueLabel);

        wValue = new QSlider(wValueWidget);
        wValue->setObjectName(QString::fromUtf8("wValue"));
        wValue->setOrientation(Qt::Horizontal);

        wValueLayout->addWidget(wValue);


        horizontalLayout_4->addLayout(wValueLayout);


        valueLayout->setWidget(5, QFormLayout::FieldRole, wValueWidget);

        resetLayout = new QHBoxLayout();
        resetLayout->setObjectName(QString::fromUtf8("resetLayout"));
        resetLayout->setContentsMargins(-1, 3, -1, 6);
        resetSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        resetLayout->addItem(resetSpacer);

        resetButton = new QPushButton(formLayoutWidget);
        resetButton->setObjectName(QString::fromUtf8("resetButton"));

        resetLayout->addWidget(resetButton);


        valueLayout->setLayout(6, QFormLayout::FieldRole, resetLayout);

        descriptionLabel = new QLabel(formLayoutWidget);
        descriptionLabel->setObjectName(QString::fromUtf8("descriptionLabel"));
        sizePolicy.setHeightForWidth(descriptionLabel->sizePolicy().hasHeightForWidth());
        descriptionLabel->setSizePolicy(sizePolicy);
        descriptionLabel->setMaximumSize(QSize(250, 16777215));
        QFont font4;
        font4.setItalic(true);
        descriptionLabel->setFont(font4);
        descriptionLabel->setWordWrap(true);

        valueLayout->setWidget(7, QFormLayout::SpanningRole, descriptionLabel);

        splitter->addWidget(formLayoutWidget);

        mainLayout->addWidget(splitter);


        gridLayout_2->addLayout(mainLayout, 0, 0, 1, 1);

        shaderInputEditor->setCentralWidget(centralwidget);

        retranslateUi(shaderInputEditor);
        QObject::connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), shaderInputEditor, SLOT(activateValue(QTreeWidgetItem*,QTreeWidgetItem*)));
        QObject::connect(resetButton, SIGNAL(clicked()), shaderInputEditor, SLOT(resetValue()));
        QObject::connect(wValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setWValue(int)));
        QObject::connect(zValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setZValue(int)));
        QObject::connect(yValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setYValue(int)));
        QObject::connect(xValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setXValue(int)));

        QMetaObject::connectSlotsByName(shaderInputEditor);
    } // setupUi

    void retranslateUi(QMainWindow *shaderInputEditor)
    {
        shaderInputEditor->setWindowTitle(QApplication::translate("shaderInputEditor", "Shader Input Editor", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("shaderInputEditor", "name", 0, QApplication::UnicodeUTF8));
        nameValue->setText(QApplication::translate("shaderInputEditor", "uniform name", 0, QApplication::UnicodeUTF8));
        typeLabel->setText(QApplication::translate("shaderInputEditor", "type", 0, QApplication::UnicodeUTF8));
        typeValue->setText(QApplication::translate("shaderInputEditor", "float", 0, QApplication::UnicodeUTF8));
        xLabel->setText(QApplication::translate("shaderInputEditor", "x", 0, QApplication::UnicodeUTF8));
        yLabel->setText(QApplication::translate("shaderInputEditor", "y", 0, QApplication::UnicodeUTF8));
        zLabel->setText(QApplication::translate("shaderInputEditor", "z", 0, QApplication::UnicodeUTF8));
        wLabel->setText(QApplication::translate("shaderInputEditor", "w", 0, QApplication::UnicodeUTF8));
        xValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        yValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        zValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        wValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        resetButton->setText(QApplication::translate("shaderInputEditor", "Reset", 0, QApplication::UnicodeUTF8));
        descriptionLabel->setText(QApplication::translate("shaderInputEditor", "brief description of the uniform.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class shaderInputEditor: public Ui_shaderInputEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SHADER_2D_INPUT_2D_EDITORGO2688_H
