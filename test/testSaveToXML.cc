#include <sketchioconstants.h>
#include <projecttoxml.h>
#include <sketchtests.h>

#include <vtkRenderer.h>
#include <vtkXMLUtilities.h>

#include <iostream>

using namespace std;

// have to prototype these since they recursively call each other to deal with groups
void compareObjectLists(const QList<SketchObject *> *list1, const QList<SketchObject *> *list2,
                        int &retVal, bool printDiffs = true);
void compareObjects(const SketchObject *o1, const SketchObject *o2, int &numDifferences, bool printDiffs = false);


void compareNumbers(SketchProject *proj1, SketchProject *proj2, int &retVal) {
    // make sure both have camera models created (easier than making sure neither has it)
    proj1->getCameraModel();
    proj2->getCameraModel();
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
    if (proj1->getTransformOps()->size() != proj2->getTransformOps()->size()) {
        retVal++;
        cout << "Number of transform ops is different" << endl;
    }
    if (proj1->getCameras()->size() != proj2->getCameras()->size() ) {
        retVal++;
        cout << "Number of cameras is different" << endl;
    }
}

void compareObjects(const SketchObject *o1, const SketchObject *o2, int &numDifferences, bool printDiffs) {
    if (o1->numInstances() != o2->numInstances()) {
        numDifferences++;
        if (printDiffs) cout << "Different numbers of objects contained in groups!" << endl;
        return;
    }
    const ReplicatedObject *r1 = dynamic_cast<const ReplicatedObject *>(o1);
    const ReplicatedObject *r2 = dynamic_cast<const ReplicatedObject *>(o2);
    double epsilon = Q_EPSILON;
    const SketchObject *p = o1->getParent();
    while (p != NULL) {
        epsilon *= 4;
        p = p->getParent();
    }
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
    if (!q_vec_equals(pos1,pos2,epsilon)) {
        numDifferences++;
        if (printDiffs) {
            cout << "positions wrong" << endl;
            cout << "P1: ";
            q_vec_print(pos1);
            cout << "P2: ";
            q_vec_print(pos2);
        }
    }
    if (o1->numInstances() == 1) { // test single instance things... orientation of a group is tested by
                                    // position of group memebers in recursion
        if (!q_equals(orient1,orient2,epsilon)) {
            numDifferences++;
            if (printDiffs) {
                cout << "orientations wrong" << endl;
                cout << "O1: ";
                q_print(orient1);
                cout << "O2: ";
                q_print(orient2);
            }
        }

        if (o2->getModel()->getDataSource() != o1->getModel()->getDataSource()) {
            numDifferences++;
            if (printDiffs) {
                cout << "Model got mixed up" << endl;
            }
        }
    } else { // test group specific stuff
        compareObjectLists(o1->getSubObjects(),o2->getSubObjects(),numDifferences,printDiffs);
    }
    if (o1->getNumKeyframes() != o2->getNumKeyframes()) {
        numDifferences++;
        if (printDiffs) {
            cout << "Different numbers of keyframes." << endl;
            cout << "Object 1 has: " << o1->getNumKeyframes() << endl;
            cout << "Object 2 has: " << o2->getNumKeyframes() << endl;
        }
    } else if (o1->getNumKeyframes() > 0) {
        QMapIterator<double,Keyframe> it(*o1->getKeyframes());
        while (it.hasNext()) {
            double time = it.peekNext().key();
            const Keyframe &frame1 = it.next().value();
            if (!o2->getKeyframes()->contains(time)) {
                numDifferences++;
                if (printDiffs) cout << "Keyframe for time " << time << " missing in object 2." << endl;
            }
            const Keyframe &frame2 = o2->getKeyframes()->value(time);
            q_vec_type p1, p2;
            q_type o1, o2;
            frame1.getPosition(p1);
            frame2.getPosition(p2);
            frame1.getOrientation(o1);
            frame2.getOrientation(o2);
            if (!q_vec_equals(p1,p2,epsilon)) {
                numDifferences++;
                if (printDiffs) cout << "Keyframe positions are different at time " << time << endl;
            }
            if (!q_equals(o1,o2,epsilon)) {
                numDifferences++;
                if (printDiffs) cout << "Keyframe orientations are different at time " << time << endl;
            }
        }
    }
}

