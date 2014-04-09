#ifndef UNDOSTATE_H
#define UNDOSTATE_H

namespace SketchBio {
class Project;
}

/*
 * This class is an abstract class that represents a state change of the project.
 */

class UndoState
{
public:
    UndoState( SketchBio::Project &proj );
    virtual ~UndoState();
    virtual void undo() = 0;
    virtual void redo() = 0;
    SketchBio::Project const &getProject() const;
protected:
    SketchBio::Project &project;
};

#endif // UNDOSTATE_H
