/********************************************************************************
** Form generated from reading UI file 'shader-input-editoruP2688.ui'
**
** Created: Wed Mar 27 00:41:40 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SHADER_2D_INPUT_2D_EDITORUP2688_H
#define SHADER_2D_INPUT_2D_EDITORUP2688_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_shaderInputEditor
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *titleLabel;
    QTreeWidget *treeWidget;
    QFrame *valueFrame;
    QGridLayout *gridLayout;
    QLabel *xLabel;
    QLabel *nameLabel;
    QLabel *typeValue;
    QWidget *xValueWidget;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *xValueLayout;
    QLabel *xValueLabel;
    QSlider *xValue;
    QLabel *nameValue;
    QWidget *zValueWidget;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *zValueLayout;
    QLabel *zValueLabel;
    QSlider *zValue;
    QLabel *zLabel;
    QLabel *wLabel;
    QLabel *yLabel;
    QWidget *yValueWidget;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *yValueLayout;
    QLabel *yValueLabel;
    QSlider *yValue;
    QHBoxLayout *resetLayout;
    QSpacerItem *resetSpacer;
    QPushButton *resetButton;
    QWidget *wValueWidget;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *wValueLayout;
    QLabel *wValueLabel;
    QSlider *wValue;
    QLabel *typeLabel;
    QLabel *descriptionLabel;

    void setupUi(QWidget *shaderInputEditor)
    {
        if (shaderInputEditor->objectName().isEmpty())
            shaderInputEditor->setObjectName(QString::fromUtf8("shaderInputEditor"));
        shaderInputEditor->resize(312, 419);
        shaderInputEditor->setStyleSheet(QString::fromUtf8("\n"
"QWidget#shaderInputEditor {\n"
"       background-color: #edeceb;\n"
"}\n"
"\n"
"QFrame#titleLabel,#treeWidget,#valueFrame {\n"
"       background-color: #edeceb;\n"
"       border: 2px solid #627282;\n"
"}\n"
"QFrame#titleLabel {\n"
"       border-top-color: transparent;\n"
"       border-top-left-radius: 0px;\n"
"       border-top-right-radius: 0px;\n"
"}\n"
"QFrame#treeWidget {\n"
"       border-top-color: transparent;\n"
"       border-bottom-color: transparent;\n"
"}\n"
"QFrame#valueFrame {\n"
"       border-bottom-left-radius: 0px;\n"
"       border-bottom-right-radius: 0px;\n"
"       border-bottom-color: transparent;\n"
"}\n"
"\n"
"/*******************************\n"
" ******** QLabel ***************\n"
" *******************************/\n"
"\n"
"QLabel#nameLabel,#typeLabel {\n"
"       background-color: #edeceb;\n"
"       border-radius: 4px;\n"
"       border: 1px solid #8a8b8e;\n"
"}\n"
"QLabel#nameValue,#typeValue {\n"
"       background-color: #fdfcfb;\n"
"       border-radius: 4px;\n"
"       border: 1px solid #8a8b8e;\n"
"}\n"
"QLabel#descriptionLabel {\n"
"       color: #010203;\n"
"       background-"
                        "color: #fafbfe;\n"