void compareObjectLists(const QList<SketchObject *> *list1, const QList<SketchObject *> *list2,
                        int &retVal, bool printDiffs) {
    QScopedPointer<bool, QScopedPointerArrayDeleter<bool> > used(new bool[list2->size()]);
    for (int i = 0; i < list2->size(); i++) {
        used.data()[i] = false;
    }
    for (QListIterator<SketchObject *> it1(*list1); it1.hasNext();) {
        const SketchObject *n1 = it1.next();
        int i = 0;
        bool failed = true;
        for (QListIterator<SketchObject *> it2(*list2); it2.hasNext();i++) {
            const SketchObject *n2 = it2.next();
            if (!used.data()[i]) {
                int diffs = 0;
                compareObjects(n1,n2,diffs,true);
                if (diffs == 0) {
                    used.data()[i] = true;
                    failed = false;
                    break;
                }
            }
        }
        if (failed) {
            retVal++;
            if (printDiffs) cout << "Could not match object." << endl;
        }
    }

}

void compareCameras(SketchProject *proj1, SketchProject *proj2, int &retVal) {
    const QHash<SketchObject *, vtkSmartPointer<vtkCamera> > *cameras1 = proj1->getCameras();
    const QHash<SketchObject *, vtkSmartPointer<vtkCamera> > *cameras2 = proj2->getCameras();
    for (QHashIterator<SketchObject *, vtkSmartPointer<vtkCamera> > it (*cameras1);
         it.hasNext(); ) {
        SketchObject *obj1 = it.next().key();
        bool match = false;
        for (QHashIterator<SketchObject *, vtkSmartPointer<vtkCamera> > it2 (*cameras2);
             it2.hasNext(); ) {
            SketchObject *obj2 = it2.next().key();
            int numDiffs = 0;
            compareObjects(obj1,obj2,numDiffs,true);
            if (numDiffs == 0) {
                match = true;
            }
        }
        if (match == false) {
            retVal++;
            cout << "Could not match camera." << endl;
        }
    }
}

void compareTransformOps(QSharedPointer<TransformEquals> t1, QSharedPointer<TransformEquals> t2,
                         int &retVal, bool printDiffs) {
    const QVector<ObjectPair> *l1 = t1->getPairsList(), *l2 = t2->getPairsList();
    if (l1->size() != l2->size()) {
        retVal++;
        if (printDiffs) cout << "Different numbers of object pairs in transform equals." << endl;
        return;
    }
    QScopedPointer<bool, QScopedPointerArrayDeleter<bool> > used(new bool[l2->size()]);
    for (int i = 0; i < l2->size(); i++) {
        used.data()[i] = false;
    }
    for (int i = 0; i < l1->size(); i++) {
        bool failed = true;
        for (int j = 0; j < l2->size(); j++) {
            if (!used.data()[j]) {
                int diffs = 0;
                compareObjects(l1->at(i).o1,l2->at(j).o1,diffs,false);
                compareObjects(l1->at(i).o2,l2->at(j).o2,diffs,false);
                if (diffs == 0) {
                    failed = false;
                    used.data()[i] = true;
                }
            }
        }
        if (failed) {
            retVal++;
            if (printDiffs) cout << "Could not match object pair" << endl;
        }
    }
}

