// Copyright (c) 2011 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/ShadowMapStage.h>
#include <backdropFX/ShadowMap.h>
#include <backdropFX/Manager.h>
#include <backdropFX/ShaderLibraryConstants.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Version.h>

#include <backdropFX/Utils.h>
#include <osg/io_utils>
#include <string>



namespace backdropFX
{



ShadowMapStage::ShadowInfo::ShadowInfo()
{
    internalInit();
}
ShadowMapStage::ShadowInfo::ShadowInfo( const ShadowInfo& rhs, const osg::CopyOp& copyop )
  : _depthTex( rhs._depthTex ),
    _depthFBO( rhs._depthFBO ),
    _viewport( rhs._viewport )
{
}
void ShadowMapStage::ShadowInfo::internalInit()
{
    // TBD size is a temporary hack. Eventually we want this to be view-dependent;
    // lights that are far away with attenuation will cast shadows over a small
    // area and therefore need a smaller texture size.
    const unsigned int dim( 1024 );

    _depthTex = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _depthTex.get(), "ShadowMap LightInfo _depthTex", );
    _depthTex->setName( "Shadow Depth Map" );
    _depthTex->setInternalFormat( GL_DEPTH_COMPONENT );
    _depthTex->setShadowComparison( true );
    _depthTex->setShadowCompareFunc( osg::Texture::LEQUAL ); // if in R <= texR, alpha set to 1.0
    _depthTex->setShadowTextureMode( osg::Texture::ALPHA );
    _depthTex->setBorderWidth( 0 );
    _depthTex->setTextureSize( dim, dim );
    _depthTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    _depthTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    // TBD make this a RenderBuffer.
    osg::ref_ptr< osg::Texture2D > scrachColorBuffer = new osg::Texture2D;
    UTIL_MEMORY_CHECK( scrachColorBuffer.get(), "Manager rebuild scrachColorBuffer", );
    scrachColorBuffer->setName( "Shadow Depth Map scrachColorBuffer" );
    scrachColorBuffer->setInternalFormat( GL_RGBA );
    scrachColorBuffer->setBorderWidth( 0 );
    scrachColorBuffer->setTextureSize( dim, dim );
    scrachColorBuffer->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    scrachColorBuffer->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    _depthFBO = new osg::FrameBufferObject;
    UTIL_MEMORY_CHECK( _depthFBO.get(), "ShadowMap LightInfo _depthFBO", );
    _depthFBO->setAttachment( osg::Camera::DEPTH_BUFFER,
        osg::FrameBufferAttachment( _depthTex.get() ) );
    _depthFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( scrachColorBuffer.get() ) );

    _viewport = new osg::Viewport( 0., 0., dim, dim );
}
void ShadowMapStage::ShadowInfo::resizeGLObjectBuffers( unsigned int maxSize )
{
    _depthTex->resizeGLObjectBuffers( maxSize );
    _depthFBO->resizeGLObjectBuffers( maxSize );
    _viewport->resizeGLObjectBuffers( maxSize );
}
void ShadowMapStage::ShadowInfo::releaseGLObjects( osg::State* state )
{
    _depthTex->releaseGLObjects( state );
    _depthFBO->releaseGLObjects( state );
    _viewport->releaseGLObjects( state );
}



ShadowMapStage::ShadowMapStage()
  : osgUtil::RenderStage(),
    _shadowMapNode( NULL )
{
    internalInit();
}

