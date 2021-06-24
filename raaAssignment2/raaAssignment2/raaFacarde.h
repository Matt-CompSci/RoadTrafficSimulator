#pragma once
#include <osg/MatrixTransform>

// the base class of all facardes
// this adds 3 transforms to control translation, location and scale to the asset (which may be instanced or clones) allowing unique poition, rot, scale for this instance

typedef std::list<class raaFacarde*> raaFacardes;

class raaFacarde
{
public:
	raaFacarde(osg::Node* pPart);
	raaFacarde(osg::Node* pPart, osg::Vec3 vTrans);
	raaFacarde(osg::Node* pPart, osg::Vec3 vTrans, float fRot);
	raaFacarde(osg::Node* pPart, osg::Vec3 vTrans, float fRot, float fScale);
	virtual ~raaFacarde();
	osg::Node* pPart;

	osg::MatrixTransform* translation();
	osg::MatrixTransform* rotation();
	osg::MatrixTransform* scale();
	osg::Node* root();
	void showName(bool bShow);
	static void showNames(bool bShow);
	static void toggleNames();
	
	static const raaFacardes& facardes();
	
protected:
	void init();
	
	osg::MatrixTransform* m_pTranslation;
	osg::MatrixTransform* m_pRotation;
	osg::MatrixTransform* m_pScale;
	osg::Node* m_pRoot;
	osg::Switch* m_pNameSwitch;
	static raaFacardes sm_lFacardes;
	static bool sm_bShowNames;
};

