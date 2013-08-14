/*
 * This class is an abstract class that represents a state change of the project.
 */
#include "undostate.h"

#include "sketchproject.h"

UndoState::UndoState(SketchProject &proj)
    :
      project(proj)
{
}

UndoState::~UndoState()
{
}

SketchProject const &UndoState::getProject() const
{
    return project;
}
