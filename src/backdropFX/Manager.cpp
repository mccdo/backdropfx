// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Effect.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <backdropFX/DepthPeelUtils.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/LocationData.h>
#include <backdropFX/Utils.h>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/BlendFunc>
#include <osg/ref_ptr>
#include <osgwTools/Shapes.h>
#include <osgwTools/Version.h>

#include <sstream>
#include <iomanip>



namespace backdropFX
{



/** \cond */
class GlowCameraImageDump : public osg::Camera::DrawCallback
{
public:
    GlowCameraImageDump( osg::Texture2D* texture )
      : _texture( texture ),
        _enable( false )
    {}

    void setEnable( bool enable ) { _enable = enable; }

    virtual void operator()( osg::RenderInfo& renderInfo ) const
    {
        if( !_enable )
            return;

        osg::State* state = renderInfo.getState();
        const unsigned int contextID = state->getContextID();

        const GLsizei w( _texture->getTextureWidth() );
        const GLsizei h( _texture->getTextureHeight() );

        std::string fileName;
        {
            std::ostringstream ostr;
            // Stolen / copied from BackdropCommon.
            ostr << std::setfill( '0' );
            ostr << "f" << std::setw( 6 ) << state->getFrameStamp()->getFrameNumber()
                << "_c" << std::setw( 2 ) << contextID << "_";
            ostr << "glowCamera" << ".png";
            fileName = std::string( ostr.str() );
        }

        char* pixels = new char[ w * h * 4 ];
        _texture->apply( *state );
        glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
        UTIL_GL_ERROR_CHECK( "GlowCameraImageDump" );

        backdropFX::debugDumpImage( fileName, pixels, w, h );
        delete[] pixels;
    }

protected:
    ~GlowCameraImageDump() {}

    osg::Texture* _texture;
    bool _enable;
};
/** \endcond */

/** \cond */
// Allows an RTT Camera to handle resize events.
// TBD Probably don't need this now that we create and resize out own
// texture for both glow color, and depth.
class GlowResetCallback : public osg::NodeCallback
{
public:
    GlowResetCallback()
      : _needs( false ),
        _frame( 0 )
    {}

    /** If _needs was set, save off the current frame number.
    When we hit the next frame, tell the RenderStage to run
    camera setup, and make sure the RenderStage FBO is NULL so
    it will get recreated.
    */
    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        unsigned int currentFrame = nv->getFrameStamp()->getFrameNumber();
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mutex );
            if( _needs )
            {
                _frame = currentFrame;
                _needs = false;
            }
        }
        if( currentFrame == _frame+1 )
        {
            osgUtil::CullVisitor* cv = static_cast< osgUtil::CullVisitor* >( nv );
            osgUtil::RenderStage* rs = cv->getCurrentRenderStage();
            rs->setCameraRequiresSetUp( true );
            rs->setFrameBufferObject( NULL );
        }
        traverse( node, nv );
    }

    void setNeedsCameraSetup( bool needs ) { _needs = needs; }

protected:
    bool _needs;
    unsigned int _frame;
    OpenThreads::Mutex  _mutex;
};
/** \endcond */


unsigned int Manager::skyDome           ( 1u <<  0 );
unsigned int Manager::depthPeel         ( 1u <<  1 );
unsigned int Manager::defaultFeatures   (
    Manager::skyDome |
    Manager::depthPeel
    );

backdropFX::Manager*
Manager::instance( const bool erase )
{
    static osg::ref_ptr< Manager > s_manager = new Manager;
    if( erase )
        s_manager = NULL;
    return( s_manager.get() );
}
Manager::Manager()
  : _dm( 0 ),
    _texW( 1280 ),
    _texH( 1024 )
{
    // DepthPeelBin should be created when the Manager is invoked, not
    // as a static during library load/init.
    _depthPeelBinProxy = new backdropFX::DepthPeelBin( osgUtil::RenderBin::getDefaultRenderBinSortMode() );
    osgUtil::RenderBin::addRenderBinPrototype( "DepthPeelBin", _depthPeelBinProxy.get() );
}
Manager::~Manager()
{
    osgUtil::RenderBin::removeRenderBinPrototype( _depthPeelBinProxy.get() );
}

