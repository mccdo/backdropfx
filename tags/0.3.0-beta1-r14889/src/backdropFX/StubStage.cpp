// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/StubStage.h>
#include <backdropFX/BackdropCommon.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osgwTools/Version.h>

#include <backdropFX/Utils.h>
#include <string>



namespace backdropFX
{


StubStage::StubStage()
  : osgUtil::RenderStage(),
    _backdropCommon( NULL )
{
    internalInit();
}

StubStage::StubStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _backdropCommon( NULL )
{
    internalInit();
}
StubStage::StubStage( const StubStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _backdropCommon( rhs._backdropCommon )
{
    internalInit();
}

StubStage::~StubStage()
{
}


void
StubStage::internalInit()
{
    setClearMask( 0 );
}



void
StubStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: StubStage::draw" << std::endl;


    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: SRS: FBOExtensions == NULL." << std::endl;
        return;
    }
    osg::GL2Extensions* gl2Ext( osg::GL2Extensions::Get( contextID, true ) );
    if( gl2Ext == NULL )
    {
        osg::notify( osg::WARN ) << "DPRS: GL2Extensions == NULL." << std::endl;
        return;
    }


    // Bind the FBO.
    osg::FrameBufferObject* fbo( _backdropCommon->getFBO() );
    if( fbo != NULL )
        fbo->apply( state );
    else
    {
#if( OSGWORKS_OSG_VERSION > 20907 )
        fboExt->glBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
#else
        fboExt->glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
#endif
    }
    UTIL_GL_ERROR_CHECK( "StubStage" );


    //
    // Render the input texture image to clear the depth buffer.

    _backdropCommon->performClear( renderInfo );

    // Done rendering input texture image.
    //

    // Render child stages.
    drawPreRenderStages(renderInfo,previous);


    // Unbind
    // If backdropFX is configured to render to window (no destination specified by app)
    // then this is the last code to execute as a prerender stage of the top-level
    // Camera's RenderStage. (RenderFX is drawn as post-render, in that case.)
    // After we finish here and return, one of the first things RenderStage will do 
    // is call glDrawBuffer(GL_BACK). We must unbind the FBO here, otherwise the
    // glDrawBuffer call will generate an INVALID_OPERATION error.
    if( fbo != NULL )
    {
#if( OSGWORKS_OSG_VERSION > 20907 )
        fboExt->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
#else
        fboExt->glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
#endif
    }

    if( state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS )
    {
        std::string msg( "at SRS draw end" );
        UTIL_GL_ERROR_CHECK( msg );
        UTIL_GL_FBO_ERROR_CHECK( msg, fboExt );
    }
}

void
StubStage::setBackdropCommon( BackdropCommon* backdropCommon )
{
    _backdropCommon = backdropCommon;
}




// namespace backdropFX
}
