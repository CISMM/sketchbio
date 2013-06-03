#ifndef BLENDERDECIMATIONRUNNER_H
#define BLENDERDECIMATIONRUNNER_H

#include "abstractsingleprocessrunner.h"

#include <QFile>
class QTemporaryFile;

namespace DecimationType
{
enum Type { PERCENT, POLYGON_COUNT };
}

/*
 *
 * This class is a SubprocessRunner that uses Blender to decimate complex meshes to make
 * them more usable.
 */
class BlenderDecimationRunner : public AbstractSingleProcessRunner
{
    Q_OBJECT
public:
    explicit BlenderDecimationRunner(QString objFile, DecimationType::Type type,
                                     int param, QObject *parent = 0);

    virtual void start();
    virtual bool isValid();
protected:
    virtual bool didProcessSucceed();

private:
    QFile *tempFile;
    bool valid;
};

#endif // BLENDERDECIMATIONRUNNER_H