void
Manager::internalInit()
{
    _rootNode = new osg::Group;
    UTIL_MEMORY_CHECK( _rootNode.get(), "Manager constructor _rootNode", )
    _rootNode->setName( "backdropFX Managed Root" );

    // Set the Sun position for shader lighting
    _rootNode->getOrCreateStateSet()->addUniform( LocationData::s_instance()->getSunPositionUniform() );

    _skyDome = new backdropFX::SkyDome;
    UTIL_MEMORY_CHECK( _skyDome.get(), "Manager constructor _skyDome", )
    _skyDome->setName( "SkyDome" );

    _depthPart = new backdropFX::DepthPartition;
    UTIL_MEMORY_CHECK( _depthPart.get(), "Manager constructor _depthPart", )
    _depthPart->setName( "DepthPartition" );

    {
        osg::StateSet* stateSet = _depthPart->getOrCreateStateSet();
        UTIL_MEMORY_CHECK( stateSet, "Manager constructor _depthPart StateSet", );

        ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *_depthPart );
        UTIL_MEMORY_CHECK( smccb, "Manager _depthPart SMCCB", );

        // Use bdfx-transform.vs instead of ffp-transform to support depth partitioning.
        osg::ref_ptr< osg::Shader > shader;
        std::string fileName = "shaders/gl2/bdfx-transform.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "DepthPart bdfx-transform.vs", );
        smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );
    }

    _depthPeel = new osg::Group;
    UTIL_MEMORY_CHECK( _depthPeel.get(), "Manager constructor _depthPeel", )
    _depthPeel->setName( "DepthPeel" );
    configureAsDepthPeel( _depthPeel.get() );

    _renderFX = new backdropFX::RenderingEffects;
    UTIL_MEMORY_CHECK( _renderFX.get(), "Manager constructor _renderFX", )
    _renderFX->setName( "RenderingEffects" );

    _colorBufferA = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _colorBufferA.get(), "Manager constructor _colorBufferA", );
    _colorBufferA->setName( "Color buffer A" );
    _colorBufferA->setInternalFormat( GL_RGBA );
    _colorBufferA->setBorderWidth( 0 );
    _colorBufferA->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    _colorBufferA->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    _colorBufferAFBO = new osg::FrameBufferObject;
    UTIL_MEMORY_CHECK( _colorBufferAFBO.get(), "Manager constructor _colorBufferAFBO", )
    _colorBufferAFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( _colorBufferA.get() ) );



    // Color buffer for glow.
    // TBD, in the future, the glow camera will also render a depth
    // buffer for use with the DOF Effect.
    _colorBufferGlow = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _colorBufferGlow.get(), "Manager _colorBufferGlow", );
    _colorBufferGlow->setName( "Color buffer glow" );
    _colorBufferGlow->setInternalFormat( GL_RGBA );
    _colorBufferGlow->setBorderWidth( 0 );
    _colorBufferGlow->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    _colorBufferGlow->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    _colorBufferGlow->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _colorBufferGlow->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    // The glow camera writes to two color buffers. Here's the second one.
    // It's not really a color buffer, though, it's a single-channel float
    // that stores the focal-distance-processed depth value (0.0 for
    // completely shart, and 1.0 for fully blurred). This is for
    // depth-of-field (DOF).
    _depthBuffer = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _depthBuffer.get(), "Manager _depthBuffer", );
    _depthBuffer->setName( "Depth buffer" );
    _depthBuffer->setInternalFormat( GL_RGBA );
    _depthBuffer->setBorderWidth( 0 );
    _depthBuffer->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    _depthBuffer->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    _depthBuffer->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _depthBuffer->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    //
    // Glow camera setup. Needed for Glow and DOF effects.
    // TBD needs to be general-purpose for glow, DOF, glow and DOF, or none.
    // Currently set up for glow...
    _glowCamera = new osg::Camera;
    UTIL_MEMORY_CHECK( _glowCamera, "Manager constructor _glowCamera", )
    _glowCamera->setName( "_glowCamera" );
    _glowCamera->setCullCallback( new GlowResetCallback );
    // Glow camera is initially disabled. It is enabled in EffectLibraryUtils
    // when a Glow or DOF Effect is enabled.
    _glowCamera->setNodeMask( 0 );
    // Glow camera doesn't need to clear. It uses an FSTP to clear MRTs.
    _glowCamera->setClearMask( 0 );
    _glowCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
