// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __DEFAULT_SCENE_H__
#define __DEFAULT_SCENE_H__

#include <osg/Node>
#include <osg/StateSet>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/Shader>


/** \cond */
class DefaultScene
{
public:
    DefaultScene();
    ~DefaultScene();

    virtual osg::Node* operator()( osg::Node* node=NULL );

protected:
    osg::Node* internalDefaultScene();

    osg::StateSet* glowStateSet( osg::Group* grp );
    osg::StateSet* glowTransparentStateSet( osg::Group* grp );

    osg::Geode* makePoints();
    void pointShader( osg::Group* grp );

    osg::Geometry* makeStreamline( const float radius, const osg::BoundingBox& bound, int nInstances, const osg::Vec4& color );
    osg::StateSet* streamlineStateSet( osg::Texture2D* texPos, int m, int n );
    void streamlineShader( osg::Group* grp );

    osg::Texture2D* streamlinePositionData( const osg::Vec3& loc, int& m, int& n, osg::BoundingBox& bound );
};
/** \endcond */


// __DEFAULT_SCENE_H__
#endif
