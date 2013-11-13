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

Connector::Connector(SketchObject *o1, SketchObject *o2,
                     const q_vec_type o1Pos, const q_vec_type o2Pos,
                     double a, double rad)
{
    object1 = o1;
    object2 = o2;
    q_vec_copy(object1ConnectionPosition,o1Pos);
    q_vec_copy(object2ConnectionPosition,o2Pos);
    alpha = a;
    radius = rad;
	colorMap = ColorMapType::SOLID_COLOR_GRAY;

	q_vec_type p1, p2;
    // create the line for the connector
    line = vtkSmartPointer< vtkLineSource >::New();
    getEnd1WorldPosition(p1);
    getEnd2WorldPosition(p2);
    line->SetPoint1(p1);
    line->SetPoint2(p2);
    line->Update();
    vtkSmartPointer< vtkTubeFilter > tube =
            vtkSmartPointer< vtkTubeFilter >::New();
    tube->SetInputConnection(line->GetOutputPort());
    tube->SetRadius(getRadius());
    tube->Update();
    mapper = vtkSmartPointer< vtkPolyDataMapper >::New();
    mapper->SetInputConnection(tube->GetOutputPort());
    mapper->Update();
    actor = vtkSmartPointer< vtkActor >::New();
	updateColorMap();
    actor->SetMapper(mapper);
    if (getAlpha() != 0)
    {
        actor->GetProperty()->SetOpacity(getAlpha());
    }
}

Connector::~Connector()
{
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

void Connector::updateColorMap()
{
    double range[2] = { 0.0, 1.0};
	vtkSmartPointer< vtkColorTransferFunction > colorFunc =
        vtkSmartPointer< vtkColorTransferFunction >::Take(
            ColorMapType::getColorMap(getColorMapType(),range[0],range[1])
        );
    double rgb[3];
    colorFunc->GetColor(range[1],rgb);
    actor->GetProperty()->SetColor(rgb);
}

static inline void snap(SketchObject* o, double value, q_vec_type dst)
{
	if (o != NULL) {
			vtkPolyData* model_data = o->getModel()->getAtomData(o->getModelConformation())->GetOutput();
			vtkDataArray* chain_positions = model_data->GetPointData()->GetArray("chainPosition");
			vtkVariant position_val(value);
			vtkIdType terminus_id = chain_positions->LookupValue(position_val);
			model_data->GetPoint(terminus_id, dst);
		}
}

void Connector::snapToTerminus(bool on_object1, bool snap_to_n) {
	
	double chain_position = (snap_to_n) ? 0 : 1; //which terminus to snap to (0 for N, 1 for C)
	if (on_object1) {
		snap(object1,chain_position,object1ConnectionPosition);
	}
	else {
		snap(object2,chain_position,object2ConnectionPosition);
	}
}

vtkLineSource* Connector::getLine() {
	return line;
}

vtkActor* Connector::getActor() {
	return actor;
}

void Connector::setColorMapType(ColorMapType::Type cmap) {
	colorMap = cmap;
	updateColorMap();
}