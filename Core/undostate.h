#ifndef UNDOSTATE_H
#define UNDOSTATE_H

class SketchProject;

/*
 * This class is an abstract class that represents a state change of the project.
 */

class UndoState
{
public:
    UndoState( SketchProject &proj );
    virtual ~UndoState();
    virtual void undo() = 0;
    virtual void redo() = 0;
    SketchProject const &getProject() const;
protected:
    SketchProject &project;
};

#endif // UNDOSTATE_H
