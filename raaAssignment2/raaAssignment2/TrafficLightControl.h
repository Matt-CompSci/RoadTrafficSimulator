#pragma once

#include "TrafficLightFacarde.h"
#include <list>
#include "raaNodeCallbackFacarde.h"


typedef std::list<TrafficLightFacarde*> trafficLightList;

class TrafficLightControl : public raaNodeCallbackFacarde
{
public:
	TrafficLightControl(osg::Node* pPart, osg::Vec3 vTrans, float fRot, float fScale);
	virtual ~TrafficLightControl();
	void operator() (osg::Node* node, osg::NodeVisitor* nv) override;
	void addTrafficLight(TrafficLightFacarde* pTrafficLight);
	void changeTrafficLight(TrafficLightFacarde* pTrafficLight);
	trafficLightList getTrafficLights();

protected:
	trafficLightList m_lTrafficLights;
	int timeCount;
};

