// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/SkyDomeStage.h>
#include <backdropFX/BackdropCommon.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Version.h>

#include <backdropFX/Utils.h>
#include <string>
#include <sstream>
#include <iomanip>



namespace backdropFX
{


SkyDomeStage::SkyDomeStage()
  : osgUtil::RenderStage(),
    _enable( true )
{
    internalInit();
}

SkyDomeStage::SkyDomeStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _enable( true )
{
    internalInit();
}
SkyDomeStage::SkyDomeStage( const SkyDomeStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _enable( rhs._enable )
{
    internalInit();
}

SkyDomeStage::~SkyDomeStage()
{
}


void
SkyDomeStage::internalInit()
{
    setClearMask( GL_DEPTH_BUFFER_BIT );
}




void
SkyDomeStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: SkyDomeStage::draw" << std::endl;


    // Fix for redmine 434. Camera must be set in renderInfo.
    // Required for any draw-time code that queries the camera
    // from renderInfo, such as OcclusionQueryNode.
    // The parent camera/RenderStage doesn't do this because we
    // are pre-render stages; it hasn't had a chance yet.
    if( _camera )
        renderInfo.pushCamera( _camera );

    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: SDS: FBOExtensions == NULL." << std::endl;
        return;
    }


    // Bind the FBO.
    osg::FrameBufferObject* fbo( _backdropCommon->getFBO() );
    if( fbo != NULL )
    {
        fbo->apply( state );
    }
    else
    {
        osgwTools::glBindFramebuffer( fboExt, GL_DRAW_FRAMEBUFFER_EXT, 0 );
        osgwTools::glBindFramebuffer( fboExt, GL_READ_FRAMEBUFFER_EXT, 0 );
    }

    state.applyAttribute( getViewport() );

    // Draw
    {
        UTIL_GL_ERROR_CHECK( "SDS pre performClear()" );
        _backdropCommon->performClear( renderInfo );

        if( _enable )
        {
            UTIL_GL_ERROR_CHECK( "SDS pre drawImplementation()" );
            RenderBin::drawImplementation( renderInfo, previous );
        }

        // I believe this has the net result of restoring OpenGL to the
        // current state at the corresponding SkyDome node.
        //state.apply();
    }

    const bool dumpImages = (
        ( _backdropCommon->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 );
    if( dumpImages )
    {
        osg::Viewport* vp = getViewport();
        GLint x( 0 ), y( 0 );
        GLsizei w, h;
        if( vp != NULL )
        {
            w = vp->width();
            h = vp->height();
        }
        else
        {
            osg::notify( osg::WARN ) << "BDFX: SkyDome: Null viewport dumping images. Using 512x512." << std::endl;
            w = h = 512;
        }

        const std::string fileName( createFileName( contextID ) );

        char* pixels = new char[ w * h * 4 ];
        glReadPixels( x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels );
        backdropFX::debugDumpImage( fileName, pixels, w, h );
        delete[] pixels;
    }


    if( state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS )
    {
        std::string msg( "at SDS draw end" );
        UTIL_GL_ERROR_CHECK( msg );
        UTIL_GL_FBO_ERROR_CHECK( msg, fboExt );
    }

    // Fix for redmine 434. See SkyDomeStage::draw() for more info.
    if( _camera )
        renderInfo.popCamera();
}

void
SkyDomeStage::setEnable( bool enable )
{
    _enable = enable;
    setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}


void
SkyDomeStage::setBackdropCommon( BackdropCommon* backdropCommon )
{
    _backdropCommon = backdropCommon;
}

std::string
SkyDomeStage::createFileName( unsigned int contextID )
{
    std::ostringstream ostr;
    ostr << _backdropCommon->debugImageBaseFileName( contextID );
    ostr << "skydome.png";
    return( ostr.str() );
}


// namespace backdropFX
}
