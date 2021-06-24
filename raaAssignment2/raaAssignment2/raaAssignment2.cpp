#include <iostream>
#include <math.h>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/AnimationPath>
#include <osg/Matrix>
#include <osg/Material>
#include <osg/ShapeDrawable>
#include "raaInputController.h"
#include "raaAssetLibrary.h"
#include "raaFacarde.h"
#include "raaSwitchActivator.h"
#include "raaRoadTileFacarde.h"
#include "raaAnimationPointFinder.h"
#include "raaAnimatedFacarde.h"
#include "raaCarFacarde.h"
#include "raaTrafficSystem.h"
#include "TrafficLightControl.h"
#include "TrafficLightFacarde.h"
#include "raaBoundCalculator.h"



typedef std::vector<raaAnimationPointFinder>raaAnimationPointFinders;
osg::Group* g_pRoot = 0; // root of the sg
float g_fTileSize = 472.441f; // width/depth of the standard road tiles
std::string g_sDataPath = "../../Data/";

enum raaRoadTileType
{
	Normal,
	LitTJunction,
	LitXJunction,
};

void addRoadTile(std::string sAssetName, std::string sPartName, int xUnit, int yUnit, float fRot, osg::Group* pParent)
{

	raaFacarde* pFacarde = new raaRoadTileFacarde(raaAssetLibrary::getNamedAsset(sAssetName, sPartName), osg::Vec3(g_fTileSize * xUnit, g_fTileSize * yUnit, 0.0f), fRot);
	pParent->addChild(pFacarde->root());
}

int lights = 0;

void addLightsTile(std::string sAssetName, std::string sPartName, int xUnit, int yUnit, float fRot, osg::Group* pParent, osg::Group* trafficLights)
{
	// Create traffic control

	TrafficLightControl* pFacarde = new TrafficLightControl(raaAssetLibrary::getNamedAsset(sAssetName, sPartName), osg::Vec3(g_fTileSize * xUnit, g_fTileSize * yUnit, 0.0f), fRot, 1.0f);
	pParent->addChild(pFacarde->root());

	// Dynamically give traffic light names based on the amount in the group
	int length = trafficLights->getNumChildren();

	float angleInRadians = fRot * (M_PI / 180);
	float pavementRatio = 0.8f;

	if(sin(fRot) > cos(fRot) && sAssetName != "roadCurve") {
		angleInRadians += M_PI;
	}


	if(sAssetName == "roadCurve") {
		angleInRadians += M_PI / 4;
		pavementRatio = 0.3f;
	}

	// Create left light
	if(sAssetName != "roadCurve") {
		lights++;
		std::string name = "trafficLight" + std::to_string(lights);
		TrafficLightFacarde* leftLight = new TrafficLightFacarde(raaAssetLibrary::getClonedAsset("trafficLight", name), osg::Vec3(g_fTileSize * xUnit + sin(angleInRadians) * (g_fTileSize / 2) * pavementRatio, g_fTileSize * yUnit + cos(angleInRadians) * (g_fTileSize / 2) * pavementRatio, 0.0f), fRot - 90.0f, 0.08f);
		// Add lights to controller
		pFacarde->addTrafficLight(leftLight);
		

		// Add to trafficLights OSG::Group
		trafficLights->addChild(leftLight->root());
	}
	
	// Create right light
	lights++;
	std::string name = "trafficLight" + std::to_string(lights);
	TrafficLightFacarde* rightLight = new TrafficLightFacarde(raaAssetLibrary::getClonedAsset("trafficLight", name), osg::Vec3(g_fTileSize * xUnit + sin(angleInRadians + M_1_PI) * (g_fTileSize / 2) * pavementRatio, g_fTileSize * yUnit + cos(angleInRadians + M_1_PI) * (g_fTileSize / 2) * pavementRatio, 0.0f), fRot + 90.0f, 0.08f);
	pFacarde->addTrafficLight(rightLight);
	trafficLights->addChild(rightLight->root());



	
}

osg::Node* buildAnimatedVehicleAsset()
{
	osg::Group* pGroup = new osg::Group();

	osg::Geode* pGB = new osg::Geode();
	osg::ShapeDrawable* pGeomB = new osg::ShapeDrawable(new osg::Box(osg::Vec3(0.0f, 0.0f, 0.0f), 100.0f, 60.0f, 40.0f));
	osg::Material* pMat = new osg::Material();
	pMat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3f, 0.3f, 0.1f, 1.0f));
	pMat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.8f, 0.3f, 1.0f));
	pMat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0f, 1.0f, 0.6f, 1.0f));

	pGroup->addChild(pGB);
	pGB->addDrawable(pGeomB);

	pGB->getOrCreateStateSet()->setAttribute(pMat, osg::StateAttribute::ON || osg::StateAttribute::OVERRIDE);
	pGB->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE), osg::StateAttribute::ON || osg::StateAttribute::OVERRIDE);

	return pGroup;
}



