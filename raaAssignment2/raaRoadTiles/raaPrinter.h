#pragma once

#include <osg/NodeVisitor>

class raaPrinter : public osg::NodeVisitor
{
public:
	raaPrinter();
	virtual ~raaPrinter();

	virtual void apply(osg::Node& node);
};