"       border: 1px solid #8a8b8e;\n"
"       border-radius: 0px;\n"
"}\n"
"\n"
"/*******************************\n"
" ******** QSplitter *************\n"
" *******************************/\n"
"QSplitter::handle:horizontal { width: 6px; }\n"
"QSplitter::handle:vertical { height: 6px; }\n"
"\n"
""));
        verticalLayout = new QVBoxLayout(shaderInputEditor);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        titleLabel = new QLabel(shaderInputEditor);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setMinimumSize(QSize(0, 22));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setUnderline(false);
        font.setWeight(75);
        titleLabel->setFont(font);
        titleLabel->setFrameShape(QFrame::Box);
        titleLabel->setFrameShadow(QFrame::Sunken);
        titleLabel->setLineWidth(1);

        verticalLayout->addWidget(titleLabel);

        treeWidget = new QTreeWidget(shaderInputEditor);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        treeWidget->setHeaderItem(__qtreewidgetitem);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setMaximumSize(QSize(16777215, 16777215));
        treeWidget->header()->setVisible(false);

        verticalLayout->addWidget(treeWidget);

        valueFrame = new QFrame(shaderInputEditor);
        valueFrame->setObjectName(QString::fromUtf8("valueFrame"));
        valueFrame->setAutoFillBackground(false);
        valueFrame->setFrameShape(QFrame::Box);
        valueFrame->setFrameShadow(QFrame::Raised);
        gridLayout = new QGridLayout(valueFrame);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        xLabel = new QLabel(valueFrame);
        xLabel->setObjectName(QString::fromUtf8("xLabel"));
        xLabel->setMinimumSize(QSize(50, 24));
        xLabel->setMaximumSize(QSize(80, 16777215));
        QFont font1;
        font1.setBold(true);
        font1.setWeight(75);
        xLabel->setFont(font1);
        xLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(xLabel, 3, 0, 1, 1);

        nameLabel = new QLabel(valueFrame);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));
        nameLabel->setMinimumSize(QSize(50, 24));
        nameLabel->setMaximumSize(QSize(80, 16777215));
        QFont font2;
        font2.setBold(true);
        font2.setUnderline(false);
        font2.setWeight(75);
        nameLabel->setFont(font2);
        nameLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(nameLabel, 1, 0, 1, 1);

        typeValue = new QLabel(valueFrame);
        typeValue->setObjectName(QString::fromUtf8("typeValue"));
        typeValue->setMaximumSize(QSize(150, 16777215));

        gridLayout->addWidget(typeValue, 2, 1, 1, 1);

        xValueWidget = new QWidget(valueFrame);
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


        gridLayout->addWidget(xValueWidget, 3, 1, 1, 1);

        nameValue = new QLabel(valueFrame);
        nameValue->setObjectName(QString::fromUtf8("nameValue"));
        nameValue->setMaximumSize(QSize(150, 16777215));

        gridLayout->addWidget(nameValue, 1, 1, 1, 1);

        zValueWidget = new QWidget(valueFrame);
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


        gridLayout->addWidget(zValueWidget, 5, 1, 1, 1);

        zLabel = new QLabel(valueFrame);
        zLabel->setObjectName(QString::fromUtf8("zLabel"));
        zLabel->setMinimumSize(QSize(50, 24));
        zLabel->setMaximumSize(QSize(80, 16777215));
        zLabel->setFont(font1);
        zLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(zLabel, 5, 0, 1, 1);

        wLabel = new QLabel(valueFrame);
        wLabel->setObjectName(QString::fromUtf8("wLabel"));
        wLabel->setMinimumSize(QSize(50, 24));
        wLabel->setMaximumSize(QSize(80, 16777215));
        wLabel->setFont(font1);
        wLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(wLabel, 6, 0, 1, 1);

        yLabel = new QLabel(valueFrame);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));
        yLabel->setMinimumSize(QSize(50, 24));
        yLabel->setMaximumSize(QSize(80, 16777215));
        yLabel->setFont(font1);
        yLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(yLabel, 4, 0, 1, 1);

        yValueWidget = new QWidget(valueFrame);
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


        gridLayout->addWidget(yValueWidget, 4, 1, 1, 1);

        resetLayout = new QHBoxLayout();
        resetLayout->setObjectName(QString::fromUtf8("resetLayout"));
        resetLayout->setContentsMargins(-1, 3, -1, 6);
        resetSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        resetLayout->addItem(resetSpacer);

        resetButton = new QPushButton(valueFrame);
        resetButton->setObjectName(QString::fromUtf8("resetButton"));

        resetLayout->addWidget(resetButton);


        gridLayout->addLayout(resetLayout, 7, 1, 1, 1);

        wValueWidget = new QWidget(valueFrame);
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


        gridLayout->addWidget(wValueWidget, 6, 1, 1, 1);

        typeLabel = new QLabel(valueFrame);
        typeLabel->setObjectName(QString::fromUtf8("typeLabel"));
        typeLabel->setMinimumSize(QSize(50, 24));
        typeLabel->setMaximumSize(QSize(80, 16777215));
        typeLabel->setFont(font1);
        typeLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(typeLabel, 2, 0, 1, 1);

        descriptionLabel = new QLabel(valueFrame);
        descriptionLabel->setObjectName(QString::fromUtf8("descriptionLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(descriptionLabel->sizePolicy().hasHeightForWidth());
        descriptionLabel->setSizePolicy(sizePolicy);
        descriptionLabel->setMaximumSize(QSize(250, 16777215));
        QFont font4;
        font4.setItalic(true);
        descriptionLabel->setFont(font4);
        descriptionLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        descriptionLabel->setWordWrap(true);

        gridLayout->addWidget(descriptionLabel, 8, 0, 1, 2);


        verticalLayout->addWidget(valueFrame);


        retranslateUi(shaderInputEditor);
        QObject::connect(wValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setWValue(int)));
        QObject::connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), shaderInputEditor, SLOT(activateValue(QTreeWidgetItem*,QTreeWidgetItem*)));
        QObject::connect(yValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setYValue(int)));
        QObject::connect(xValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setXValue(int)));
        QObject::connect(resetButton, SIGNAL(clicked()), shaderInputEditor, SLOT(resetValue()));
        QObject::connect(zValue, SIGNAL(valueChanged(int)), shaderInputEditor, SLOT(setZValue(int)));

        QMetaObject::connectSlotsByName(shaderInputEditor);
    } // setupUi

    void retranslateUi(QWidget *shaderInputEditor)
    {
        shaderInputEditor->setWindowTitle(QApplication::translate("shaderInputEditor", "Form", 0, QApplication::UnicodeUTF8));
        titleLabel->setText(QApplication::translate("shaderInputEditor", "Shader-Input Editor", 0, QApplication::UnicodeUTF8));
        xLabel->setText(QApplication::translate("shaderInputEditor", "x", 0, QApplication::UnicodeUTF8));
        nameLabel->setText(QApplication::translate("shaderInputEditor", "name", 0, QApplication::UnicodeUTF8));
        typeValue->setText(QApplication::translate("shaderInputEditor", "float", 0, QApplication::UnicodeUTF8));
        xValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        nameValue->setText(QApplication::translate("shaderInputEditor", "uniform name", 0, QApplication::UnicodeUTF8));
        zValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        zLabel->setText(QApplication::translate("shaderInputEditor", "z", 0, QApplication::UnicodeUTF8));
        wLabel->setText(QApplication::translate("shaderInputEditor", "w", 0, QApplication::UnicodeUTF8));
        yLabel->setText(QApplication::translate("shaderInputEditor", "y", 0, QApplication::UnicodeUTF8));
        yValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        resetButton->setText(QApplication::translate("shaderInputEditor", "Reset", 0, QApplication::UnicodeUTF8));
        wValueLabel->setText(QApplication::translate("shaderInputEditor", "0.0", 0, QApplication::UnicodeUTF8));
        typeLabel->setText(QApplication::translate("shaderInputEditor", "type", 0, QApplication::UnicodeUTF8));
        descriptionLabel->setText(QApplication::translate("shaderInputEditor", "brief description of the uniform.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class shaderInputEditor: public Ui_shaderInputEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SHADER_2D_INPUT_2D_EDITORUP2688_H
