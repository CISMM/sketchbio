#include "controlFunctions.h"
#include "savedxmlundostate.h"

#include <sstream>

#include <QApplication>
#include <QClipboard>

#include <vtkXMLUtilities.h>
#include <vtkXMLDataElement.h>

#include <sketchioconstants.h>
#include <sketchobject.h>
#include <sketchproject.h>
#include <connector.h>
#include <keyframe.h>
#include <objectgroup.h>
#include <projecttoxml.h>
#include <springconnection.h>
#include <transformmanager.h>
#include <worldmanager.h>

static inline void addXMLUndoState(SketchProject *project)
{
	UndoState *last = project->getLastUndoState();
	SavedXMLUndoState *lastXMLState = dynamic_cast< SavedXMLUndoState * >(last);
	SavedXMLUndoState *newXMLState = NULL;
	if (lastXMLState != NULL)
	{
		QSharedPointer< std::string > lastState =
				lastXMLState->getAfterState().toStrongRef();
		newXMLState = new SavedXMLUndoState(*project,lastState);
		if (lastXMLState->getBeforeState().isNull())
		{
			project->popUndoState();
		}
	}
	else
	{
		QSharedPointer< std::string > lastStr(NULL);
		newXMLState = new SavedXMLUndoState(*project,lastStr);
	}
	project->addUndoState(newXMLState);
}

namespace ControlFunctions
{
	// ===== BEGIN ANIMATION FUNCTIONS =====

