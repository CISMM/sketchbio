#ifndef BLENDERDECIMATIONRUNNER_H
#define BLENDERDECIMATIONRUNNER_H

#include "abstractsingleprocessrunner.h"

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
    explicit BlenderDecimationRunner(const QString &objFile, DecimationType::Type type,
                                     double param, QObject *parent = 0);

    virtual void start();
    virtual bool isValid();
protected:
    virtual bool didProcessSucceed(QString output);

private:
    QTemporaryFile *tempFile;
    bool valid;
};

#endif // BLENDERDECIMATIONRUNNER_H