osg::AnimationPath* createAnimationPath(raaAnimationPointFinders apfs, osg::Group* pRoadGroup)
{
	float fAnimTime = 0.0f;
	osg::AnimationPath* ap = new osg::AnimationPath();

	for (int i = 0; i < apfs.size(); i++)
	{
		float fDistance = 0.0f;
		osg::Vec3 vs;
		osg::Vec3 ve;

		vs.set(apfs[i].translation().x(), apfs[i].translation().y(), apfs[i].translation().z());

		if (i == apfs.size() - 1)
			ve.set(apfs[0].translation().x(), apfs[0].translation().y(), apfs[0].translation().z());
		else
			ve.set(apfs[i + 1].translation().x(), apfs[i + 1].translation().y(), apfs[i + 1].translation().z());

		float fXSqr = pow((ve.x() - vs.x()), 2);
		float fYSqr = pow((ve.y() - vs.y()), 2);
		float fZSqr = pow((ve.z() - vs.z()), 2);

		fDistance = sqrt(fXSqr + fYSqr);
		ap->insert(fAnimTime, osg::AnimationPath::ControlPoint(apfs[i].translation(), apfs[i].rotation()));
		fAnimTime += (fDistance / 10.0f);
	}

	return ap;
}

void buildRoad(osg::Group* pRoadGroup, osg::Group* pTrafficLightGroup)
{
	//Traffic Lights
	osg::Group* trafficLightGroup = new osg::Group();

	g_pRoot->addChild(trafficLightGroup);
	addLightsTile("roadStraight", "tile0", 0, 1, 0.0f, pRoadGroup, pTrafficLightGroup);
	addRoadTile("roadCurve", "tile1", 1, 1, 90.0f, pRoadGroup);
	addRoadTile("roadTJunction", "tile2", 1, 2, 90.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile3", 0, 2, 0.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile4", 2, 2, 90.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile5", 0, 3, 180.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile6", 2, 3, 270.0f, pRoadGroup);
	addRoadTile("roadXJunction", "tile7", -1, 3, 270.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile8", -1, 2, 90.0f, pRoadGroup);
	addRoadTile("roadTJunction", "tile9", -1, 1, 180.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile10", -1, 4, 270.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile11", 0, 4, 0.0f, pRoadGroup);
	addLightsTile("roadStraight", "tile12", 1, 4, 0.0f, pRoadGroup, pTrafficLightGroup);
	addRoadTile("roadStraight", "tile13", 2, 4, 0.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile14", 3, 4, 180.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile15", 3, 3, 90.0f, pRoadGroup);
	addLightsTile("roadCurve", "tile16", -2, 3, 270.0f, pRoadGroup, pTrafficLightGroup);
	addRoadTile("roadStraight", "tile17", -2, 2, 90.0f, pRoadGroup);
	addRoadTile("roadStraight", "tile18", -2, 1, 90.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile19", -2, 0, 0.0f, pRoadGroup);
	addRoadTile("roadCurve", "tile20", -1, 0, 90.0f, pRoadGroup);
}

void createCarOne(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile3", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));




	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car1"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarTwo(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car2"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarThree(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile20", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 2, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car3"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarFour(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile18", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile18", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile17", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile16", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile20", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile19", 5, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car4"), ap, 50.0);
	carGroup->addChild(pCar->root());
}


void createCarFive(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 6, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 6, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 12, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 7, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car5"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarSix(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile12", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 12, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 0, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car6"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarSeven(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile14", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 12, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 0, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car7"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarEight(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile15", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 11, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 12, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 5, pRoadGroup));

	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car8"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarNine(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();

	apfs.push_back(raaAnimationPointFinder("tile3", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 2, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car9"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarTen(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car10"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarEleven(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car11"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarTwelve(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 4, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car12"), ap, 50.0);
	carGroup->addChild(pCar->root());
}

void createCarThirteen(osg::Group* pRoadGroup, osg::Group* carGroup)
{
	raaAnimationPointFinders apfs;
	osg::AnimationPath* ap = new osg::AnimationPath();


	apfs.push_back(raaAnimationPointFinder("tile14", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile14", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile13", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile12", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile11", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile10", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile3", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile5", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 14, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile7", 10, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile8", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 8, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 9, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile9", 7, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile0", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile1", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile2", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile4", 2, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 3, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 4, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile6", 5, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 0, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 1, pRoadGroup));
	apfs.push_back(raaAnimationPointFinder("tile15", 2, pRoadGroup));



	ap = createAnimationPath(apfs, pRoadGroup);
	// NOTE: you will need to extend or develop the car facarde to manage the animmation speed and events
	raaCarFacarde* pCar = new raaCarFacarde(g_pRoot, raaAssetLibrary::getNamedAsset("vehicle", "car13"), ap, 50.0);
	carGroup->addChild(pCar->root());
}




int main(int argc, char** argv)
{
	raaAssetLibrary::start();
	raaTrafficSystem::start();

	osgViewer::Viewer viewer;

	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "-d") g_sDataPath = argv[++i];
	}

	// the root of the scene - use for rendering
	g_pRoot = new osg::Group();
	g_pRoot->ref();

	// build asset library - instances or clones of parts can be created from this
	raaAssetLibrary::loadAsset("roadStraight", g_sDataPath + "roadStraight.osgb");
	raaAssetLibrary::loadAsset("roadCurve", g_sDataPath + "roadCurve.osgb");
	raaAssetLibrary::loadAsset("roadTJunction", g_sDataPath + "roadTJunction.osgb");
	raaAssetLibrary::loadAsset("roadXJunction", g_sDataPath + "roadXJunction.osgb");
	raaAssetLibrary::loadAsset("trafficLight", g_sDataPath + "raaTrafficLight.osgb");
	raaAssetLibrary::insertAsset("vehicle", buildAnimatedVehicleAsset());

	// add a group node to the scene to hold the road sub-tree
	osg::Group* pRoadGroup = new osg::Group();
	g_pRoot->addChild(pRoadGroup);

	// Add a group node to the scene to hold the traffic light sub-tree
	osg::Group* trafficLightGroup = new osg::Group();
	g_pRoot->addChild(trafficLightGroup);

	// Add a group node to the scene to hold the cars sub-tree
	osg::Group* carGroup = new osg::Group();
	g_pRoot->addChild(carGroup);

	// Create road
	buildRoad(pRoadGroup, g_pRoot);

	// Add car one
	createCarOne(pRoadGroup, carGroup);
	createCarTwo(pRoadGroup, carGroup);
	createCarThree(pRoadGroup, carGroup);
	createCarFour(pRoadGroup, carGroup);
	createCarFive(pRoadGroup, carGroup);
	createCarSix(pRoadGroup, carGroup);
	createCarSeven(pRoadGroup, carGroup);
	createCarEight(pRoadGroup, carGroup);
	createCarNine(pRoadGroup, carGroup);
	createCarNine(pRoadGroup, carGroup);
	createCarTen(pRoadGroup, carGroup);
	createCarEleven(pRoadGroup, carGroup);
	createCarTwelve(pRoadGroup, carGroup);
	createCarThirteen(pRoadGroup, carGroup);

	// osg setup stuff
	osg::GraphicsContext::Traits* pTraits = new osg::GraphicsContext::Traits();
	pTraits->x = 20;
	pTraits->y = 20;
	pTraits->width = 600;
	pTraits->height = 480;
	pTraits->windowDecoration = true;
	pTraits->doubleBuffer = true;
	pTraits->sharedContext = 0;

	osg::GraphicsContext* pGC = osg::GraphicsContext::createGraphicsContext(pTraits);
	osgGA::KeySwitchMatrixManipulator* pKeyswitchManipulator = new osgGA::KeySwitchMatrixManipulator();
	pKeyswitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator());
	pKeyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator());
	pKeyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator());
	viewer.setCameraManipulator(pKeyswitchManipulator);
	osg::Camera* pCamera = viewer.getCamera();
	pCamera->setGraphicsContext(pGC);
	pCamera->setViewport(new osg::Viewport(0, 0, pTraits->width, pTraits->height));

	viewer.setUpViewOnSingleScreen();
	viewer.setUpViewInWindow(pTraits->x, pTraits->y, pTraits->width, pTraits->height);
	

	// add own event handler - this currently switches on an off the animation points
	viewer.addEventHandler(new raaInputController(g_pRoot));

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the record camera path handler
	viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

	// add the LOD Scale handler
	viewer.addEventHandler(new osgViewer::LODScaleHandler);

	// add the screen capture handler
	viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

	// set the scene to render
	viewer.setSceneData(g_pRoot);

	viewer.realize();


	return viewer.run();
	
}


