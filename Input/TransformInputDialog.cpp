#include "TransformInputDialog.h"

#include <quat.h>

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

TransformInputDialog::TransformInputDialog(const QString &title,QWidget *parent) :
    QDialog(parent),
    translate(),
    rotate()
{
    this->setModal(true);
    this->setWindowTitle(title);
    QGridLayout *layout = new QGridLayout(this);
    this->setLayout(layout);
    char axisName = 'X';
    QLabel *label = new QLabel("Translation",this);
    layout->addWidget(label,1,0);
    label = new QLabel("Rotation",this);
    layout->addWidget(label,2,0);
    for (int i = 0; i < 3; i++)
    {
        label = new QLabel(QString("%1:").arg(axisName));
        layout->addWidget(label, 1, 2*i+1);
        label = new QLabel(QString("%1:").arg(axisName));
        layout->addWidget(label, 2, 2*i+1);
        QDoubleSpinBox *spinBox = this->makeTransformationEditBox();
        layout->addWidget(spinBox, 1, 2*(i+1));
        translate.append(spinBox);
        spinBox = this->makeTransformationEditBox(true);
        layout->addWidget(spinBox, 2, 2*(i+1));
        rotate.append(spinBox);
        axisName++;
    }
    QPushButton *okButton = new QPushButton("OK",this);
    layout->addWidget(okButton,3,5);
    QPushButton *cancelButton = new QPushButton("Cancel",this);
    layout->addWidget(cancelButton,3,6);
    this->connect(okButton,SIGNAL(clicked()),this,SLOT(emitTransformAndClose()));
    this->connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
}

void TransformInputDialog::getTranslation(double *out)
{
    out[0] = translate[0]->value();
    out[1] = translate[1]->value();
    out[2] = translate[2]->value();
}

void TransformInputDialog::getRotation(double *out)
{
    out[0] = rotate[0]->value();
    out[1] = rotate[1]->value();
    out[2] = rotate[2]->value();
}

void TransformInputDialog::setTranslation(double *in)
{
    translate[0]->setValue(in[0]);
    translate[1]->setValue(in[1]);
    translate[2]->setValue(in[2]);
}

void TransformInputDialog::setRotation(double *in)
{
    for (int i = 0; i < 3; i++)
    {
        if (in[i] < 0)
        {
            in[i] += 360.0;
        }
    }
    rotate[0]->setValue(in[0]);
    rotate[1]->setValue(in[1]);
    rotate[2]->setValue(in[2]);
}

void TransformInputDialog::emitTransformAndClose()
{
    emit transformAquired(
                this->translate[0]->value(),
            this->translate[1]->value(),
            this->translate[2]->value(),
            this->rotate[0]->value(),
            this->rotate[1]->value(),
            this->rotate[2]->value());
    this->accept();
}

QDoubleSpinBox *TransformInputDialog::makeTransformationEditBox(bool inputIsAngle)
{
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
    spinBox->setDecimals(6);
    if (inputIsAngle)
    {
        spinBox->setRange(0,359.999999);
        spinBox->setWrapping(true);
    }
    else
    {
        spinBox->setRange(-1000,1000);
    }
    return spinBox;
}