ShadowMapStage::ShadowMapStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _shadowMapNode( NULL )
{
    internalInit();
}
ShadowMapStage::ShadowMapStage( const ShadowMapStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _shadowMapNode( rhs._shadowMapNode)
{
    internalInit();
}

ShadowMapStage::~ShadowMapStage()
{
    setClearMask( GL_DEPTH_BUFFER_BIT );
}


void ShadowMapStage::internalInit()
{
    _viewProjStateSet = new osg::StateSet();
    osg::Uniform* vpu = new osg::Uniform( osg::Uniform::FLOAT_MAT4, "bdfx_shadowViewProj" );
    _viewProjStateSet->addUniform( vpu );

    _depthTexStateSet = new osg::StateSet();
    _depthTexStateSet->addUniform( vpu );
    // Texture state sttribute will be set during draw.
    osg::Uniform* su = new osg::Uniform( osg::Uniform::SAMPLER_2D_SHADOW, "bdfx_shadowDepthMap" );
    su->set( BDFX_TEX_UNIT_SHADOW_MAP );
    _depthTexStateSet->addUniform( su );
}



void ShadowMapStage::setFBO( osg::FrameBufferObject* fbo )
{
    _fbo = fbo;
}
const osg::FrameBufferObject& ShadowMapStage::getFBO() const
{
    return( *_fbo );
}


void ShadowMapStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: ShadowMapStage::draw" << std::endl;


    if( _camera )
        renderInfo.pushCamera( _camera );

    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: SRS: FBOExtensions == NULL." << std::endl;
        return;
    }

    // We'll set our own viewport, but OSG lazy state setting will override it
    // with the viewport it thinks should be in effect. So let's go ahead and
    // set it here, so OSG thinks it's already set.
    state.applyAttribute( getViewport() );


    Manager& mgr = *( Manager::instance() );
    if( _shadowInfoVec.size() < mgr.getNumLights() )
        _shadowInfoVec.resize( mgr.getNumLights() );

    ShadowInfoVec::iterator itr;
    unsigned int idx;
    for( itr=_shadowInfoVec.begin(), idx=0; itr!=_shadowInfoVec.end(); itr++, idx++ )
    {
        if( !( mgr.getLightEnable( idx ) ) )
            continue;
        if( !itr->valid() )
            *itr = new ShadowInfo;

        const ShadowInfo* si = itr->get();
        osg::Vec4 lightPos = mgr.getLightPosition( idx );

        // TBD need to support multiple lights/shadows
        if( idx == 0 )
            _depthTexStateSet->setTextureAttribute( BDFX_TEX_UNIT_SHADOW_MAP, si->_depthTex.get() );

        osg::Matrixf lightViewProj = computeLightMatrix( lightPos, getCamera()->getViewMatrix() );
        osg::Uniform* vpu = _depthTexStateSet->getUniform( "bdfx_shadowViewProj" );
        vpu->set( lightViewProj );

        // Bind the FBO.
        si->_depthFBO->apply( state );
        UTIL_GL_FBO_ERROR_CHECK( "SMS Post ShadowInfo FBO bind", fboExt );

        // Apply the viewport for this specific shadow depth map.
        si->_viewport->apply( state );

        // Draw
        {
            UTIL_GL_ERROR_CHECK( "SMS pre performClear()" );
            _shadowMapNode->performClear( renderInfo );
            
            RenderBin::drawImplementation( renderInfo, previous );

            //state.apply();
        }
    }

    // TBD dump image

    // Restore viewport.
    getViewport()->apply( state );


    if( state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS )
    {
        std::string msg( "at SRS draw end" );
        UTIL_GL_ERROR_CHECK( msg );
        UTIL_GL_FBO_ERROR_CHECK( msg, fboExt );
    }

    if( _camera )
        renderInfo.popCamera();
}

osg::Matrixf ShadowMapStage::computeLightMatrix( const osg::Vec4 pos, const osg::Matrix& view )
{
    osg::Vec3 look, at, up;
    view.getLookAt( look, at, up );
    at -= look;
    at.normalize();
    at = osg::Vec3( 0., 0., 0. );
    up = osg::Vec3( 0., 0., 1. );

    osg::Matrixf lightView, lightProj;
    if( pos[3] == 1.0 )
    {
        // Positional
        lightView = osg::Matrixf::lookAt( osg::Vec3( pos[0], pos[1], pos[2] ), at, up );
        lightProj = osg::Matrixf::perspective( 40., 1., 10., 150. );
    }
    else
    {
        // Directional
        at *= 30.;
        lightView = osg::Matrixf::lookAt( at + osg::Vec3( pos[0], pos[1], pos[2] ), at, up );
        lightProj = osg::Matrixf::ortho( -30., 30., -30., 30., -100., 100. );
    }
    return( lightView * lightProj );
}


void ShadowMapStage::setShadowMapNode( ShadowMap* shadowMapNode )
{
    _shadowMapNode = shadowMapNode;
}

void ShadowMapStage::setViewProjStateSet( osg::StateSet* ss )
{
    _viewProjStateSet = ss;
}
osg::StateSet* ShadowMapStage::getViewProjStateSet() const
{
    return( _viewProjStateSet.get() );
}
void ShadowMapStage::setDepthTexStateSet( osg::StateSet* ss )
{
    _depthTexStateSet = ss;
}
osg::StateSet* ShadowMapStage::getDepthTexStateSet() const
{
    return( _depthTexStateSet.get() );
}


void ShadowMapStage::resizeGLObjectBuffers( unsigned int maxSize )
{
    ShadowInfoVec::iterator itr;
    for( itr=_shadowInfoVec.begin(); itr != _shadowInfoVec.end(); itr++ )
    {
        if( itr->valid() )
            (*itr)->resizeGLObjectBuffers( maxSize );
    }

    osg::Object::resizeGLObjectBuffers( maxSize );
}

void ShadowMapStage::releaseGLObjects( osg::State* state ) const
{
    ShadowInfoVec::const_iterator itr;
    for( itr=_shadowInfoVec.begin(); itr != _shadowInfoVec.end(); itr++ )
    {
        if( itr->valid() )
            (*itr)->releaseGLObjects( state );
    }

    osg::Object::releaseGLObjects( state );
}



// namespace backdropFX
}
