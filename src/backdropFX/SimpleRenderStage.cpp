// Copyright (c) 2008 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/SimpleRenderStage.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osgwTools/Version.h>

#include <backdropFX/Utils.h>
#include <string>



namespace backdropFX
{


SimpleRenderStage::SimpleRenderStage()
  : osgUtil::RenderStage()
{
    internalInit();
}

SimpleRenderStage::SimpleRenderStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs )
{
    internalInit();
}
SimpleRenderStage::SimpleRenderStage( const SimpleRenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs )
{
    internalInit();
}

SimpleRenderStage::~SimpleRenderStage()
{
    setClearMask( 0 );
}


void
SimpleRenderStage::internalInit()
{
}



void
SimpleRenderStage::setFBO( osg::FrameBufferObject* fbo )
{
    _fbo = fbo;
}
const osg::FrameBufferObject&
SimpleRenderStage::getFBO() const
{
    return( *_fbo );
}


void
SimpleRenderStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: SimpleRenderStage::draw" << std::endl;


    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: SRS: FBOExtensions == NULL." << std::endl;
        return;
    }


    // Bind the FBO.
    if( _fbo != NULL )
        _fbo->apply( state );
    else
    {
#if( OSGWORKS_OSG_VERSION > 20906 )
        fboExt->glBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
#else
        fboExt->glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
#endif
    }

    // Draw
    {
        state.applyAttribute( _viewport.get() );
        glClear( GL_DEPTH_BUFFER_BIT );
        
        RenderBin::drawImplementation( renderInfo, previous );

        state.apply();
    }

    // Unbind
    if( _fbo != NULL )
    {
#if( OSGWORKS_OSG_VERSION > 20906 )
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


// namespace backdropFX
}
