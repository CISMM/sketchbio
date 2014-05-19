#include "measuringtape.h"

MeasuringTape::MeasuringTape(SketchObject* o1, SketchObject* o2, const q_vec_type o1Pos,
              const q_vec_type o2Pos, bool display)
	:
	Connector(o1, o2, o1Pos, o2Pos, TAPE_ALPHA_VALUE,
                TAPE_DISPLAY_RADIUS, display),
				lengthActor(vtkSmartPointer< vtkTextActor3D >::New())
{
	updateLengthDisplay();
}

MeasuringTape::~MeasuringTape()
{
}

void MeasuringTape::getMidpoint(q_vec_type out) {
	q_vec_type pos1, pos2;
	getEnd1WorldPosition(pos1);
    getEnd2WorldPosition(pos2);
	out[0] = (pos1[0] + pos2[0]) / 2;
	out[1] = (pos1[1] + pos2[1]) / 2;
	out[2] = (pos1[2] + pos2[2]) / 2;
}

void MeasuringTape::updateLengthDisplay() {
	QString lengthstr = QString::number(getLength());
	lengthActor->SetInput(lengthstr.toStdString().c_str());
	q_vec_type midpt;
	getMidpoint(midpt);
	lengthActor->SetPosition(midpt);
}

void MeasuringTape::updateLine() {
	Connector::updateLine();
	QString lengthstr = QString::number(getLength());
	lengthActor->SetInput(lengthstr.toStdString().c_str());
	q_vec_type midpt;
	getMidpoint(midpt);
	lengthActor->SetPosition(midpt);
}