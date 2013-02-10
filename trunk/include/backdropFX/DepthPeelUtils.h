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


/** \defgroup DepthPeelUtils Utilities for Working with Depth Peeling and Transparency. */
/*@{*/

/** This function adds uniform
variables necessary to support BlendColor-like transparency without
GL2 predefined uniforms. */
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

/** This function removes uniforms used to implement BlendColor
transparency without GL2 predefined uniforms. */
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

/** Configure an arbitrary node for depth peeling. To do this, the function
sets the following states and shaders:

Add the required shader modules for depth peeling to the ShaderModuleCullCallback.

These modules are:

\li bdfx-main.vs
\li bdfx-main.fs
\li bdfx-depthpeel.fs

Disable blending by setting the mode to OFF | OVERRIDE.

Set AlphaFunc to pass with the OVERRIDE bit if alpha > 0.0.

The following uniforms and default values are:

\li Ppaque depth map texture unit
\li Previous layer depth map texture unit
\li Depth offset values
\li Depth peeling enable flag
*/
BACKDROPFX_EXPORT void configureAsDepthPeel( osg::Group* group );

/** Toggle depth peeling on a group node that is configured for depth peeling. */
BACKDROPFX_EXPORT void depthPeelEnable( osg::Group* group );
BACKDROPFX_EXPORT void depthPeelDisable( osg::Group* group );

/*@}*/


// backdropFX
}


// __BACKDROPFX_DEPTH_PEEL_UTILS_H__
#endif