#if( OSGWORKS_OSG_VERSION >= 20906 )
    _glowCamera->setImplicitBufferAttachmentMask(
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT );
#endif
    _glowCamera->setRenderOrder( osg::Camera::PRE_RENDER );
    _glowCamera->attach( osg::Camera::COLOR_BUFFER0, _colorBufferGlow.get() );
    _glowCamera->attach( osg::Camera::COLOR_BUFFER1, _depthBuffer.get() );

    // Set up for debug image dump from the glowCamera.
    _glowCamera->setPostDrawCallback( new GlowCameraImageDump( _colorBufferGlow.get() ) );

    {
        // Because the glow camera uses MRT, we need to "clear" the color buffers
        // by drawing a fullscreen quad.
        osg::ref_ptr< osg::Geode > geode = new osg::Geode;
        geode->setCullingActive( false );
        // Verts in clip coord / NDC space, requires no transform.
        osg::Drawable* draw = osgwTools::makePlane( osg::Vec3( -1., -1., 1. ),
            osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 2., 0. ) );
        draw->setUseDisplayList( false );
        draw->setUseVertexBufferObjects( true );
        geode->addDrawable( draw );
        _glowCamera->addChild( geode.get() );

        osg::StateSet* stateSet = geode->getOrCreateStateSet();
        stateSet->setRenderBinDetails( -1, "RenderBin" );

        osg::Depth* depth = new osg::Depth;
        depth->setFunction( osg::Depth::ALWAYS );
        stateSet->setAttributeAndModes( depth );

        osg::ref_ptr< osg::Program > program = new osg::Program();
        osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( "shaders/GlowCameraClear.vs" ) );
        program->addShader( shader );
        shader = new osg::Shader( osg::Shader::FRAGMENT );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( "shaders/GlowCameraClear.fs" ) );
        program->addShader( shader );
        stateSet->setAttributeAndModes( program.get() );
    }

    {
        osg::StateSet* ss = _glowCamera->getOrCreateStateSet();

        ss->setMode( GL_CULL_FACE,
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

        // Required for streamlines to render properly during glow pass.
        osg::BlendFunc* bf = new osg::BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        UTIL_MEMORY_CHECK( bf, "Managet glow BlendFunc", )
        ss->setAttributeAndModes( bf );

        osg::Uniform* glowUniform = new osg::Uniform( "bdfx_glowColor",
            osg::Vec4( 0., 0., 0., 1. ) );
        UTIL_MEMORY_CHECK( glowUniform, "Manager glowUniform", )
        ss->addUniform( glowUniform );


        ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *_glowCamera );
        UTIL_MEMORY_CHECK( smccb, "Manager _glowCamera SMCCB", );

        osg::ref_ptr< osg::Shader > shader;
        std::string fileName;
        fileName = "shaders/gl2/glow-main.fs";
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _glowCamera glow-main.fs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        fileName = "shaders/gl2/glow-texture.fs";
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _glowCamera glow-texture.fs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get() );

        fileName = "shaders/gl2/glow-init.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _glowCamera glow-init.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        fileName = "shaders/gl2/bdfx-main.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager bdfx-main.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get() );

        fileName = "shaders/gl2/glow-transform.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager glow-transform.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get() );

        fileName = "shaders/gl2/glow-finalize.fs";
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager glow-finalize.fs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        //
        // Don't need lighting or eyecoords in the glow pass.
        fileName = "shaders/gl2/ffp-lighting-off.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager ffp-lighting-off.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        fileName = "shaders/gl2/ffp-eyecoords-off.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager ffp-eyecoords-off.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );
    }

    // Det the default Effect (used when the client application hasn't
    // specified any other Effects in the EffectVector.
    osg::ref_ptr< Effect > effect = new Effect;
    effect->setName( "Default effect" );
    effect->addInput( 0, _colorBufferA.get() );
    effect->setProgram( createEffectProgram( "none" ) );
    _renderFX->setDefaultEffect( effect.get() );
}


