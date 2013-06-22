#include <iostream>

#include <QScopedPointer>
#include <QTime>

#include <vtkCubeSource.h>

#include <sketchtests.h>
#include <sketchmodel.h>
#include <transformequals.h>

inline SketchModel *getModel() {
    vtkSmartPointer<vtkCubeSource> cube =
            vtkSmartPointer<vtkCubeSource>::New();
    cube->SetBounds(-1,1,-1,1,-1,1);
    cube->Update();
    QString fileName = ModelUtilities::createFileFromVTKSource(
                cube,"transform_equals_test_cube_model");
    SketchModel *model = new SketchModel(1,1);
    model->addConformation(fileName,fileName);
    return model;
}

inline void initializeVectorsAndQuats(q_vec_type nv, q_vec_type p1, q_vec_type p2, q_vec_type p3, q_vec_type p4,
                                      q_vec_type p5, q_vec_type p6, q_type idq, q_type rX, q_type rY, q_type rZ,
                                      q_type rrX, q_type rrY, q_type rrZ) {
    q_vec_set(nv,0,0,0);
    q_vec_set(p1,4,0,0);
    q_vec_set(p2,0,5,0);
    q_vec_set(p3,0,0,6);
    q_vec_set(p4,-2,0,0);
    q_vec_set(p5,0,-3,0);
    q_vec_set(p6,0,0,-7);
    q_make(idq,1,0,0,0);
    q_make(rX,1,0,0,Q_PI/2);
    q_make(rY,0,1,0,Q_PI/2);
    q_make(rZ,0,0,1,Q_PI/2);
    q_make(rrX,1,0,0,Q_PI);
    q_make(rrY,0,1,0,Q_PI);
    q_make(rrZ,0,0,1,Q_PI);
}

class IdGen : public GroupIdGenerator {
private:
    int num;
public:
    IdGen() { num = 0;}
    virtual ~IdGen() {}
    virtual int getNextGroupId() { return ++num;}
};

// this tests the motions of the first pair of objects to see if they are correct.
// no additional pairs are tested
int test1() {
    int errors = 0;
    QScopedPointer<SketchModel> m(getModel());
    q_vec_type nv, p1, p2, p3, p4, p5, p6, tv1, tv2, tv3;
    q_type idq, rX, rY, rZ, rrX, rrY, rrZ, t1, t2, t3;
    initializeVectorsAndQuats(nv,p1,p2,p3,p4,p5,p6,idq,rX,rY,rZ,rrX,rrY,rrZ);
    QScopedPointer<SketchObject> o1(new ModelInstance(m.data())), o2(new ModelInstance(m.data()));
    o1->setPosition(p1);
    o2->setPosition(p2);
    QScopedPointer<GroupIdGenerator> g(new IdGen());
    QScopedPointer<TransformEquals> eq(new TransformEquals(o1.data(),o2.data(),g.data()));
    // move second one
    // fake adding force so mode gets set right
    o2->addForce(nv,nv);
    o2->setPosition(p3);
    o2->getPosition(tv1);
    o1->getPosition(tv2);
    if (!q_vec_equals(p3,tv1)) {
        errors++;
        std::cout << "Position not set right" << std::endl;
    }
    if (!q_vec_equals(p1,tv2)) {
        errors++;
        std::cout << "Setting position of 2 affected 1" << std::endl;
    }
    q_vec_subtract(tv3,p3,p1);
    // move first one
    // fake adding force again
    o1->addForce(nv,nv);
    o1->setPosition(p4);
    o1->getPosition(tv1);
    o2->getPosition(tv2);
    q_vec_add(tv3,tv3,p4);
    if (!q_vec_equals(p4,tv1)) {
        errors++;
        std::cout << "Position not set right" << std::endl;
    }
    if (!q_vec_equals(tv2,tv3)) {
        errors++;
        std::cout << "Setting position of 1 affected 2 wrong" << std::endl;
    }
    // rotate the second one
    // fake adding force
    o2->addForce(nv,nv);
    o2->setOrientation(rX);
    o1->getPosition(tv1);
    o2->getPosition(tv2);
    o1->getOrientation(t1);
    o2->getOrientation(t2);
    if (!q_vec_equals(p4,tv1)) {
        errors++;
        std::cout << "Position of 1 changed when 2 rotated" << std::endl;
    }
    if (!q_vec_equals(tv2,tv3)) {
        errors++;
        std::cout << "Position of 2 changed when 2 rotated" << std::endl;
    }
    if (!q_equals(idq,t1)) {
        errors++;
        std::cout << "Orientation of 1 changed when 2 rotated" << std::endl;
    }
    if (!q_equals(rX,t2)) {
        errors++;
        std::cout << "Orientation of 2 not set right" << std::endl;
    }
    // rotate the first one
    // calculate where 2 should be:
    q_vec_type tmp;
    // calculating new pos/orient of #2... complex
    o2->getPosition(tmp);
    q_vec_invert(tv3,p4);
    // 1 has no original tranform -- skipping
    q_xform(tv3,rX,tv3);
    q_vec_add(tv3,tv3,tmp);
    q_xform(tv3,rY,tv3);
    q_vec_add(tv3,tv3,p4);
    q_mult(t3,rY,rX);
    // fake adding force
    o1->addForce(nv,nv);
    o1->setOrientation(rY);
    o1->getPosition(tv1);
    o2->getPosition(tv2);
    o1->getOrientation(t1);
    o2->getOrientation(t2);
    if (!q_vec_equals(p4,tv1)) {
        errors++;
        std::cout << "Position of 1 changed when 1 rotated" << std::endl;
    }
    if (!q_vec_equals(tv2,tv3)) {
        errors++;
        std::cout << "Position of 2 wrong after 1 rotated" << std::endl;
    }
    if (!q_equals(rY,t1)) {
        errors++;
        std::cout << "Orientation of 1 not set right" << std::endl;
    }
    if (!q_equals(t3,t2)) {
        errors++;
        std::cout << "Orientation of 2 wrong after 1 rotated" << std::endl;
    }
    if (errors == 0) {
        std::cout << "Passed test 1" << std::endl;
    }
    return errors;
}