void compareWorldObjects(SketchProject *proj1, SketchProject *proj2, int &retVal) {
    compareObjectLists(proj1->getWorldManager()->getObjects(),
                       proj2->getWorldManager()->getObjects(),
                       retVal);
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
                if (!q_vec_equals(pos1,pos2)) {
                    diffs++;
                    if (printDiffs) cout << "Object connection positions don't match" << endl;
                }
                v = 0;
                compareObjects(isp1->getObject2(),isp2->getObject1(),v,printDiffs);
                if (v != 0) {
                    diffs++;
                    if (printDiffs) cout << "Cannot match objects in 2-obj spring." << endl;
                } else {
                    isp1->getObject2ConnectionPosition(pos1);
                    isp2->getObject1ConnectionPosition(pos2);
                    if (!q_vec_equals(pos1,pos2)) {
                        diffs++;
                        if (printDiffs) cout << "Object connection positions don't match" << endl;
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
        if (!q_vec_equals(pos1,pos2)) {
            diffs++;
            if (printDiffs) cout << "Object connection positions don't match" << endl;
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
                if (!q_vec_equals(pos1,pos2)) {
                    diffs++;
                    if (printDiffs) cout << "Object connection positions don't match" << endl;
                }
                if (diffs && printDiffs) {
                    q_vec_print(pos1);
                    q_vec_print(pos2);
                }
            }
        } else { // if they are 1-obj springs test the world position of the 2nd endpoint

            sp1->getEnd2WorldPosition(pos1);
            sp2->getEnd2WorldPosition(pos2);
            if (!q_vec_equals(pos1,pos2)) {
                diffs++;
                if (printDiffs) cout << "World connection positions don't match" << endl;
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
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
    proj1->setProjectDir("test/test1");
    proj2->setProjectDir("test/test1");

    SketchModel *m1 = proj1->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = proj1->addObject(m1,pos1,orient1);

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    const SketchObject *o2 = proj2->getWorldManager()->getObjectIterator().next();
    compareObjects(o1,o2,retVal);

    if (retVal == 0) {
        cout << "Passed test 1" << endl;
    }

    root->Delete();
    return retVal;
}


int testSave2() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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
    proj1->addCamera(pos1,orient1);

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);
    compareCameras(proj1.data(), proj2.data(), retVal);

    if (retVal == 0) {
        cout << "Passed test 2" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave3() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);
    compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);

    if (retVal == 0) {
        cout << "Passed test 3" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave4() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);
    compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);
    compareSprings(proj1->getWorldManager()->getSpringsIterator().next(),
                   proj2->getWorldManager()->getSpringsIterator().next(),retVal,true);

    if (retVal == 0) {
        cout << "Passed test 4" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave6() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    SketchObject *o4 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o4->setPosAndOrient(pos1,orient1);
    SketchObject *o5 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o5->setPosAndOrient(pos1,orient1);
    ObjectGroup *grp = new ObjectGroup();
    grp->addObject(o3);
    grp->addObject(o4);
    grp->addObject(o5);
    pos1[Q_Z] = -40;
    pos1[Q_Y] = 300 / Q_PI;
    q_make(orient1,0,1,0,Q_PI/3);
    grp->setPosAndOrient(pos1,orient1);
    proj1->addObject(grp);
    proj1->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    proj1->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);
    compareReplications(proj1->getReplicas()->at(0),proj2->getReplicas()->at(0),retVal);
    compareSprings(proj1->getWorldManager()->getSpringsIterator().next(),
                   proj2->getWorldManager()->getSpringsIterator().next(),retVal,true);

    if (retVal == 0) {
        cout << "Passed test 6" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave9() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    vtkSmartPointer<vtkRenderer> r = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> project(new SketchProject(r,a,b));
    project->setProjectDir("test/test2");

    QDir dir = project->getProjectDir();
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);

    vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());

    if (ProjectToXML::xmlToProject(project.data(),root) == ProjectToXML::XML_TO_DATA_SUCCESS) {
        cout << "Passed test 9" << endl;
    } else {
        cout << "Failed to load" << endl;
        root->Delete();
        return 1;
    }
    root->Delete();
    return 0;
}


