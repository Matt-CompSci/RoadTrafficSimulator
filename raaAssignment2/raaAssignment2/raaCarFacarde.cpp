#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Material>
#include "raaTrafficSystem.h"
#include "raaCarFacarde.h"
#include <iostream>
#include "raaBoundCalculator.h"
#include "raaFacarde.h"
#include "raaCollisionTarget.h"
#include "TrafficLightFacarde.h"
#include "TrafficLightControl.h"

raaCarFacarde::raaCarFacarde(osg::Node* pWorldRoot, osg::Node* pPart, osg::AnimationPath* ap, double dSpeed): raaAnimatedFacarde(pPart, ap, dSpeed)
{
	raaTrafficSystem::addTarget(this); // adds the car to the traffic system (static class) which holds a reord of all the dynamic parts in the system
	this->dSpeed = dSpeed;

}

raaCarFacarde::~raaCarFacarde()
{
	raaTrafficSystem::removeTarget(this); // removes the car from the traffic system (static class) which holds a reord of all the dynamic parts in the system
}

void raaCarFacarde::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	osg::Vec3 worldCollisionPoint = getWorldCollisionPoint();
	osg::Vec3 worldDetectionPoint = getWorldDetectionPoint();
	//std::cout << "CAR: " << worldCollisionPoint[0] << ',' << worldCollisionPoint[1] << ',' << worldCollisionPoint[2] << std::endl;
	float closestTrafficLightDistance = 999999;
	osg::Vec3 closestTrafficLightPosition;
	TrafficLightFacarde* closestTrafficLight;
	int closestTrafficLightStatus = 0;

	float closestCarDistance = 999999;
	osg::Vec3 closestCarPosition;
	raaCarFacarde* closestCar;


	// Find the nearest traffic light and car
	for(raaFacarde* facarde : sm_lFacardes) {
		
		// Get name
		std::string name = facarde->pPart->getName();

		// Get position of facarde
		raaBoundCalculator* facardeBounds = new raaBoundCalculator(facarde->pPart);
		osg::Vec3 facardePos = facardeBounds->centre() * facarde->translation()->getWorldMatrices()[0];




		if(name.find("trafficLight") != std::string::npos) {
			// Calculate distance between car and facarde
			float dx = facardePos[0] - worldCollisionPoint[0];
			float dy = facardePos[1] - worldCollisionPoint[1];
			float distance = sqrt((dx * dx) + (dy * dy));

			TrafficLightFacarde* trafficLight = dynamic_cast<TrafficLightFacarde*>(facarde);
			int trafficLightStatus = trafficLight->m_iTrafficLightStatus;
			if(distance < closestTrafficLightDistance && (trafficLightStatus == 1 || trafficLightStatus == 2)) {
				closestTrafficLight = trafficLight;
				closestTrafficLightDistance = distance;
				closestTrafficLightPosition = facardePos;
				closestTrafficLightStatus = trafficLightStatus;
			}
		}

		else if(name.find("car") != std::string::npos) {
			// Calculate distance between car and facarde
			float dx = facardePos[0] - worldDetectionPoint[0];
			float dy = facardePos[1] - worldDetectionPoint[1];
			float distance = sqrt((dx * dx) + (dy * dy));

			raaCarFacarde* car = dynamic_cast<raaCarFacarde*>(facarde);

			//std::cout << name << std::endl;
			if(distance < closestCarDistance && name != pPart->getName()) {
				closestCar = car;
				closestCarDistance = distance;
				closestCarPosition = facardePos;
			}
		}
	}

	// If closestTrafficLight is not a null pointer
	if(closestTrafficLight != NULL) {
		//std::cout << closestTrafficLightDistance << std::endl;
		//std::cout << closestTrafficLight->root()->getName() << closestTrafficLightPosition[0] << ',' << closestTrafficLightPosition[1] << ',' << closestTrafficLightPosition[2] << std::endl;
		//std::cout << closestTrafficLightStatus << std::endl;
		if(closestTrafficLightDistance < 100 || (closestCarDistance < 150 && closestTrafficLightStatus != 0)) {
			setCurrentSpeed(0);
		}
		else {
			setCurrentSpeed(50);
		}
	} else {
		setCurrentSpeed(50);
	}

	raaAnimationPathCallback::operator()(node, nv);
}

osg::Vec3f raaCarFacarde::getWorldDetectionPoint()
{
	//osg::Vec3 worldPosition = osg::Vec3(pPart->getWorldMatrices()[0], pPart->getWorldMatrices()[1], pPart->getWorldMatrices()[2]);
	raaBoundCalculator* bounds = new raaBoundCalculator(pPart);
	//return bounds->centre() * pPart->getWorldMatrices()[0];
	return (bounds->centre() + osg::Vec3(100, 0, 0)) * translation()->getWorldMatrices()[0];
}

osg::Vec3f raaCarFacarde::getWorldCollisionPoint()
{
	raaBoundCalculator* bounds = new raaBoundCalculator(pPart);
	//return bounds->centre() * pPart->getWorldMatrices()[0];
	return (bounds->centre() - osg::Vec3(100, 0, 0)) * translation()->getWorldMatrices()[0];
}

