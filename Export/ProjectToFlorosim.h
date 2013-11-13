#ifndef PROJECTTOFLOROSIM_H
#define PROJECTTOFLOROSIM_H

class SketchProject;
class QString;

/*
 * Handles writing SketchBio objects to Florosim
 */
namespace ProjectToFlorosim
{
    /*
     * Writes the current state of the given project to a Florosim xml file.
     */
    bool writeProjectToFlorosim(SketchProject* project, const QString& filename);
}

#endif // PROJECTTOFLOROSIM_H
