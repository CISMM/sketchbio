#ifndef PROJECTTOFLOROSIM_H
#define PROJECTTOFLOROSIM_H

namespace SketchBio {
class Project;
}
class QString;

/*
 * Handles writing SketchBio objects to Florosim
 */
namespace ProjectToFlorosim
{
    /*
     * Writes the current state of the given project to a Florosim xml file.
     */
    bool writeProjectToFlorosim(SketchBio::Project* project, const QString& filename);
}

#endif // PROJECTTOFLOROSIM_H
