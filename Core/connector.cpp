#include "connector.h"

#include <vtkSmartPointer.h>
#include <vtkLineSource.h>
#include <vtkTubeFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <vtkPolyData.h>
#include <vtkFieldData.h>
#include <vtkPointData.h>

#include "sketchobject.h"
#include "sketchmodel.h"

/*
 * This class represents the visible state of the connector.  This default implementation
 * has no visible state
 */
class Connector::VisiblityData {
public:
    virtual ~VisiblityData() {}
    virtual vtkLineSource* getLine() const { return NULL; }
    virtual vtkActor* getActor() const { return NULL; }
    virtual void updateColorMap(ColorMapType::Type cmap) {}
    virtual void updateLine(q_vec_type p1, q_vec_type p2) {}
};

/*
 * This subclass of visible state object represents the visible state of the connector
 * as a (possibly translucent) tube.
 */
class Connector::LineVisibilityData : public Connector::VisiblityData
{
public:
    LineVisibilityData(q_vec_type p1, q_vec_type p2, double radius,
             ColorMapType::Type cmap, double alpha);
    virtual ~LineVisibilityData() {}
    virtual vtkLineSource* getLine() const { return line; }
    virtual vtkActor* getActor() const { return actor; }
    virtual void updateColorMap(ColorMapType::Type cmap);
    virtual void updateLine(q_vec_type p1, q_vec_type p2);
private:
    vtkSmartPointer< vtkLineSource > line;
    vtkSmartPointer<vtkActor> actor;
};

Connector::LineVisibilityData::LineVisibilityData(q_vec_type p1, q_vec_type p2, double radius,
                   ColorMapType::Type cmap, double alpha)
    :
      line(vtkSmartPointer< vtkLineSource >::New()),
      actor(vtkSmartPointer< vtkActor >::New())
{
    line->SetPoint1(p1);
    line->SetPoint2(p2);
    line->Update();
    vtkSmartPointer< vtkTubeFilter > tube =
            vtkSmartPointer< vtkTubeFilter >::New();
    tube->SetInputConnection(line->GetOutputPort());
    tube->SetRadius(radius);
    tube->Update();
    vtkSmartPointer< vtkPolyDataMapper > mapper =
            vtkSmartPointer< vtkPolyDataMapper >::New();
    mapper->SetInputConnection(tube->GetOutputPort());
    mapper->Update();
    updateColorMap(cmap);
    actor->SetMapper(mapper);
    if (alpha != 0)
    {
        actor->GetProperty()->SetOpacity(alpha);
    }
}

void Connector::LineVisibilityData::updateColorMap(ColorMapType::Type cmap)
{
    double range[2] = { 0.0, 1.0};
    ColorMapType::ColorMap map(cmap,"modelNum");
    vtkSmartPointer< vtkColorTransferFunction > colorFunc =
        vtkSmartPointer< vtkColorTransferFunction >::Take(
            map.getColorMap(range[0],range[1])
        );
    double rgb[3];
    colorFunc->GetColor(range[1],rgb);
    actor->GetProperty()->SetColor(rgb);
}

void Connector::LineVisibilityData::updateLine(q_vec_type p1, q_vec_type p2)
{
    line->SetPoint1(p1);
    line->SetPoint2(p2);
}

//#########################################################################
//#########################################################################
//#########################################################################
// Implementation of Connector itself

Connector::Connector()
  : d(NULL)
{
}

Connector::Connector(SketchObject *o1, SketchObject *o2,
                     const q_vec_type o1Pos, const q_vec_type o2Pos,
                     double a, double rad, bool display)
    :
    object1(o1),
    object2(o2),
    alpha(a),
    radius(rad),
    d(NULL),
    colorMap(ColorMapType::SOLID_COLOR_GRAY)
{
    q_vec_copy(object1ConnectionPosition,o1Pos);
    q_vec_copy(object2ConnectionPosition,o2Pos);

    if (display)
    {
        q_vec_type p1, p2;
        // create the line for the connector
        getEnd1WorldPosition(p1);
        getEnd2WorldPosition(p2);
        d.reset(new LineVisibilityData(p1,p2,radius,colorMap,alpha));
    }
    else
    {
        d.reset(new VisiblityData);
    }

}

Connector::~Connector()
{
}

