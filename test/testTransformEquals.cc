#include <transformequals.h>
#include <sketchtests.h>
#include <vtkCubeSource.h>

inline SketchModel *getModel() {
    vtkSmartPointer<vtkCubeSource> cube = vtkSmartPointer<vtkCubeSource>::New();
    cube->SetBounds(-1,1,-1,1,-1,1);
    cube->Update();
    vtkSmartPointer<vtkTransformPolyDataFilter> data =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    data->SetInputConnection(cube->GetOutputPort());
    vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
    t->Identity();
    t->Update();
    data->SetTransform(t);
    data->Update();
    return new SketchModel("",data,1,1);
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
    SketchModel *m = getModel();
    q_vec_type nv, p1, p2, p3, p4, p5, p6, tv1, tv2, tv3;
    q_type idq, rX, rY, rZ, rrX, rrY, rrZ, t1, t2, t3;
    initializeVectorsAndQuats(nv,p1,p2,p3,p4,p5,p6,idq,rX,rY,rZ,rrX,rrY,rrZ);
    SketchObject *o1 = new ModelInstance(m), *o2 = new ModelInstance(m);
    o1->setPosition(p1);
    o2->setPosition(p2);
    GroupIdGenerator *g = new IdGen();
    TransformEquals *eq = new TransformEquals(o1,o2,g);
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
        q_vec_print(tv2);
        q_vec_print(tv3);
    }
    if (!q_equals(rY,t1)) {
        errors++;
        std::cout << "Orientation of 1 not set right" << std::endl;
    }
    if (!q_equals(t3,t2)) {
        errors++;
        std::cout << "Orientation of 2 wrong after 1 rotated" << std::endl;
    }
    delete eq;
    delete g;
    delete o1;
    delete o2;
    delete m;
    return errors;
}

int main() {
    return test1();
}