void
Manager::rebuild( unsigned int featureFlags )
{
    if( !_rootNode.valid() )
        internalInit();

    // Debug info.
    if( osg::getNotifyLevel() > osg::NOTICE ) {
        osg::notify( osg::INFO ) << "BDFX: rebuilding with flags: " << featureFlags << std::endl;
    }

    _rootNode->removeChildren( 0, _rootNode->getNumChildren() );
    _depthPart->removeChildren( 0, _depthPart->getNumChildren() );
    _depthPeel->removeChildren( 0, _depthPeel->getNumChildren() );
    // Don't remove the first child, it clears the MRT buffers.
    _glowCamera->removeChildren( 1, _glowCamera->getNumChildren()-1 );


    if( featureFlags & skyDome )
        _rootNode->addChild( _skyDome.get() );

    // Node that DepthPartition is always present. It is "disabled" by
    // setting the number of partitions to 1.
    _rootNode->addChild( _depthPart.get() );

    // The depth peel Group is always present. It is enabled or disabled
    // using DepthPeelUtils.h utility functions. When disabled, it acts
    // like a simple Group node, and the app should get regular OSG
    // transparency.
    _depthPart->addChild( _depthPeel.get() );
    if( featureFlags & depthPeel )
        depthPeelEnable( _depthPeel.get() );
    else
        depthPeelDisable( _depthPeel.get() );

    // Note that the glow camera is always a child of the root.
    // It disabled by its node mask when not in use.
    _rootNode->addChild( _glowCamera.get() );
    // Set a NULL Viewport. It's RenderStage gets the previous RenderStage's Viewport (from top-level Camera, e.g.)
    _glowCamera->setViewport( NULL );
    if( _sceneData.valid() )
        _glowCamera->addChild( _sceneData.get() );

    // Note that RenderingEffects is always present, even if there are no effects.
    // In that case, it just splats its input.
    _rootNode->addChild( _renderFX.get() );

    // Set the current debugging mode.
    _skyDome->setDebugMode( _dm );
    _depthPart->setDebugMode( _dm );
    // DepthPeelBin pulls the debug mode from the Manager instance.
    _renderFX->setDebugMode( _dm );

    if( _sceneData.valid() )
    {
        // Attach scene data. There will always be a DepthPartition node
        // (when disabled, it will be set to 1 partition), and there will
        // always be a _depthPeel child (when disabled, it will be configured
        // as a standard Group).
        _depthPeel->addChild( _sceneData.get() );
    }

    //
    // Wire up colorBufferA. This is usually the accumulated output of
    // SkyDOme, DepthPartition, and DepthPeel, used as input to
    // RenderingEffects.
    if( featureFlags & skyDome )
        _depthPart->setClearMode( backdropFX::BackdropCommon::CLEAR_OFF );
    else
        _depthPart->setClearMode( backdropFX::BackdropCommon::CLEAR );

    if( featureFlags & skyDome )
        _skyDome->setFBO( _colorBufferAFBO.get() );
    _depthPart->setFBO( _colorBufferAFBO.get() );

    // Wire up RenderingEffects output
    _renderFX->setFBO( _fbo.get() );


    // Rebuild shader modules.
    {
        // Need to save the glow camera's current node mask, and enable the glow
        // camera for this traversal so that shaders are built and ready to go
        // when the app enables the glow camera.
        osg::Node::NodeMask mask = _glowCamera->getNodeMask();
        _glowCamera->setNodeMask( 0xffffffff );

        backdropFX::RebuildShaderModules rsm;
        _rootNode->accept( rsm );

        _glowCamera->setNodeMask( mask );
    }

    resize();
}

void
Manager::resize()
{
    if( !_rootNode.valid() )
        internalInit();

    // Resize the color buffer.
    _colorBufferA->setTextureSize( _texW, _texH );
    _colorBufferA->dirtyTextureObject();

    // Create a new render buffer (for depth), sized appropriate.
    osg::RenderBuffer* rb = new osg::RenderBuffer( _texW, _texH, GL_DEPTH_COMPONENT );
    UTIL_MEMORY_CHECK( rb, "Manager rebuild depth RenderBuffer", )
    _colorBufferAFBO->setAttachment( osg::Camera::DEPTH_BUFFER,
        osg::FrameBufferAttachment( rb ) );

    // Resize the glow camera buffers.
    {
        _colorBufferGlow->setTextureSize( _texW, _texH );
        _colorBufferGlow->dirtyTextureObject();

        _depthBuffer->setTextureSize( _texW, _texH );
        _depthBuffer->dirtyTextureObject();

        // Why are we making a texture? We don't need one.
        // a) The short answer: we might, in the future, when we do DOF.
        // b) The long answer: We use RTTViewport to always render into the lower-left
        // corner. But if we don't have depth attached, RenderStage create a depth
        // buffer for us, sized w+x, h+y. If xy are non zero, the depth buffer is bigger
        // that our color buffer and some drivers throw an OpenGL error for that. It'd be
        // nice if we could just size and attach our own RenderBuffer, but OSG doesn't let
        // you attach a RenderBuffer to a Camera. So we have to use a texture.
        osg::Texture2D* depthBufferGlow = new osg::Texture2D;
        UTIL_MEMORY_CHECK( depthBufferGlow, "Manager depthBufferGlow", )
        depthBufferGlow->setInternalFormat( GL_DEPTH_COMPONENT );
        depthBufferGlow->setBorderWidth( 0 );
        depthBufferGlow->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
        depthBufferGlow->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );
        depthBufferGlow->setTextureSize( _texW, _texH );

        _glowCamera->attach( osg::Camera::DEPTH_BUFFER, depthBufferGlow );
    }

    // This is how we set the CameraRequiresSetUp flag in the glowCamera's RenderStage.
    {
        osg::NodeCallback* nodecb = _glowCamera->getCullCallback();
        GlowResetCallback* grc = dynamic_cast< GlowResetCallback* >( nodecb );
        if( ( nodecb != NULL ) && ( grc == NULL ) )
            grc = dynamic_cast< GlowResetCallback* >( nodecb->getNestedCallback() );
        if( grc != NULL )
            grc->setNeedsCameraSetup( true );
    }

    // Send a resize to all rendering effects.
    _renderFX->setTextureWidthHeight( _texW, _texH );

    // This uniform contains the maximum width and height of our internal textures.
    // TBD Not sure any shaders currently use it.
    if( !_widthHeight.valid() )
    {
        _widthHeight = new osg::Uniform( osg::Uniform::FLOAT_VEC2, "bdfx_widthHeight" );
        UTIL_MEMORY_CHECK( _widthHeight.get(), "Manager rebuild depth _widthHeight uniform", )
        _rootNode->getOrCreateStateSet()->addUniform( _widthHeight.get() );
    }
    _widthHeight->set( osg::Vec2f( _texW, _texH ) );
}