	void keyframeAll(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Release to keyframe all objects.");
		}
		else // button released
		{
			double time = project->getViewTime();
			QListIterator< SketchObject* > itr =
					project->getWorldManager()->getObjectIterator();
			while ( itr.hasNext() )
			{
				itr.next()->addKeyframeForCurrentLocation(time);
			}
			project->getWorldManager()->setAnimationTime(time);
			//emit newDirectionsString(" ");
			addXMLUndoState(project);
		}
		return;
	}

	void toggleKeyframeObject(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Move to an object and release to add or remove a keyframe\nfor the current location and time.");
		}
		else // button released
		{
			SketchObject* nearestObj = project->getNearest(hand);
			double nearestObjDist = project->getDistance(hand);

			SketchObject* nearestObjParent = nearestObj->getParent();

			if (nearestObjDist < DISTANCE_THRESHOLD && nearestObjParent == NULL)
			{
				// add/update/remove keyframe
				double time = project->getViewTime();
				bool newFrame = nearestObj->hasChangedSinceKeyframe(time);
				if (newFrame)
				{
					nearestObj->addKeyframeForCurrentLocation(time);
				}
				else
				{
					nearestObj->removeKeyframeForTime(time);
				}
				// make sure the object doesn't move just because we removed
				// the keyframe... principle of least astonishment.
				// using this instead of setAnimationTime since it doesn't update
				// positions
				project->getWorldManager()->setKeyframeOutlinesForTime(time);
				addXMLUndoState(project);
				//emit newDirectionsString(" ");
			}
			else if (nearestObjParent != NULL)
			{
				//emit newDirectionsString("Cannot keyframe objects in a group individually,\n"
				//						 "keyframe the group and it will save the state of all of them.");
			}
			else
			{
				//emit newDirectionsString(" ");
			}
		}
		return;
	}

	// !!! getXXXTrackerPosInWorldCoords() & getXXXTrackerOrientInWorldCoords() are not generalized !!!
	void addCamera(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Release the button to add a camera.");
		}
		else // button released
		{
			TransformManager *transforms = project->getTransformManager();
			q_vec_type pos;
			q_type orient;
			transforms->getRightTrackerPosInWorldCoords(pos);
			transforms->getRightTrackerOrientInWorldCoords(orient);
			project->addCamera(pos,orient);
			addXMLUndoState(project);
			//emit newDirectionsString(" ");
		}
		return;
	}

	void showAnimationPreview(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			if (project->isShowingAnimation())
			{
				//emit newDirectionsString("Release to stop the animation preview.");
			}
			else
			{
				//emit newDirectionsString("Release to preview the animation.");
			}
		}
		else // button released
		{
			if (project->isShowingAnimation())
			{
				project->stopAnimation();
				//emit newDirectionsString(" ");
			}
			else
			{
				// play sample animation
				project->startAnimation();
				//emit newDirectionsString(" ");
			}
		}
		return;
	}

	// ===== END ANIMATION FUNCTIONS =====

	// ===== BEGIN GROUP EDITING FUNCTIONS =====

	void toggleGroupMembership(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Select the group with the left hand\n"
			//						 "and the object to add/remove with the right hand");
		}
		else // button released
		{
			WorldManager *world = project->getWorldManager();

			SketchObject* leftNearestObj = project->getNearest(0);
			double leftNearestObjDist = project->getDistance(0);

			SketchObject* rightNearestObj = project->getNearest(1);
			double rightNearestObjDist = project->getDistance(1);

			if (leftNearestObjDist < DISTANCE_THRESHOLD && rightNearestObjDist < DISTANCE_THRESHOLD)
			{
				ObjectGroup *grp = dynamic_cast< ObjectGroup * >(leftNearestObj);
				if (leftNearestObj == rightNearestObj)
				{
					if (grp == NULL)
						return;
					SketchObject *rHand = project->getRightHandObject();
					SketchObject *obj = WorldManager::getClosestObject(
								*grp->getSubObjects(),rHand,rightNearestObjDist);
					if (rightNearestObjDist < DISTANCE_THRESHOLD)
					{
						grp->removeObject(obj);
						world->addObject(obj);
					}
				}
				else
				{
					if (grp == NULL ||
							(grp != NULL &&
							 dynamic_cast< ObjectGroup * >(rightNearestObj) != NULL ))
					{
						grp = new ObjectGroup();
						world->removeObject(leftNearestObj);
						grp->addObject(leftNearestObj);
						world->addObject(grp);
					}
					world->removeObject(rightNearestObj);
					grp->addObject(rightNearestObj);
				}
				addXMLUndoState(project);
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	void copyObject(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Select an object and release the button to copy");
		}
		else // button released
		{
			SketchObject* nearestObj = project->getNearest(hand);
			double nearestObjDist = project->getDistance(hand);

			if (nearestObjDist < DISTANCE_THRESHOLD)
			{
				vtkSmartPointer< vtkXMLDataElement > elem =
						vtkSmartPointer< vtkXMLDataElement >::Take(
							ProjectToXML::objectToClipboardXML(nearestObj)
						);
				std::stringstream ss;
				vtkXMLUtilities::FlattenElement(elem,ss);
				QClipboard *clipboard = QApplication::clipboard();
				clipboard->setText(ss.str().c_str());
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	// !!! getXXXTrackerPosInWorldCoords() is not generalized !!!
	void pasteObject(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Release the button to paste");
		}
		else // button released
		{
			std::stringstream ss;
			QClipboard *clipboard = QApplication::clipboard();
			ss.str(clipboard->text().toStdString());
			vtkSmartPointer< vtkXMLDataElement > elem =
					vtkSmartPointer< vtkXMLDataElement >::Take(
						vtkXMLUtilities::ReadElementFromStream(ss)
						);
			if (elem)
			{
				q_vec_type rpos;
				project->getTransformManager()->getRightTrackerPosInWorldCoords(rpos);
				if (ProjectToXML::objectFromClipboardXML(project,elem,rpos) ==
						ProjectToXML::XML_TO_DATA_FAILURE)
				{
					std::cout << "Read xml correctly, but reading object failed."
							  << std::endl;
				}
				else
				{
					addXMLUndoState(project);
				}
			}
			else
			{
				std::cout << "Failed to read object." << std::endl;
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	// ===== END GROUP EDITING FUNCTIONS =====

	// ===== BEGIN COLOR EDITING FUNCTIONS =====

	void changeObjectColor(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Move to the object and release to change\n"
            //                     " the color map of the object");
		}
		else // button released
		{
			SketchObject* nearestObj = project->getNearest(hand);
			double nearestObjDist = project->getDistance(hand);

			Connector* nearestSpring = project->getNearestSpring(hand);
			double nearestSpringDist = project->getSpringDistance(hand);

			if (nearestObjDist < DISTANCE_THRESHOLD && !nearestObj->getArrayToColorBy().isEmpty())
			{
				ColorMapType::Type cmap = nearestObj->getColorMapType();
				using namespace ColorMapType;
				switch (cmap)
				{
				case SOLID_COLOR_RED:
					cmap = SOLID_COLOR_GREEN;
					break;
				case SOLID_COLOR_GREEN:
					cmap = SOLID_COLOR_BLUE;
					break;
				case SOLID_COLOR_BLUE:
					cmap = SOLID_COLOR_YELLOW;
					break;
				case SOLID_COLOR_YELLOW:
					cmap = SOLID_COLOR_PURPLE;
					break;
				case SOLID_COLOR_PURPLE:
					cmap = SOLID_COLOR_CYAN;
					break;
				case SOLID_COLOR_CYAN:
					cmap = BLUE_TO_RED;
					break;
				case BLUE_TO_RED:
					cmap = SOLID_COLOR_RED;
					break;
				default:
					cmap = SOLID_COLOR_RED;
					break;
				}
				nearestObj->setColorMapType(cmap);
				addXMLUndoState(project);
			}
			else if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD) {
				ColorMapType::Type cmap = nearestSpring->getColorMapType();
				using namespace ColorMapType;
				switch (cmap)
				{
				case SOLID_COLOR_RED:
					cmap = SOLID_COLOR_GREEN;
					break;
				case SOLID_COLOR_GREEN:
					cmap = SOLID_COLOR_BLUE;
					break;
				case SOLID_COLOR_BLUE:
					cmap = SOLID_COLOR_YELLOW;
					break;
				case SOLID_COLOR_YELLOW:
					cmap = SOLID_COLOR_PURPLE;
					break;
				case SOLID_COLOR_PURPLE:
					cmap = SOLID_COLOR_CYAN;
					break;
				case SOLID_COLOR_CYAN:
					cmap = SOLID_COLOR_GRAY;
					break;
				case SOLID_COLOR_GRAY:
					cmap = SOLID_COLOR_RED;
					break;
				default:
					cmap = SOLID_COLOR_GRAY;
					break;
				}
				nearestSpring->setColorMapType(cmap);
				addXMLUndoState(project);
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	void changeObjectColorVariable(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Move to the object and release to change\n"
            //                " the variable the object is colored by.");
		}
		else // button released
		{
			SketchObject* nearestObj = project->getNearest(hand);
			double nearestObjDist = project->getDistance(hand);

			if (nearestObjDist < DISTANCE_THRESHOLD) {
				QString arr(nearestObj->getArrayToColorBy());
				if (arr == QString("modelNum"))
				{
					arr = QString("chainPosition");
				}
				else if (arr == QString("chainPosition"))
				{
					arr = QString("charge");
				}
				else if (arr == QString("charge"))
				{
					arr = QString("modelNum");
				}
				nearestObj->setArrayToColorBy(arr);
				addXMLUndoState(project);
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	void toggleObjectVisibility(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Move to an object and release to toggle "
            //                "object visibility.");
		}
		else // button released
		{
			SketchObject* nearestObj = project->getNearest(hand);
			double nearestObjDist = project->getDistance(hand);

			if (nearestObjDist < DISTANCE_THRESHOLD)
			{
				// toggle object visible
				// TODO - shawn, make easier to use so that only one call is needed
				SketchObject::setIsVisibleRecursive(nearestObj,!nearestObj->isVisible());
				project->getWorldManager()->changedVisibility(nearestObj);
				addXMLUndoState(project);
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	void toggleShowInvisibleObjects(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Release to toggle whether invisible objects are shown.");
		}
		else // button released
		{
			// toggle invisible objects shown
			WorldManager *world = project->getWorldManager();
			if (world->isShowingInvisible())
				world->hideInvisibleObjects();
			else
				world->showInvisibleObjects();
			//emit newDirectionsString(" ");
		}
		return;
	}

	// ===== END COLOR EDITING FUNCTIONS =====

	// ===== BEGIN SPRING EDITING FUNCTIONS =====

	// !! INCOMPLETE !! - Needs a "hand-oriented" state mechanism
	void grabSpringOrWorld(SketchProject *project, int hand, bool wasPressed)
	{
	/*	if (wasPressed)
		{
			Connector* nearestSpring = project->getNearestSpring(hand);
			double nearestSpringDist = project->getSpringDistance(hand);

			// NOTE: If the "hand-oriented" state mechanism takes (int hand) as an argument,
			//       then we won't need the outer IF-ELSE statement.
			if (hand == 1) // right hand
			{
				if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD)
					rightGrabbedSpring = true;
				else if (grabbedWorld == WORLD_NOT_GRABBED)
					grabbedWorld = RIGHT_GRABBED_WORLD;
			}
			else // left hand
			{
				if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD)
					leftGrabbedSpring = true;
				else if (grabbedWorld == WORLD_NOT_GRABBED)
					grabbedWorld = LEFT_GRABBED_WORLD;
			}
		}
		else // button released
		{
			Connector* nearestSpring = project->getNearestSpring(hand);
			double nearestSpringDist = project->getSpringDistance(hand);

			if (hand == 1) // right hand
			{
				if (grabbedWorld == RIGHT_GRABBED_WORLD)
				{
					grabbedWorld = WORLD_NOT_GRABBED;
				}
				else if (rightGrabbedSpring)
				{
					rightGrabbedSpring = false;
					project->setOutlineSpring(RIGHT_SIDE_OUTLINE,nearestSpring,rAtEnd1);
					addXMLUndoState(project);
				}
			}
			else // left hand
			{
				if (grabbedWorld == LEFT_GRABBED_WORLD)
				{
					grabbedWorld = WORLD_NOT_GRABBED;
				}
				else if (leftGrabbedSpring)
				{
					leftGrabbedSpring = false;
					project->setOutlineSpring(LEFT_SIDE_OUTLINE,nearestSpring,lAtEnd1);
					addXMLUndoState(project);
				}
			}
		}
	*/	return;
	}

	void deleteSpring(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Move to a spring and release to delete the spring.");
		}
		else // button released
		{
			Connector* thisHandNearestSpring = project->getNearestSpring(hand);
			double thisHandNearestSpringDist = project->getSpringDistance(hand);

			Connector* otherHandNearestSpring = project->getNearestSpring((hand + 1) % 2);

			if (thisHandNearestSpringDist < SPRING_DISTANCE_THRESHOLD)
			{
				project->getWorldManager()->removeSpring(thisHandNearestSpring);
				if (thisHandNearestSpring == otherHandNearestSpring)
				{
					otherHandNearestSpring = NULL;
				}
				thisHandNearestSpring = NULL;
			}
			//emit newDirectionsString(" ");
		}
		return;
	}

	// !! INCOMPLETE !!
	void snapSpringToTerminus(SketchProject *project, int hand, bool wasPressed)
	{
	/*	if (wasPressed)
		{
			Connector* nearestSpring = project->getNearestSpring(hand);
			double nearestSpringDist = project->getSpringDistance(hand);

			snapMode = true;
			//emit newDirectionsString("Move to a spring and choose which terminus to snap to.");
			if (nearestSpringDist < SPRING_DISTANCE_THRESHOLD) {
				double value =  analogStatus[ ANALOG_RIGHT(TRIGGER_ANALOG_IDX) ];
				bool snap_to_n = (value < 0.5) ? true : false;
				nearestSpring->snapToTerminus(rAtEnd1, snap_to_n);
			}
		}
		else // button released
		{
			snapMode = false;
			//emit newDirectionsString(" ");
		}
	*/	return;
	}

	// !! INCOMPLETE !!
	// ...also...
	// !!! getXXXTrackerPosInWorldCoords() is not generalized !!!
	void createSpring(SketchProject *project, int hand, bool wasPressed)
	{
	/*	if (wasPressed)
		{
			//emit newDirectionsString("Release at location of new spring.");
		}
		else // button released
		{
			q_vec_type pos1, pos2 = {0, 1, 0};
			project->getTransformManager()->getRightTrackerPosInWorldCoords(pos1);
			q_vec_add(pos2,pos1,pos2);
			double stiffness;
			stiffness = ( 1 - analogStatus[ANALOG_LEFT(TRIGGER_ANALOG_IDX)]);
			SpringConnection* spring = SpringConnection::makeSpring(
						NULL,
						NULL,
						pos1,
						pos2,
						true,
						stiffness,
						0,
						true);
			project->addConnector(spring);
			addXMLUndoState(project);
			//emit newDirectionsString(" ");
		}
	*/	return;
	}

	// !!! getXXXTrackerPosInWorldCoords() is not generalized !!!
	void createTransparentConnector(SketchProject *project, int hand, bool wasPressed)
	{
		if (wasPressed)
		{
			//emit newDirectionsString("Release to create a transparent connector at the current location.");
		}
		else // button released
		{
			q_vec_type pos1, pos2 = {0, 1, 0};
			project->getTransformManager()->getRightTrackerPosInWorldCoords(pos1);
			q_vec_add(pos2,pos1,pos2);
			Connector* conn = new Connector(NULL,NULL,pos1,pos2,0.3,10);
			project->addConnector(conn);
			addXMLUndoState(project);
			//emit newDirectionsString(" ");
		}
		return;
	}

	// ===== END SPRING EDITING FUNCTIONS =====

	// ===== BEGIN UTILITY FUNCTIONS =====

	void resetViewPoint(SketchProject *project, int hand, bool wasPressed)
	{
		if (!wasPressed) // button released
		{
			project->getTransformManager()->setRoomEyeOrientation(0, 0);
		}
	}

	// ===== END UTILITY FUNCTIONS =====
}