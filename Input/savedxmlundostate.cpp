#include "savedxmlundostate.h"

#include <sstream>

#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>
#include <vtkXMLUtilities.h>

#include <sketchproject.h>

#include <projecttoxml.h>

SavedXMLUndoState::SavedXMLUndoState(SketchProject &proj,
                                     QSharedPointer<std::string> previous)
    :
      UndoState(proj),
      before(previous),
      after(NULL)
{
    vtkSmartPointer< vtkXMLDataElement > xml =
            vtkSmartPointer< vtkXMLDataElement >::Take(
                ProjectToXML::projectToXML(&project)
                );
    std::stringstream stream;
    vtkXMLUtilities::FlattenElement(xml,stream);
    after = QSharedPointer< std::string >( new std::string(stream.str()) );
}

inline void restoreToSavePoint(QSharedPointer< std::string > &savePt,
                               SketchProject &project)
{
    if (savePt)
    {
        vtkSmartPointer< vtkXMLDataElement > xml =
                vtkSmartPointer< vtkXMLDataElement >::Take(
                    vtkXMLUtilities::ReadElementFromString(savePt->c_str())
                    );
        if (!xml)
            return;
        project.clearProject();
        ProjectToXML::xmlToProject(&project,xml);
		project.setViewTime(project.getViewTime());
    }
}

void SavedXMLUndoState::undo()
{
    restoreToSavePoint(before,project);
}

void SavedXMLUndoState::redo()
{
    restoreToSavePoint(after,project);
}

QWeakPointer< std::string > SavedXMLUndoState::getBeforeState()
{
    return before.toWeakRef();
}

QWeakPointer< std::string > SavedXMLUndoState::getAfterState()
{
    return after.toWeakRef();
}
