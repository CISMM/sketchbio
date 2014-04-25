#ifndef TRANSFORMEDITOPERATIONSTATE_H
#define TRANSFORMEDITOPERATIONSTATE_H

#include <QObject>
#include <QVector>

class SketchObject;
#include <sketchproject.h> // for OperationState class

class TransformEditOperationState : public QObject, public SketchBio::OperationState {
    Q_OBJECT
    Q_DISABLE_COPY(TransformEditOperationState)
public:
    TransformEditOperationState(SketchBio::Project* p);
    virtual ~TransformEditOperationState();
    void addObject(SketchObject* obj);
    QVector<SketchObject*>& getObjs();

public slots:
    void setObjectTransform(double,double,double,double,double,double);
    void cancelOperation();

private:
    QVector<SketchObject*> objs;
    SketchBio::Project *proj;
};

#endif // TRANSFORMEDITOPERATIONSTATE_H
