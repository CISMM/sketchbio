/*
 * This class is an abstract class that represents a state change of the project.
 */
#include "undostate.h"

#include "sketchproject.h"

UndoState::UndoState(SketchBio::Project &proj)
    :
      project(proj)
{
}

UndoState::~UndoState()
{
}

SketchBio::Project const &UndoState::getProject() const
{
    return project;
}
