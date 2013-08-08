#ifndef TESTCOREHELPERS_H
#define TESTCOREHELPERS_H

class SketchModel;
class SketchObject;

namespace TestCoreHelpers
{
//#########################################################################
// Creates a SketchModel that is just a vtkCubeSource... centered on the origin,
// side length 2
SketchModel *getCubeModel();
//#########################################################################
// Creates a SketchModel that is just a vtkSphereSource... raduis of 4,
// some tests need to know the size of the model
SketchModel *getSphereModel();
//#########################################################################
// tests if the behavioral methods of a SketchObject are working.  This includes
// get/set methods.  Assumes a newly constructed SketchObject with no modifications
// made to it yet. Used in tests for multiple object types.
int testSketchObjectActions(SketchObject *obj);
//#########################################################################
// tests if the SketchObject initialized correctly (note if methods used are
//  overridden, this should not be called, use your own test function to
//  check initial state).  This does not modify the SketchObject, just makes
//  sure that the initial state was set up correctly. Used for multiple types
//  of object's tests
int testNewSketchObject(SketchObject *obj);
}

#endif // TESTCOREHELPERS_H