int test2() {
    int errors = 0;
    QScopedPointer<SketchModel> m(getModel());
    q_vec_type nv, p1, p2, p3, p4, p5, p6, tv1, tv2, tv3;
    q_type idq, rX, rY, rZ, rrX, rrY, rrZ, t1, t2, t3;
    initializeVectorsAndQuats(nv,p1,p2,p3,p4,p5,p6,idq,rX,rY,rZ,rrX,rrY,rrZ);
    QScopedPointer<SketchObject> o1(new ModelInstance(m.data())), o2(new ModelInstance(m.data()));
    QScopedPointer<SketchObject> o3(new ModelInstance(m.data())), o4(new ModelInstance(m.data()));
    o1->setPosition(p1);
    o2->setPosition(p2);
    o3->setPosition(p3);
    o4->setPosition(p4);
    QScopedPointer<GroupIdGenerator> g(new IdGen());
    QScopedPointer<TransformEquals> eq(new TransformEquals(o1.data(),o2.data(),g.data()));
    // test positioning of second pair
    eq->addPair(o3.data(),o4.data());
    o4->getPosition(tv2);
    q_vec_subtract(tv1,p2,p1);
    q_vec_subtract(tv3,tv2,p3);
    if (!q_vec_equals(tv1,tv3)) {
        errors++;
        std::cout << "Object 4 in wrong position initially." << std::endl;
    }
    o3->setOrientation(rZ);
    o4->getPosition(tv2);
    q_vec_subtract(tv3,tv2,p3);
    q_invert(t1,rZ);
    q_xform(tv3,t1,tv3);
    if (!q_vec_equals(tv1,tv3)) {
        errors++;
        std::cout << "Object 4 moved wrong when 3 rotated." << std::endl;
    }
    o4->getOrientation(t2);
    if (!q_vec_equals(t2,rZ)) {
        errors++;
        std::cout << "Object 4 rotated wrong when 3 rotated." << std::endl;
    }
    o3->setOrientation(idq); // undo this so as not to confuse things utterly
    o2->setPosition(p6);
    q_vec_subtract(tv1,p6,p1);
    o4->getPosition(tv2);
    q_vec_subtract(tv3,tv2,p3);
    if (!q_vec_equals(tv3,tv1)) {
        errors++;
        std::cout << "Object 4 moved wrong when 2 moved." << std::endl;
    }
    o1->getPosition(tv1);
    if (!q_vec_equals(tv1,p1)) {
        errors++;
        std::cout << "Object 1 moved when object 2 updated." << std::endl;
    }
    o3->getPosition(tv3);
    if (!q_vec_equals(tv3,p3)) {
        errors++;
        std::cout << "Object 3 moved when object 2 updated." << std::endl;
    }
    o2->setOrientation(rrX);
    o4->getOrientation(t2);
    if (!q_equals(t2,rrX)) {
        errors++;
        std::cout << "Object 4 rotated wrong when object 2 rotated." << std::endl;
    }
    o1->addForce(nv,nv);
    o2->setOrientation(rY);
    o1->setOrientation(rY);
    o2->getOrientation(t3);
    if (!q_equals(t3,rrY)) {
        errors++;
        std::cout << "Double rotation wrong." << std::endl;
    }
    if (errors == 0) {
        std::cout << "Passed test 2" << std::endl;
    }
    return errors;
}

