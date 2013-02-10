// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_UTILS_H__
#define __BACKDROPFX_UTILS_H__ 1

#include <backdropFX/Export.h>
#include <osg/Notify>
#include <osg/FrameBufferObject>
#include <osgwTools/Version.h>
#include <string>



namespace backdropFX {


// Getting the Uniform location by a cached ID, instead of with a string lookup,
// is supported on the 2.8 branch starting with 2.8.5, and on the 2.9 branch
// starting with 2.9.10.
#define OSG_SUPPORTS_UNIFORM_ID \
    ( ( ( OSGWORKS_OSG_VERSION < 20900 ) && ( OSGWORKS_OSG_VERSION >= 20805 ) ) || \
    ( OSGWORKS_OSG_VERSION >= 20910 ) )



#define UTIL_MEMORY_CHECK( ptr, message, failureReturn ) \
{ \
    if( ptr == NULL ) \
    { \
        osg::notify( osg::WARN ) << "backdropFX: " << message << ": Memory allocation error." << std::endl; \
        return failureReturn; \
    } \
}


#define UTIL_GL_ERROR_CHECK( msg ) \
    backdropFX::errorCheck( msg );

void BACKDROPFX_EXPORT errorCheck( const std::string& msg );

#define UTIL_GL_FBO_ERROR_CHECK( msg, fboExt ) \
    backdropFX::fboErrorCheck( msg, fboExt );

void BACKDROPFX_EXPORT fboErrorCheck( const std::string& msg, osg::FBOExtensions* fboExt );



void debugDumpImage( const std::string& fileName, const char* pixels, const int w, const int h );
void debugDumpDepthImage( const std::string& fileName, const short* pixels, const int w, const int h );

/** Convenience routine for creating a uniform name string for an array uniform.
For example, if you want "bdfx_light[0].diffuse", then you would pass:
\li \c prefix: "bdfx_light["
\li \c element: 0
\li \c suffix: "].diffuse"
*/
std::string BACKDROPFX_EXPORT elementName( const std::string& prefix, int element, const std::string& suffix );


// namespace backdropFX
}

// __BACKDROPFX_UTILS_H__
#endif
