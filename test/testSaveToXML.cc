
#include <sketchioconstants.h>
#include <projecttoxml.h>
#include <vtkRenderer.h>
#include <vtkXMLUtilities.h>

#include <iostream>

using namespace std;

void compareNumbers(SketchProject *proj1, SketchProject *proj2, int &retVal) {
    if (proj2->getModelManager()->getNumberOfModels() != proj1->getModelManager()->getNumberOfModels()) {
        retVal++;
        cout << "Number of models different" << endl;
    }
    if (proj2->getWorldManager()->getNumberOfObjects() != proj1->getWorldManager()->getNumberOfObjects()) {
        retVal++;
        cout << "Number of objects is different" << endl;
    }
    if (proj1->getWorldManager()->getNumberOfSprings() != proj2->getWorldManager()->getNumberOfSprings()) {
        retVal++;
        cout << "Number of springs is different" << endl;
    }
    if (proj1->getNumberOfReplications() != proj2->getNumberOfReplications()) {
        retVal++;
        cout << "Number of replications is different" << endl;
    }
}

void compareObjects(const SketchObject *o1, const SketchObject *o2, int &numDifferences, bool printDiffs = false) {
    const ReplicatedObject *r1 = dynamic_cast<const ReplicatedObject *>(o1);
    const ReplicatedObject *r2 = dynamic_cast<const ReplicatedObject *>(o2);
    double epsilon = Q_EPSILON;
    if (r1 != NULL && r2 != NULL) {
        if (r1->getReplicaNum() != r2->getReplicaNum()) {
            numDifferences++;
            if (printDiffs) cout << "Replicas with different replica nums" << endl;
            return;
        } else {
            epsilon *= r1->getReplicaNum();
        }
    } else if ((r1 == NULL) ^ (r2 == NULL)) {
        numDifferences++;
        if (printDiffs) cout << "One is a replica, the other is not" << endl;
        return;
    }
    q_vec_type pos1, pos2;
    q_type orient1, orient2;
    o1->getPosition(pos1);
    o2->getOrientation(orient1);
    o2->getPosition(pos2);
    o2->getOrientation(orient2);
    for (int i = 0; i < 3; i++) {
        if (Q_ABS(pos1[i] - pos2[i]) > epsilon) {
            numDifferences++;
            if (printDiffs) {
                cout << "positions wrong" << endl;
                cout << "P1: ";
                q_vec_print(pos1);
                cout << "P2: ";
                q_vec_print(pos2);
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        if (Q_ABS(orient1[i] - orient2[i]) > epsilon) {
            numDifferences++;
            if (printDiffs) {
                cout << "orientations wrong" << endl;
                cout << "O1: ";
                q_print(orient1);
                cout << "O2: ";
                q_print(orient2);
                cout << "Difference in " << i << " of " << (orient1[i]-orient2[i]) << endl;
            }
        }
    }
    if (o1->doPhysics() != o2->doPhysics()) {
        numDifferences++;
        if (printDiffs) {
            cout << "Physics info wrong" << endl;
            cout << o1->doPhysics() << " " << o2->doPhysics() << endl;
        }
    }

    if (o2->getConstModel()->getDataSource() != o1->getConstModel()->getDataSource()) {
        numDifferences++;
        if (printDiffs) {
            cout << "Model got mixed up" << endl;
        }
    }
}

void compareObjectLists(SketchProject *proj1, SketchProject *proj2, int &retVal) {
    bool *used = new bool[proj2->getWorldManager()->getNumberOfObjects()];
    for (int i = 0; i < proj2->getWorldManager()->getNumberOfObjects(); i++) {
        used[i] = false;
    }
    for (QListIterator<SketchObject *> it1 = proj1->getWorldManager()->getObjectIterator();
         it1.hasNext();) {
        const SketchObject *n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QListIterator<SketchObject *> it2 = proj2->getWorldManager()->getObjectIterator();
             it2.hasNext();i++) {
            const SketchObject *n2 = it2.next();
            if (!used[i]) {
                int diffs = 0;
                compareObjects(n1,n2,diffs);
                if (diffs == 0) {
                    used[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed) {
            retVal++;
            cout << "Could not match object." << endl;
        }
    }

    delete[] used;
}

void compareReplications(const StructureReplicator *rep1, const StructureReplicator *rep2,
                        int &diffs, bool printDiffs = false) {
    int v = 0;
    compareObjects(rep1->getFirstObject(),rep2->getFirstObject(),v,printDiffs);
    if ( v != 0) {
        diffs++;
        if (printDiffs) cout << "First objects didn't match" << endl;
    }
    v = 0;
    compareObjects(rep1->getSecondObject(),rep2->getSecondObject(),v,printDiffs);
    if ( v != 0) {
        diffs++;
        if (printDiffs) cout << "Second objects didn't match" << endl;
    }
    if (rep1->getNumShown() != rep2->getNumShown()) {
        diffs++;
        if (printDiffs) cout << "Number of replicas different" << endl;
    }
}

void compareSprings(const SpringConnection *sp1, const SpringConnection *sp2,
                    int &diffs, bool printDiffs = false) {
    const InterObjectSpring *isp1 = dynamic_cast<const InterObjectSpring *>(sp1);
    const InterObjectSpring *isp2 = dynamic_cast<const InterObjectSpring *>(sp2);
    if ((isp1 == NULL) ^ (isp2 == NULL)) {
        diffs++;
        if (printDiffs) cout << "Different kinds of spring" << endl;
        return;
    }
    int v = 0;
    q_vec_type pos1, pos2;
    compareObjects(sp1->getObject1(),sp2->getObject1(),v,printDiffs);
    if (v != 0) {
        if (isp2 != NULL) { // they are 2-obj springs
            v = 0;
            compareObjects(sp1->getObject1(),isp2->getObject2(),v,printDiffs);
            if (v != 0) {
                diffs++;
                if (printDiffs) cout << "Cannot match objects in 2-obj spring." << endl;
            } else {
                isp1->getObject1ConnectionPosition(pos1);
                isp2->getObject2ConnectionPosition(pos2);
                for (int i = 0; i < 3; i++) {
                    if (Q_ABS(pos1[i]-pos2[i]) > Q_EPSILON) {
                        diffs++;
                        if (printDiffs) cout << "Object connection positions don't match" << endl;
                    }
                }
                v = 0;
                compareObjects(isp1->getObject2(),isp2->getObject1(),v,printDiffs);
                if (v != 0) {
                    diffs++;
                    if (printDiffs) cout << "Cannot match objects in 2-obj spring." << endl;
                } else {
                    isp1->getObject2ConnectionPosition(pos1);
                    isp2->getObject1ConnectionPosition(pos2);
                    for (int i = 0; i < 3; i++) {
                        if (Q_ABS(pos1[i]-pos2[i]) > Q_EPSILON) {
                            diffs++;
                            if (printDiffs) cout << "Object connection positions don't match" << endl;
                        }
                    }
                }
            }
        } else { // they are 1-obj springs
            diffs++;
            if (printDiffs) cout << "Cannot match objects in 1-obj spring" << endl;
        }
    } else { // the first objects match
        sp1->getObject1ConnectionPosition(pos1);
        sp2->getObject1ConnectionPosition(pos2);
        for (int i = 0; i < 3; i++) {
            if (Q_ABS(pos1[i]-pos2[i]) > Q_EPSILON) {
                diffs++;
                if (printDiffs) cout << "Object connection positions don't match" << endl;
            }
        }
        if (isp1 != NULL) { // if they are 2-obj springs
            v = 0;
            compareObjects(isp1->getObject2(),isp2->getObject2(),v,printDiffs);
            if (v != 0) {
                diffs++;
                if (printDiffs) cout << "Unable to match objects in 2-obj spring" << endl;
            } else { // if the second objects match
                isp1->getObject2ConnectionPosition(pos1);
                isp2->getObject2ConnectionPosition(pos2);
                for (int i = 0; i < 3; i++) {
                    if (Q_ABS(pos1[i]-pos2[i]) > Q_EPSILON) {
                        diffs++;
                        if (printDiffs) cout << "Object connection positions don't match" << endl;
                    }
                }
                if (diffs) {
                    q_vec_print(pos1);
                    q_vec_print(pos2);
                }
            }
        } else { // if they are 1-obj springs test the world position of the 2nd endpoint

            sp1->getEnd2WorldPosition(pos1);
            sp2->getEnd2WorldPosition(pos2);
            for (int i = 0; i < 3; i++) {
                if (Q_ABS(pos1[i]-pos2[i]) > Q_EPSILON) {
                    diffs++;
                    if (printDiffs) cout << "World connection positions don't match" << endl;
                }
            }
        }
    }
    if (Q_ABS(sp1->getStiffness()-sp2->getStiffness()) > Q_EPSILON) {
        diffs++;
        if (printDiffs) cout << "Stiffness is different" << endl;
    }
    if (Q_ABS(sp1->getMinRestLength() - sp2->getMinRestLength()) > Q_EPSILON) {
        diffs++;
        if (printDiffs) cout << "Minimum rest length changed" << endl;
    }
    if (Q_ABS(sp1->getMaxRestLength() - sp2->getMaxRestLength()) > Q_EPSILON) {
        diffs++;
        if (printDiffs) cout << "Maximum rest length changed" << endl;
    }
}

int testSave1() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    SketchProject *proj1 = new SketchProject(r1,a,b);
    SketchProject *proj2 = new SketchProject(r2,a,b);
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");

    SketchModel *m1 = proj1->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);

    vtkXMLDataElement *root = projectToXML(proj1);

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    xmlToProject(proj2,root);

    compareNumbers(proj1,proj2,retVal);
    const SketchObject *o2 = proj2->getWorldManager()->getObjectIterator().next();
    compareObjects(o1,o2,retVal);

    if (retVal == 0) {
        cout << "Passed test 1" << endl;
    }

    root->Delete();
    delete proj1;
    delete proj2;
    return retVal;
}


int testSave2() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    SketchProject *proj1 = new SketchProject(r1,a,b);
    SketchProject *proj2 = new SketchProject(r2,a,b);
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");

    SketchModel *m1 = proj1->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    proj1->addObject(m1,pos1,orient1);

    vtkXMLDataElement *root = projectToXML(proj1);

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    xmlToProject(proj2,root);

    compareNumbers(proj1,proj2,retVal);
    compareObjectLists(proj1,proj2,retVal);

    if (retVal == 0) {
        cout << "Passed test 2" << endl;
    }

    root->Delete();
    delete proj1;
    delete proj2;
    return retVal;
}

int testSave3() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    SketchProject *proj1 = new SketchProject(r1,a,b);
    SketchProject *proj2 = new SketchProject(r2,a,b);
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");

    SketchModel *m1 = proj1->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    proj1->addReplication(o1,o2,12);


    vtkXMLDataElement *root = projectToXML(proj1);

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    xmlToProject(proj2,root);

    compareNumbers(proj1,proj2,retVal);
    compareObjectLists(proj1,proj2,retVal);
    compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);

    if (retVal == 0) {
        cout << "Passed test 3" << endl;
    }

    root->Delete();
    delete proj1;
    delete proj2;
    return retVal;
}

int testSave4() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    SketchProject *proj1 = new SketchProject(r1,a,b);
    SketchProject *proj2 = new SketchProject(r2,a,b);
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");

    SketchModel *m1 = proj1->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = proj1->addObject(m1,pos1,orient1);
    proj1->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    proj1->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)


    vtkXMLDataElement *root = projectToXML(proj1);

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    xmlToProject(proj2,root);

    compareNumbers(proj1,proj2,retVal);
    compareObjectLists(proj1,proj2,retVal);
    compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);
    compareSprings(proj1->getWorldManager()->getSpringsIterator().next(),
                   proj2->getWorldManager()->getSpringsIterator().next(),retVal,true);

    if (retVal == 0) {
        cout << "Passed test 4" << endl;
    }

    root->Delete();
    delete proj1;
    delete proj2;
    return retVal;
}

int main() {
    return testSave1() + testSave2() + testSave3() + testSave4();
}
