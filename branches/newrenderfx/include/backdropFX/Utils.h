// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_UTILS_H__
#define __BACKDROPFX_UTILS_H__ 1

#include <backdropFX/Export.h>
#include <osg/Notify>
#include <osg/FrameBufferObject>
#include <string>



namespace backdropFX {


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


// namespace backdropFX
}

// __BACKDROPFX_UTILS_H__
#endif
