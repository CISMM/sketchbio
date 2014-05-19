#ifndef MEASURINGTAPE_H
#define MEASURNIGTAPE_H

#include <quat.h>
#include "connector.h"

#include <vtkTextActor3D.h>
#include <vtkSmartPointer.h>
class vtkTextActor3D;

// the default values of alpha and radius for a MeasuringTape
static const double TAPE_ALPHA_VALUE = 0.5;
static const double TAPE_DISPLAY_RADIUS = 2.0;

/*
 * This class is a type of connector that has a length and will be displayed as a line
 * along with its length and tick marks to act as a measuring tape, so it can be used to
 * measure distances and determine the scale of objects
*/

class MeasuringTape : public Connector
{
public:
	MeasuringTape(SketchObject* o1, SketchObject* o2, const q_vec_type o1Pos,
              const q_vec_type o2Pos, bool display = true);
    virtual ~MeasuringTape();

	vtkTextActor3D* getLengthActor() const { return lengthActor; }
	
	void getMidpoint(q_vec_type out);

	void updateLengthDisplay();
	void updateLine();
private:
	vtkSmartPointer< vtkTextActor3D > lengthActor;
};

#endif