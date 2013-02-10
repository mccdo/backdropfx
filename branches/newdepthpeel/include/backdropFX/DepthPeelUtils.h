// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_DEPTH_PEEL_UTILS_H__
#define __BACKDROPFX_DEPTH_PEEL_UTILS_H__ 1


#include <backdropFX/Export.h>
#include <osgwTools/TransparencyUtils.h>
#include <osg/StateSet>
#include <osg/Group>
#include <osg/Notify>


namespace backdropFX
{


template< class T >
bool transparentEnable( T* nodeOrDrawable, float alpha )
{
    bool success = osgwTools::transparentEnable( nodeOrDrawable, alpha );

    if( success )
    {
        osg::StateSet* stateSet = nodeOrDrawable->getOrCreateStateSet();
        stateSet->addUniform( new osg::Uniform( "bdfx_depthPeelAlpha.useAlpha", 1 ) );
        stateSet->addUniform( new osg::Uniform( "bdfx_depthPeelAlpha.alpha", alpha ) );
    }
    return( success );
}

template< class T >
bool transparentDisable( T* nodeOrDrawable, bool recursive=false )
{
    if( recursive )
        // TBD recursive removal of depth peel uniforms is TBD.
        osg::notify( osg::WARN ) << "BDFX: transparentDisable currently doesn't support recursive." << std::endl;

    bool success = osgwTools::transparentDisable( nodeOrDrawable, recursive );

    if( success )
    {
        osg::StateSet* stateSet = nodeOrDrawable->getOrCreateStateSet();
        stateSet->removeUniform( "bdfx_depthPeelAlpha.useAlpha" );
        stateSet->removeUniform( "bdfx_depthPeelAlpha.alpha" );
    }
    return( success );
}

BACKDROPFX_EXPORT void configureAsDepthPeel( osg::Group* group );

BACKDROPFX_EXPORT void depthPeelEnable( osg::Group* group );
BACKDROPFX_EXPORT void depthPeelDisable( osg::Group* group );


// backdropFX
}


// __BACKDROPFX_DEPTH_PEEL_UTILS_H__
#endif