void Connector::initConnector(SketchObject *o1, SketchObject *o2, const q_vec_type o1Pos, const q_vec_type o2Pos, double a, double rad, bool display) {
  object1 = o1;
  object2 = o2;
  q_vec_copy(object1ConnectionPosition,o1Pos);
  q_vec_copy(object2ConnectionPosition,o2Pos);
  alpha = a;
  radius = rad;
  colorMap = ColorMapType::SOLID_COLOR_GRAY;
  if (display) {
    q_vec_type p1, p2;
    // create the line for the connector
    getEnd1WorldPosition(p1);
    getEnd2WorldPosition(p2);
    d.reset(new LineVisibilityData(p1,p2,radius,colorMap,alpha));
  } else {
    d.reset(new VisiblityData);
  }
}

bool Connector::isInitialized() {
  return !d.isNull();
}

void Connector::setObject1(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd1WorldPosition(wPos);
    object1 = obj;
    setEnd1WorldPosition(wPos);
}

void Connector::setObject2(SketchObject *obj)
{
    q_vec_type wPos;
    getEnd2WorldPosition(wPos);
    object2 = obj;
    setEnd2WorldPosition(wPos);
}

void Connector::getEnd1WorldPosition(q_vec_type out) const {
    if (object1 != NULL)
        object1->getModelSpacePointInWorldCoordinates(object1ConnectionPosition,out);
    else
        q_vec_copy(out,object1ConnectionPosition);
}

void Connector::setEnd1WorldPosition(const q_vec_type newPos) {
    if (object1 != NULL)
        object1->getWorldSpacePointInModelCoordinates(newPos,object1ConnectionPosition);
    else
        q_vec_copy(object1ConnectionPosition,newPos);
}

void Connector::getEnd2WorldPosition(q_vec_type out) const {
    if (object2 != NULL)
        object2->getModelSpacePointInWorldCoordinates(object2ConnectionPosition,out);
    else
        q_vec_copy(out,object2ConnectionPosition);
}

void Connector::setEnd2WorldPosition(const q_vec_type newPos) {
    if (object2 != NULL)
        object2->getWorldSpacePointInModelCoordinates(newPos,object2ConnectionPosition);
    else
        q_vec_copy(object2ConnectionPosition,newPos);
}

double Connector::getLength() {
    q_vec_type point1, point2;
    getEnd1WorldPosition(point1);
    getEnd2WorldPosition(point2);
    return q_vec_distance(point1,point2);
}

static inline void snap(SketchObject* o, double value, q_vec_type dst)
{
	if (o != NULL) {
        vtkPolyDataAlgorithm *atomData = o->getModel()->getAtomData(o->getModelConformation());
        if (atomData != NULL) {
            vtkPolyData* model_data = atomData->GetOutput();
			vtkDataArray* chain_positions = model_data->GetPointData()->GetArray("chainPosition");
			vtkVariant position_val(value);
			vtkIdType terminus_id = chain_positions->LookupValue(position_val);
            if (terminus_id >= 0) { // if -1 returned, terminus does not exist
                model_data->GetPoint(terminus_id, dst);
            }
        }
    }
}

void Connector::snapToTerminus(bool on_object1, bool snap_to_n) {
	
	double chain_position = (snap_to_n) ? 0 : 1; //which terminus to snap to (0 for N, 1 for C)
  if (on_object1) {
    if(object1 == NULL || object1->numInstances() != 1) { return; }
		snap(object1,chain_position,object1ConnectionPosition);
	}
  else {
    if(object2 == NULL || object2->numInstances() != 1) { return; }
		snap(object2,chain_position,object2ConnectionPosition);
	}
}

void Connector::setColorMapType(ColorMapType::Type cmap) {
	colorMap = cmap;
	updateColorMap();
}

// These next four methods are deferred to the VisibilityData

vtkLineSource* Connector::getLine()
{
    return d.data()->getLine();
}

vtkActor* Connector::getActor()
{
    return d.data()->getActor();
}

void Connector::updateLine()
{
    // get the line endpoints
    q_vec_type p1, p2;
    getEnd1WorldPosition(p1);
    getEnd2WorldPosition(p2);
    d.data()->updateLine(p1,p2);
}

void Connector::updateColorMap()
{
    d.data()->updateColorMap(colorMap);
}
