/********************************************************************************
** Form generated from reading UI file 'shader-input-editorzU4307.ui'
**
** Created by: Qt User Interface Compiler version 4.8.5
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SHADER_2D_INPUT_2D_EDITORZU4307_H
#define SHADER_2D_INPUT_2D_EDITORZU4307_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_shaderInputEditor
{
public:
    QGridLayout *gridLayout;
    QSplitter *splitter;
    QTreeWidget *treeWidget;
    QFrame *frame;
    QVBoxLayout *verticalLayout;
    QFrame *valueFrame;
    QFormLayout *formLayout;
    QLabel *nameLabel;
    QLabel *nameValue;
    QLabel *typeLabel;
    QLabel *typeValue;
    QLabel *xLabel;
    QLabel *yLabel;
    QLabel *zLabel;
    QLabel *wLabel;
    QLineEdit *yValueEdit;
    QLineEdit *wValueEdit;
    QLineEdit *zValueEdit;
    QLineEdit *xValueEdit;
    QWidget *buttonContainer;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *resetButton;
    QPushButton *applyButton;

    void setupUi(QWidget *shaderInputEditor)
    {
        if (shaderInputEditor->objectName().isEmpty())
            shaderInputEditor->setObjectName(QString::fromUtf8("shaderInputEditor"));
        shaderInputEditor->resize(600, 500);
        shaderInputEditor->setStyleSheet(QString::fromUtf8("\n"
"QWidget#shaderInputEditor {\n"
"       background-color: #edeceb;\n"
"}\n"
"\n"
"QFrame#titleLabel,#treeWidget,#valueFrame,#buttonContainer {\n"
"       background-color: #edeceb;\n"
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
"       background-color: #fafbfe;"
                        "\n"
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
        gridLayout = new QGridLayout(shaderInputEditor);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        splitter = new QSplitter(shaderInputEditor);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setLineWidth(0);
        splitter->setOrientation(Qt::Horizontal);
        treeWidget = new QTreeWidget(splitter);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        treeWidget->setHeaderItem(__qtreewidgetitem);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setEnabled(true);
        treeWidget->setMaximumSize(QSize(16777215, 16777215));
        treeWidget->setFocusPolicy(Qt::StrongFocus);
        treeWidget->setStyleSheet(QString::fromUtf8("QTreeWidget::item {\n"
"    height: 22px;\n"
"}"));
        treeWidget->setFrameShape(QFrame::StyledPanel);
        treeWidget->setFrameShadow(QFrame::Plain);
        treeWidget->setLineWidth(1);
        treeWidget->setRootIsDecorated(false);
        treeWidget->setAnimated(true);
        splitter->addWidget(treeWidget);
        treeWidget->header()->setVisible(false);
        frame = new QFrame(splitter);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setFocusPolicy(Qt::StrongFocus);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Plain);
        verticalLayout = new QVBoxLayout(frame);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        valueFrame = new QFrame(frame);
        valueFrame->setObjectName(QString::fromUtf8("valueFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(valueFrame->sizePolicy().hasHeightForWidth());
        valueFrame->setSizePolicy(sizePolicy);
        valueFrame->setFocusPolicy(Qt::StrongFocus);
        valueFrame->setAutoFillBackground(false);
        valueFrame->setFrameShape(QFrame::NoFrame);
        valueFrame->setFrameShadow(QFrame::Plain);
        valueFrame->setLineWidth(0);
        formLayout = new QFormLayout(valueFrame);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        nameLabel = new QLabel(valueFrame);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));
        nameLabel->setMinimumSize(QSize(50, 24));
        nameLabel->setMaximumSize(QSize(80, 16777215));
        QFont font;
        font.setBold(true);
        font.setUnderline(false);
        font.setWeight(75);
        nameLabel->setFont(font);
        nameLabel->setAlignment(Qt::AlignCenter);

        formLayout->setWidget(0, QFormLayout::LabelRole, nameLabel);

        nameValue = new QLabel(valueFrame);
        nameValue->setObjectName(QString::fromUtf8("nameValue"));
        nameValue->setMaximumSize(QSize(150, 24));

        formLayout->setWidget(0, QFormLayout::FieldRole, nameValue);

        typeLabel = new QLabel(valueFrame);
        typeLabel->setObjectName(QString::fromUtf8("typeLabel"));
        typeLabel->setMinimumSize(QSize(50, 24));
        typeLabel->setMaximumSize(QSize(80, 16777215));
        QFont font1;
        font1.setBold(true);
        font1.setWeight(75);
        typeLabel->setFont(font1);
        typeLabel->setAlignment(Qt::AlignCenter);

        formLayout->setWidget(1, QFormLayout::LabelRole, typeLabel);

        typeValue = new QLabel(valueFrame);
        typeValue->setObjectName(QString::fromUtf8("typeValue"));
        typeValue->setMaximumSize(QSize(150, 24));

        formLayout->setWidget(1, QFormLayout::FieldRole, typeValue);

        xLabel = new QLabel(valueFrame);
        xLabel->setObjectName(QString::fromUtf8("xLabel"));
        xLabel->setMinimumSize(QSize(50, 24));
        xLabel->setMaximumSize(QSize(80, 16777215));
        xLabel->setFont(font1);
        xLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(2, QFormLayout::LabelRole, xLabel);

        yLabel = new QLabel(valueFrame);
        yLabel->setObjectName(QString::fromUtf8("yLabel"));
        yLabel->setMinimumSize(QSize(50, 24));
        yLabel->setMaximumSize(QSize(80, 16777215));
        yLabel->setFont(font1);
        yLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(3, QFormLayout::LabelRole, yLabel);

        zLabel = new QLabel(valueFrame);
        zLabel->setObjectName(QString::fromUtf8("zLabel"));
        zLabel->setMinimumSize(QSize(50, 24));
        zLabel->setMaximumSize(QSize(80, 16777215));
        zLabel->setFont(font1);
        zLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(4, QFormLayout::LabelRole, zLabel);

        wLabel = new QLabel(valueFrame);
        wLabel->setObjectName(QString::fromUtf8("wLabel"));
        wLabel->setMinimumSize(QSize(50, 24));
        wLabel->setMaximumSize(QSize(80, 16777215));
        wLabel->setFont(font1);
        wLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(5, QFormLayout::LabelRole, wLabel);

        yValueEdit = new QLineEdit(valueFrame);
        yValueEdit->setObjectName(QString::fromUtf8("yValueEdit"));
        yValueEdit->setMouseTracking(false);
        yValueEdit->setAcceptDrops(false);

        formLayout->setWidget(3, QFormLayout::FieldRole, yValueEdit);

        wValueEdit = new QLineEdit(valueFrame);
        wValueEdit->setObjectName(QString::fromUtf8("wValueEdit"));
        wValueEdit->setMouseTracking(false);
        wValueEdit->setAcceptDrops(false);

        formLayout->setWidget(5, QFormLayout::FieldRole, wValueEdit);

        zValueEdit = new QLineEdit(valueFrame);
        zValueEdit->setObjectName(QString::fromUtf8("zValueEdit"));
        zValueEdit->setMouseTracking(false);
        zValueEdit->setAcceptDrops(false);

        formLayout->setWidget(4, QFormLayout::FieldRole, zValueEdit);

        xValueEdit = new QLineEdit(valueFrame);
        xValueEdit->setObjectName(QString::fromUtf8("xValueEdit"));
        xValueEdit->setMouseTracking(false);
        xValueEdit->setAcceptDrops(false);

        formLayout->setWidget(2, QFormLayout::FieldRole, xValueEdit);


        verticalLayout->addWidget(valueFrame);

        buttonContainer = new QWidget(frame);
        buttonContainer->setObjectName(QString::fromUtf8("buttonContainer"));
        buttonContainer->setFocusPolicy(Qt::StrongFocus);
        horizontalLayout = new QHBoxLayout(buttonContainer);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(9, 9, -1, 9);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        resetButton = new QPushButton(buttonContainer);
        resetButton->setObjectName(QString::fromUtf8("resetButton"));
        resetButton->setFocusPolicy(Qt::NoFocus);
        QIcon icon;
        QString iconThemeName = QString::fromUtf8("document-revert");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        resetButton->setIcon(icon);

        horizontalLayout->addWidget(resetButton);

        applyButton = new QPushButton(buttonContainer);
        applyButton->setObjectName(QString::fromUtf8("applyButton"));
        applyButton->setFocusPolicy(Qt::NoFocus);
        QIcon icon1;
        iconThemeName = QString::fromUtf8("document-save");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon1 = QIcon::fromTheme(iconThemeName);
        } else {
            icon1.addFile(QString::fromUtf8(""), QSize(), QIcon::Normal, QIcon::Off);
        }
        applyButton->setIcon(icon1);

        horizontalLayout->addWidget(applyButton);


        verticalLayout->addWidget(buttonContainer);

        splitter->addWidget(frame);

        gridLayout->addWidget(splitter, 0, 0, 1, 1);


        retranslateUi(shaderInputEditor);
        QObject::connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), shaderInputEditor, SLOT(activateValue(QTreeWidgetItem*,QTreeWidgetItem*)));
        QObject::connect(applyButton, SIGNAL(clicked()), shaderInputEditor, SLOT(valueUpdated()));
        QObject::connect(resetButton, SIGNAL(clicked()), shaderInputEditor, SLOT(resetValue()));
        QObject::connect(wValueEdit, SIGNAL(returnPressed()), applyButton, SLOT(click()));
        QObject::connect(xValueEdit, SIGNAL(returnPressed()), applyButton, SLOT(click()));
        QObject::connect(yValueEdit, SIGNAL(returnPressed()), applyButton, SLOT(click()));
        QObject::connect(zValueEdit, SIGNAL(returnPressed()), applyButton, SLOT(click()));

        QMetaObject::connectSlotsByName(shaderInputEditor);
    } // setupUi

    void retranslateUi(QWidget *shaderInputEditor)
    {
        shaderInputEditor->setWindowTitle(QApplication::translate("shaderInputEditor", "Form", nullptr));
        nameLabel->setText(QApplication::translate("shaderInputEditor", "name", nullptr));
        nameValue->setText(QApplication::translate("shaderInputEditor", "uniform name", nullptr));
        typeLabel->setText(QApplication::translate("shaderInputEditor", "type", nullptr));
        typeValue->setText(QApplication::translate("shaderInputEditor", "float", nullptr));
        xLabel->setText(QApplication::translate("shaderInputEditor", "x", nullptr));
        yLabel->setText(QApplication::translate("shaderInputEditor", "y", nullptr));
        zLabel->setText(QApplication::translate("shaderInputEditor", "z", nullptr));
        wLabel->setText(QApplication::translate("shaderInputEditor", "w", nullptr));
        resetButton->setText(QApplication::translate("shaderInputEditor", "Reset", nullptr));
        applyButton->setText(QApplication::translate("shaderInputEditor", "Apply", nullptr));
    } // retranslateUi

};

namespace Ui {
    class shaderInputEditor: public Ui_shaderInputEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SHADER_2D_INPUT_2D_EDITORZU4307_H
