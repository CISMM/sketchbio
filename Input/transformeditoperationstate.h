#ifndef TRANSFORMEDITOPERATIONSTATE_H
#define TRANSFORMEDITOPERATIONSTATE_H

#include <QObject>
#include <QVector>

class SketchObject;
#include <sketchproject.h>

class TransformEditOperationState : public QObject, public SketchBio::OperationState {
    Q_OBJECT;
public:
    TransformEditOperationState(SketchBio::Project* p) : OperationState(), proj(p) {}
    virtual ~TransformEditOperationState() {}
    void addObject(SketchObject* obj) {
        objs.append(obj);
    }
    QVector<SketchObject*>& getObjs() {
        return objs;
    }

public slots:
    void setObjectTransform(double,double,double,double,double,double);
    void cancelOperation();

private:
    QVector<SketchObject*> objs;
    SketchBio::Project *proj;
};

#endif // TRANSFORMEDITOPERATIONSTATE_H