int test3() {
    int errors = 0;
    QScopedPointer<SketchModel> m(getModel());
    q_vec_type nv, p1, p2, p3, p4, p5, p6, tv1;
    q_type idq, rX, rY, rZ, rrX, rrY, rrZ;
    initializeVectorsAndQuats(nv,p1,p2,p3,p4,p5,p6,idq,rX,rY,rZ,rrX,rrY,rrZ);
    QScopedPointer<SketchObject> o1(new ModelInstance(m.data())), o2(new ModelInstance(m.data()));
    QScopedPointer<SketchObject> o3(new ModelInstance(m.data())), o4(new ModelInstance(m.data()));
    o1->setPosition(p1);
    o2->setPosition(p2);
    o3->setPosition(p3);
    o4->setPosition(p4);
    QScopedPointer<GroupIdGenerator> g(new IdGen());
    QScopedPointer<TransformEquals> eq(new TransformEquals(o1.data(),o2.data(),g.data()));
    eq->addPair(o3.data(),o4.data());

    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());

    int order[4];
    for (int i = 0; i < 4; i++) {
        order[i] = i;
    }
    for (int i = 0; i < 400; i++) {
        for (int j = 3; j >= 0; j--) {
            int r = qrand() % (j+1);
            int tmp = order[j];
            order[j] = order[r];
            order[r] = tmp;
        }
        for (int j = 0; j < 3; j++) {
            SketchObject *obj = NULL;
            switch(order[j]) {
            case 0:
                obj = o1.data();
                break;
            case 1:
                obj = o2.data();
                break;
            case 2:
                obj = o3.data();
                break;
            case 3:
                obj = o4.data();
                break;
            }
            if (obj != NULL) {
                obj->addForce(p1,p2);
                obj->getPosition(tv1);
            }
        }
    }
    std::cout << "Passed test 3" << std::endl;
    return errors;
}

int test4() {
    int errors = 0;
    QScopedPointer<SketchModel> m(getModel());
    q_vec_type nv, p1, p2, p3, p4, p5, p6, tv1, tv2, tv3;
    q_type idq, rX, rY, rZ, rrX, rrY, rrZ, tq1;
    initializeVectorsAndQuats(nv,p1,p2,p3,p4,p5,p6,idq,rX,rY,rZ,rrX,rrY,rrZ);
    QScopedPointer<SketchObject> o1(new ModelInstance(m.data())), o2(new ModelInstance(m.data()));
    QScopedPointer<SketchObject> o3(new ModelInstance(m.data())), o4(new ModelInstance(m.data()));
    o1->setPosition(p1);
    o2->setPosition(p2);
    o3->setPosition(p3);
    o4->setPosition(p4);
    QScopedPointer<GroupIdGenerator> g(new IdGen());
    QScopedPointer<TransformEquals> eq(new TransformEquals(o1.data(),o2.data(),g.data()));
    eq->addPair(o3.data(),o4.data());

    o3->setOrientation(rZ);

    q_vec_set(tv1,0,0,1);
    q_vec_add(tv1,tv1,p4);
    o4->addForce(tv1,p5);

    o4->getForce(tv2);
    if (!q_vec_equals(tv2,nv)) {
        errors++;
        q_vec_print(tv2);
        std::cout << "Force on 4 not cleared" << std::endl;
    }
    o4->getTorque(tv2);
    if (!q_vec_equals(tv2,nv)) {
        errors++;
        q_vec_print(tv2);
        std::cout << "Torque on 4 not cleared" << std::endl;
    }

    o2->getForce(tv2);
    q_invert(tq1,rZ);
    q_xform(tv3,tq1,p5);
    if (!q_vec_equals(tv3,tv2)) {
        errors++;
        q_vec_print(tv2);
        std::cout << "Force on 2 incorrect" << std::endl;
    }

    o2->getTorque(tv2);
    q_vec_set(tv1,0,0,1);
    o4->getWorldVectorInModelSpace(p5,tv3);
    q_vec_cross_product(tv3,tv1,tv3);
    if (!q_vec_equals(tv3,tv2)) {
        errors++;
        q_vec_print(tv2);
        std::cout << "Torque on 2 incorrect" << std::endl;
    }

    std::cout << "Passed test 4" << std::endl;
    return errors;
}

int main() {
    return test1() + test2() + test3() + test4();
}
