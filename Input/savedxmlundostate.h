#ifndef SAVEDXMLUNDOSTATE_H
#define SAVEDXMLUNDOSTATE_H

#include <string>

#include <QSharedPointer>

#include <undostate.h>

class SavedXMLUndoState : public UndoState
{
public:
    SavedXMLUndoState( SketchProject &proj,
                       QSharedPointer< std::string > previous);
    virtual void undo();
    virtual void redo();
private:
    QSharedPointer< std::string > before, after;
};

#endif // SAVEDXMLUNDOSTATE_H
