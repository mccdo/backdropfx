// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/RenderingEffectsStage.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Effect.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Version.h>

#include <backdropFX/Utils.h>
#include <string>



namespace backdropFX
{


RenderingEffectsStage::RenderingEffectsStage()
  : osgUtil::RenderStage(),
    _renderingEffects( NULL )
{
    internalInit();
}

RenderingEffectsStage::RenderingEffectsStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _renderingEffects( NULL )
{
    internalInit();
}
RenderingEffectsStage::RenderingEffectsStage( const RenderingEffectsStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _renderingEffects( rhs._renderingEffects )
{
    internalInit();
}

RenderingEffectsStage::~RenderingEffectsStage()
{
    setClearMask( 0 );
}


void
RenderingEffectsStage::internalInit()
{
}



void
RenderingEffectsStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: RenderingEffectsStage::draw" << std::endl;


    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: RFXS: FBOExtensions == NULL." << std::endl;
        return;
    }


    EffectVector effectVector = _renderingEffects->getEffectVector();
    if( effectVector.empty() )
    {
        Effect* defaultEffect = _renderingEffects->getDefaultEffect();
        if( defaultEffect != NULL )
        {
            defaultEffect->draw( this, renderInfo );
        }

        else
        {
            // Bind the FBO.
            osg::FrameBufferObject* fbo( _renderingEffects->getFBO() );
            if( fbo != NULL )
            {
                fbo->apply( state );
            }
            else
            {
                osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0 );
            }
            UTIL_GL_ERROR_CHECK( "RenderFXStage" );

            // Draw
            {
                // Clears to the input texture.
                _renderingEffects->performClear( renderInfo );

                // No-op, does nothing, as RenderingEffects probably has no children.
                RenderBin::drawImplementation( renderInfo, previous );
            }
        }
    }
    else
    {
        // Iterate over and render all effects in the EffectVector.
        EffectVector::iterator itr;
        for( itr=effectVector.begin(); itr != effectVector.end(); itr++ )
            (*itr)->draw( this, renderInfo );
    }


    if( state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS )
    {
        std::string msg( "at RFXS draw end" );
        UTIL_GL_ERROR_CHECK( msg );
        UTIL_GL_FBO_ERROR_CHECK( msg, fboExt );
    }
}

void
RenderingEffectsStage::setRenderingEffects( RenderingEffects* renderingEffects )
{
    _renderingEffects = renderingEffects;
}



// namespace backdropFX
}
