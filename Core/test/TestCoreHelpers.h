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
//#########################################################################
// tests if the ModelInstance initialized correctly (note: if methods used
//  either here or in testNewSketchObject are overridden, this should not be
//  called, use your own test funciton to check initial state).  This does
//  not modify the ModelInstance, just checks to see if the initial state is
//  set up correctly
int testNewModelInstance(SketchObject *obj,bool testSubObjects = true);
//#########################################################################
// tests if the behavioral methods of a ModelInstance are working. this includes
// get/set methods.  Assumes a newly constructed ModelInstance with no modifications
// made to it yet.
int testModelInstanceActions(SketchObject *obj);

//#########################################################################
// This class represents a test for SketchObjects
class ObjectTest {
public:
    virtual ~ObjectTest(){}
    // Tests the object or some feature of it and returns 0 for a pass and
    // 1 on failure
    virtual const char* getTestName() = 0;
    virtual int testObject(SketchObject* obj) = 0;
    typedef ObjectTest* (*Factory)(void*);
};
// A function that is a factory for ObjectTests

//#########################################################################
// A class for sets of ObjectTests to extend
class SetOfObjectTests {
protected:
    struct Node {
    public:
        Node* next;
        ObjectTest::Factory test;
        void* arg;
    };
    static int runTests(Node* n, SketchObject* testObject);
};
//#########################################################################
// This class keeps track of the available tests for SketchObject actions
class ObjectActionTest : SetOfObjectTests {
private:
    Node n;
    static Node* actionHead;
public:
    // Make a static one of these to add the given testmaker to the
    // list of testmakers
    ObjectActionTest(ObjectTest::Factory testMaker, void* arg);
    ~ObjectActionTest();
    // Runs all the tests for SketchObject actions
    static int runTests(SketchObject* testObject);
};
}

#endif // TESTCOREHELPERS_H
