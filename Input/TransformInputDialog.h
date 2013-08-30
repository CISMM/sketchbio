#ifndef TRANSFORMINPUTDIALOG_H
#define TRANSFORMINPUTDIALOG_H

#include <QDialog>

#include <QVector>

class QDoubleSpinBox;

class TransformInputDialog : public QDialog
{
    Q_OBJECT
public:
    // Creates a new TransformInputDialog with the given title
    explicit TransformInputDialog(const QString &title,QWidget *parent = 0);

    // Gets the translation of the transform as a vector.  The parameter should
    // be an array of length >=3 (the first 3 positions will be overwritten)
    void getTranslation(double *out);
    // Gets the rotation of the transform input as a vector.  The parameter
    // should be an array of length >= 3 (the first 3 positions will be overwritten)
    void getRotation(double *out);

    // Sets the translation of the transform from the given 3-vector.  This
    // assumes the parameter is the address of the first of 3 doubles
    void setTranslation(double *in);
    // Sets the rotation of the transform from the given 3-vector.  This
    // assumes the parameter is the address of the first of 3 doubles
    void setRotation(double *in);
    
signals:
    // emitted when Ok is pressed to send the resulting transform via a signal
    void transformAquired(double xTranslation, double yTranslation,
                          double zTranslation, double xRotation,
                          double yRotation, double zRotation);
    
public slots:
    void emitTransformAndClose();

private:
    QDoubleSpinBox *makeTransformationEditBox(bool inputIsAngle = false);

    QVector< QDoubleSpinBox * > translate;
    QVector< QDoubleSpinBox * > rotate;
    
};

#endif // TRANSFORMINPUTDIALOG_H