int testSave5() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    vtkSmartPointer<vtkRenderer> r = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> project(new SketchProject(r,a,b)), project2(new SketchProject(r,a,b));
    project->setProjectDir("test/test1");
    project2->setProjectDir("test/test1");

    SketchModel *m1 = project->addModelFromFile("models/1m1j.obj",3*sqrt(12.0),4*sqrt(13.0),1*sqrt(14.0));
    q_vec_type pos1 = {3.14,1.59,2.65}; // pi
    q_vec_scale(pos1,sqrt(Q_PI),pos1);
    q_type orient1;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    SketchObject *o1 = project->addObject(m1,pos1,orient1);
    pos1[Q_X] += 2 * Q_PI;
    q_mult(orient1,orient1,orient1);
    SketchObject *o2 = project->addObject(m1,pos1,orient1);
    o2->addKeyframeForCurrentLocation(0.0);
    project->addReplication(o1,o2,12);
    q_vec_type p1, p2;
    q_vec_set(p1,1.73,2.05,0.80); // sqrt(3)
    q_vec_scale(p1,sqrt(6.0),p1);
    q_vec_set(p2,2.23,6.06,7.97); // sqrt(5)
    q_vec_scale(p2,sqrt(7.0),p2);
    project->addSpring(o1,o2,1.41*sqrt(8.0),4.21*sqrt(10.0),3.56*sqrt(11.0),p1,p2); // sqrt(2)

    QDir dir = project->getProjectDir();
    QString file = dir.absoluteFilePath(PROJECT_XML_FILENAME);

    vtkXMLDataElement *b4 = ProjectToXML::projectToXML(project.data());
    vtkIndent indent(0);
    vtkXMLUtilities::WriteElementToFile(b4,file.toStdString().c_str(),&indent);

    b4->Delete();

    vtkXMLDataElement *root = vtkXMLUtilities::ReadElementFromFile(file.toStdString().c_str());

    if (ProjectToXML::xmlToProject(project2.data(),root) == ProjectToXML::XML_TO_DATA_SUCCESS) {
        cout << "Passed test 5" << endl;
    } else {
        cout << "Failed to load" << endl;
        root->Delete();
        return 1;
    }
    root->Delete();
    return 0;
}

int testSave7() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    proj1->addObject(o3);
    SketchObject *o4 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o4->setPosAndOrient(pos1,orient1);
    proj1->addObject(o4);
    SketchObject *o5 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o5->setPosAndOrient(pos1,orient1);
    proj1->addObject(o5);

    QWeakPointer<TransformEquals> eq = proj1->addTransformEquals(o1,o2);
    QSharedPointer<TransformEquals> sEq(eq);
    if (sEq) {
        sEq->addPair(o3,o4);
    }


    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);
    compareTransformOps(proj1->getTransformOps()->first(),
                        proj2->getTransformOps()->first(),retVal,true);

    if (retVal == 0) {
        cout << "Passed test 7" << endl;
    }

    root->Delete();
    return retVal;
}

int testSave8() {
    bool a[NUM_HYDRA_BUTTONS];
    double b[NUM_HYDRA_ANALOGS];
    int retVal = 0;
    vtkSmartPointer<vtkRenderer> r1 = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderer> r2 = vtkSmartPointer<vtkRenderer>::New();
    QScopedPointer<SketchProject> proj1(new SketchProject(r1,a,b));
    QScopedPointer<SketchProject> proj2(new SketchProject(r2,a,b));
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
    SketchObject *o3 = new ModelInstance(m1);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    proj1->addObject(o3);
    o3->addKeyframeForCurrentLocation(0.4);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    o3->addKeyframeForCurrentLocation(4.332);
    pos1[Q_Z] += 4 * Q_PI;
    q_mult(orient1,orient1,orient1);
    o3->setPosAndOrient(pos1,orient1);
    o3->addKeyframeForCurrentLocation(10.5);
    pos1[Q_Z] = 3.14;
    q_from_axis_angle(orient1,2.71,8.28,1.82,85); // e
    o3->setPosAndOrient(pos1,orient1);

    vtkXMLDataElement *root = ProjectToXML::projectToXML(proj1.data());

//    vtkIndent indent(0);
//    vtkXMLUtilities::FlattenElement(root,cout,&indent);

    ProjectToXML::xmlToProject(proj2.data(),root);

    compareNumbers(proj1.data(),proj2.data(),retVal);
    compareWorldObjects(proj1.data(),proj2.data(),retVal);

    if (retVal == 0) {
        cout << "Passed test 8" << endl;
    }

    root->Delete();
    return retVal;
}

int main() {
    return testSave1() + testSave2() + testSave3() + testSave4() + testSave5() +
            testSave6() + testSave7() + testSave8() + testSave9();
}