osg::Node*
Manager::getManagedRoot()
{
    if( !_rootNode.valid() )
        internalInit();

    return( _rootNode.get() );
}

void
Manager::setSceneData( osg::Node* node )
{
    if( !_rootNode.valid() )
        internalInit();

    _sceneData = node;

    _depthPeel->removeChild( 0u );
    _depthPeel->addChild( _sceneData.get() );

    if( _glowCamera.valid() )
    {
        // Don't remove the first child, it clears the MRT buffers.
        _glowCamera->removeChildren( 1, _glowCamera->getNumChildren()-1 );
        _glowCamera->addChild( _sceneData.get() );
    }
}


void
Manager::setFBO( osg::FrameBufferObject* dest )
{
    if( !_rootNode.valid() )
        internalInit();

    _fbo = dest;
    _renderFX->setFBO( dest );
}
osg::FrameBufferObject*
Manager::getFBO() const
{
    return( _fbo.get() );
}


SkyDome&
Manager::getSkyDome()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _skyDome.get() ) );
}

DepthPartition&
Manager::getDepthPartition()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _depthPart.get() ) );
}

osg::Group&
Manager::getDepthPeel()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _depthPeel.get() ) );
}

osg::Camera&
Manager::getGlowCamera()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _glowCamera.get() ) );
}

RenderingEffects&
Manager::getRenderingEffects()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _renderFX.get() ) );
}


osg::Texture2D*
Manager::getColorBufferA()
{
    if( !_rootNode.valid() )
        internalInit();

    return( _colorBufferA.get() );
}
osg::Texture2D*
Manager::getColorBufferGlow()
{
    if( !_rootNode.valid() )
        internalInit();

    return( _colorBufferGlow.get() );
}
osg::Texture2D*
Manager::getDepthBuffer()
{
    if( !_rootNode.valid() )
        internalInit();

    return( _depthBuffer.get() );
}




void
Manager::setTextureWidthHeight( unsigned int texW, unsigned int texH )
{
    if( ( _texW != texW ) || ( _texH != texH ) )
    {
        _texW = texW;
        _texH = texH;
        resize();
    }
}
void
Manager::getTextureWidthHeight( unsigned int& texW, unsigned int& texH ) const
{
    texW = _texW;
    texH = _texH;
}


void
Manager::setDebugMode( unsigned int dm )
{
    if( _dm != dm )
    {
        _dm = dm;

        if( !_rootNode.valid() )
            internalInit();

        // TBD set the log verbosity
        //logverbosity = ( _dm != DEBUG_OFF ) ? on : off;

        // Update all managed classes.
        _skyDome->setDebugMode( _dm );
        _depthPart->setDebugMode( _dm );
        // DepthPeelBin pulls the debug mode from the Manager instance.
        _renderFX->setDebugMode( _dm );

        // Configure _glowCamera post-draw callback to dump images.
        GlowCameraImageDump* gcid = dynamic_cast<
            GlowCameraImageDump* >(_glowCamera->getPostDrawCallback() );
        if( gcid != NULL )
            gcid->setEnable( ( _dm & BackdropCommon::debugImages ) != 0 );
    }
}
unsigned int
Manager::getDebugMode()
{
    return( _dm );
}


// namespace backdropFX
}
