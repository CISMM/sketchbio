#ifndef TRANSFORMEDITOPERATIONSTATE_H
#define TRANSFORMEDITOPERATIONSTATE_H

#include <QObject>
#include <QVector>

class SketchObject;
namespace SketchBio {
class Project;
}
#include "OperationState.h"

class TransformEditOperationState : public QObject, public SketchBio::OperationState {
    Q_OBJECT
    Q_DISABLE_COPY(TransformEditOperationState)
public:
    TransformEditOperationState(SketchBio::Project* p);
    void addObject(SketchObject* obj);
    QVector<SketchObject*>& getObjs();

    static const char SET_TRANSFORMS_OPERATION_FUNCTION[30];

public slots:
    void setObjectTransform(double,double,double,double,double,double);
    void cancelOperation();

private:
    QVector<SketchObject*> objs;
    SketchBio::Project *proj;
};

#endif // TRANSFORMEDITOPERATIONSTATE_H
