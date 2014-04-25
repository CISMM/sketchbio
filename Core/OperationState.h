#ifndef OPERATIONSTATE_H
#define OPERATIONSTATE_H

namespace SketchBio {

class OperationState
{
   public:
    OperationState() {}
    virtual ~OperationState() {}
    virtual void doFrameUpdates() {}
};

}

#endif // OPERATIONSTATE_H
