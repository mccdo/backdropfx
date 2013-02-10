// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/ShadowMap.h>
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
#include <osg/Fog>
#include <osg/Light>
#include <osgwTools/Shapes.h>
#include <osgwTools/Version.h>

#include <sstream>
#include <iomanip>



namespace backdropFX
{



/** \cond */
class EffectsCameraImageDump : public osg::Camera::DrawCallback
{
public:
    EffectsCameraImageDump( osg::Texture2D* texture )
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
            ostr << "effectsCamera" << ".png";
            fileName = std::string( ostr.str() );
        }

        char* pixels = new char[ w * h * 4 ];
        _texture->apply( *state );
        glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
        UTIL_GL_ERROR_CHECK( "EffectsCameraImageDump" );

        backdropFX::debugDumpImage( fileName, pixels, w, h );
        delete[] pixels;
    }

protected:
    ~EffectsCameraImageDump() {}

    osg::Texture* _texture;
    bool _enable;
};
/** \endcond */

/** \cond */
// Allows an RTT Camera to handle resize events.
// TBD Probably don't need this now that we create and resize our own
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
unsigned int Manager::shadowMap         ( 1u <<  1 );
unsigned int Manager::depthPeel         ( 1u <<  2 );
unsigned int Manager::defaultFeatures   (
    Manager::skyDome |
    Manager::shadowMap |
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
  : _fogEnable( false ),
    _fog( NULL ),
    _lightingEnable( true ),
    _lightModelSimplified( true ),
    _lightModelAmbient( osg::Vec4( 0.2, 0.2, 0.2, 1.0 ) ),
    _dm( 0 ),
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

    _shadowMap = new backdropFX::ShadowMap;
    UTIL_MEMORY_CHECK( _shadowMap.get(), "Manager constructor _shadowMap", )
    _shadowMap->setName( "ShadowMap" );

    _depthPart = new backdropFX::DepthPartition;
    UTIL_MEMORY_CHECK( _depthPart.get(), "Manager constructor _depthPart", )
    _depthPart->setName( "DepthPartition" );

    _depthPeel = new osg::Group;
    UTIL_MEMORY_CHECK( _depthPeel.get(), "Manager constructor _depthPeel", )
    _depthPeel->setName( "DepthPeel" );
    configureAsDepthPeel( _depthPeel.get() );

    _renderFX = new backdropFX::RenderingEffects;
    UTIL_MEMORY_CHECK( _renderFX.get(), "Manager constructor _renderFX", )
    _renderFX->setName( "RenderingEffects" );


    //
    // Framebuffer / texture allocations
    //

    // Color buffer A

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

    _colorBufferGlow = new osg::Texture2D;
    UTIL_MEMORY_CHECK( _colorBufferGlow.get(), "Manager _colorBufferGlow", );
    _colorBufferGlow->setName( "Color buffer glow" );
    _colorBufferGlow->setInternalFormat( GL_RGBA );
    _colorBufferGlow->setBorderWidth( 0 );
    _colorBufferGlow->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
    _colorBufferGlow->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
    _colorBufferGlow->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    _colorBufferGlow->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    // The effects camera writes to two color buffers. Here's the second one.
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

    // Depth partition setup.
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

        // As the bdfx-* family of shader modules continues to grow in features,
        // we'll need someplace to set default shader modules. _depthPart is perhaps
        // not the logical choice, but is conveniently located at the root of the
        // scene.
        fileName = std::string( "shaders/gl2/surface-surface-off.fs" );
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "DepthPart surface-surface-off.fs", );
        smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );

        fileName = std::string( "shaders/gl2/perpixel-lighting-off.fs" );
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "DepthPart perpixel-lighting-off.fs", );
        smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );
    }

    //
    // Effects camera setup. Needed for Glow and DOF effects.
    // TBD needs to be general-purpose for glow, DOF, glow and DOF, or none.
    // Currently set up for glow...
    _effectsCamera = new osg::Camera;
    UTIL_MEMORY_CHECK( _effectsCamera, "Manager constructor _effectsCamera", )
    _effectsCamera->setName( "_effectsCamera" );
    _effectsCamera->setCullCallback( new GlowResetCallback );
    // Effects camera is initially disabled. It is enabled in RenderingEffects::setEffectSet
    // when glow, DOF, or heat distortion effects are active.
    _effectsCamera->setNodeMask( 0 );
    // The effects camera doesn't need to clear. It uses an FSTP to clear MRTs.
    _effectsCamera->setClearMask( 0 );
    _effectsCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER );
#if( OSGWORKS_OSG_VERSION >= 20906 )
    _effectsCamera->setImplicitBufferAttachmentMask(
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT|osg::Camera::IMPLICIT_DEPTH_BUFFER_ATTACHMENT,
        osg::Camera::IMPLICIT_COLOR_BUFFER_ATTACHMENT );
#endif
    _effectsCamera->setRenderOrder( osg::Camera::PRE_RENDER );
    _effectsCamera->attach( osg::Camera::COLOR_BUFFER0, _colorBufferGlow.get() );
    _effectsCamera->attach( osg::Camera::COLOR_BUFFER1, _depthBuffer.get() );

    // Set up for debug image dump from the effectsCamera.
    _effectsCamera->setPostDrawCallback( new EffectsCameraImageDump( _colorBufferGlow.get() ) );

    {
        // Because the effects camera uses MRT, we need to "clear" the color buffers
        // by drawing a fullscreen quad.
        osg::ref_ptr< osg::Geode > geode = new osg::Geode;
        geode->setCullingActive( false );
        // Verts in clip coord / NDC space, requires no transform.
        osg::Drawable* draw = osgwTools::makePlane( osg::Vec3( -1., -1., 1. ),
            osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 2., 0. ) );
        draw->setUseDisplayList( false );
        draw->setUseVertexBufferObjects( true );
        geode->addDrawable( draw );
        _effectsCamera->addChild( geode.get() );

        osg::StateSet* stateSet = geode->getOrCreateStateSet();
        stateSet->setRenderBinDetails( -1, "RenderBin" );

        osg::Depth* depth = new osg::Depth;
        depth->setFunction( osg::Depth::ALWAYS );
        stateSet->setAttributeAndModes( depth );

        osg::ref_ptr< osg::Program > program = new osg::Program();
        osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( "shaders/effects/EffectsCameraClear.vs" ) );
        if( shader->getShaderSource().empty() )
        {
            osg::notify( osg::WARN ) << "backdropFX: Manager::internalInit: can't load shaders/effects/EffectsCameraClear.vs" << std::endl;
        }
        shader->setName( "EffectsCameraClear.vs" );
        program->addShader( shader );
        shader = new osg::Shader( osg::Shader::FRAGMENT );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( "shaders/effects/EffectsCameraClear.fs" ) );
        if( shader->getShaderSource().empty() )
        {
            osg::notify( osg::WARN ) << "backdropFX: Manager::internalInit: can't load shaders/effects/EffectsCameraClear.fs" << std::endl;
        }
        shader->setName( "EffectsCameraClear.fs" );
        program->addShader( shader );
        stateSet->setAttributeAndModes( program.get() );
    }

    {
        // Set simplified state for the effects pass. We don't need to set
        // any uniforms for the shaders here, as this is done when the app
        // calls RenderingEffects::setEffectsSet(), via the addEffectsPassUniforms()
        // function (and also removeEffectsPassUniforms()).

        osg::StateSet* ss = _effectsCamera->getOrCreateStateSet();

        ss->setMode( GL_CULL_FACE,
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );

        // Required for streamlines to render properly during glow pass.
        osg::BlendFunc* bf = new osg::BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        UTIL_MEMORY_CHECK( bf, "Managet glow BlendFunc", )
        ss->setAttributeAndModes( bf );


        ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *_effectsCamera );
        UTIL_MEMORY_CHECK( smccb, "Manager _effectsCamera SMCCB", );

        osg::ref_ptr< osg::Shader > shader;
        std::string fileName;
        fileName = "shaders/gl2/glow-main.fs";
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _effectsCamera glow-main.fs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        fileName = "shaders/gl2/glow-texture.fs";
        __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _effectsCamera glow-texture.fs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get() );

        fileName = "shaders/gl2/glow-init.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager _effectsCamera glow-init.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get(),
            ShaderModuleCullCallback::InheritanceOverride );

        fileName = "shaders/gl2/bdfx-main.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager bdfx-main.vs", );
        smccb->setShader( getShaderSemantic( fileName ), shader.get() );

        fileName = "shaders/gl2/shadowmap-texcoords-off.vs";
        __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
        UTIL_MEMORY_CHECK( shader, "Manager shadowmap-texcoords-off.vs", );
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

    // Set the default Effect (used when the client application hasn't
    // specified any other Effects in the EffectVector.
    osg::ref_ptr< Effect > effect = new Effect;
    effect->setName( "Default effect" );
    effect->addInput( 0, _colorBufferA.get() );
    effect->setProgram( createEffectProgram( "none" ) );
    _renderFX->setDefaultEffect( effect.get() );
}



void Manager::rebuild( unsigned int featureFlags )
{
    if( !_rootNode.valid() )
        internalInit();

    // Debug info.
    osg::notify( osg::INFO ) << "BDFX: rebuilding with flags: " << std::hex << featureFlags << std::dec << std::endl;
    if( ( featureFlags & skyDome ) != 0 )
        osg::notify( osg::INFO ) << "BDFX:\tskyDome" << std::endl;
    if( ( featureFlags & shadowMap ) != 0 )
        osg::notify( osg::INFO ) << "BDFX:\tshadowMap" << std::endl;
    if( ( featureFlags & depthPeel ) != 0 )
        osg::notify( osg::INFO ) << "BDFX:\tdepthPeel" << std::endl;

    _rootNode->removeChildren( 0, _rootNode->getNumChildren() );
    _depthPart->removeChildren( 0, _depthPart->getNumChildren() );
    _depthPeel->removeChildren( 0, _depthPeel->getNumChildren() );
    // Don't remove the first child, it clears the MRT buffers.
    _effectsCamera->removeChildren( 1, _effectsCamera->getNumChildren()-1 );


    if( featureFlags & skyDome )
        _rootNode->addChild( _skyDome.get() );

    if( featureFlags & shadowMap )
    {
        _rootNode->addChild( _shadowMap.get() );
        if( _sceneData.valid() )
            _shadowMap->addChild( _sceneData.get() );
    }

    // Node that DepthPartition is always present. It is "disabled" by
    // setting the number of partitions to 1.
    _rootNode->addChild( _depthPart.get() );

    // Turn on a Sun light.
    if( featureFlags & skyDome )
    {
        osg::Light* light = new osg::Light( 8 ); // TBD hardcoded 8 for Sun.
        setLight( light );

        setLightEnable( 8, true ); // TBD hardcoded 8 for Sun.
    }
    else
    {
        setLightEnable( 8, false ); // TBD hardcoded 8 for Sun.
    }

    // Configure appropriate shaders and uniforms on the DepthPartition Node
    // for fog, shadows, and lights.
    setSceneFogState( _depthPart.get() );
    setSceneFogState( _effectsCamera.get() );
    setSceneShadowState( ( ( featureFlags & shadowMap ) != 0 ) );
    setSceneLightState();


    // The depth peel Group is always present. It is enabled or disabled
    // using DepthPeelUtils.h utility functions. When disabled, it acts
    // like a simple Group node, and the app should get regular OSG
    // transparency.
    _depthPart->addChild( _depthPeel.get() );
    if( featureFlags & depthPeel )
        depthPeelEnable( _depthPeel.get() );
    else
        depthPeelDisable( _depthPeel.get() );

    // Note that the effects camera is always a child of the root.
    // It disabled by its node mask when not in use.
    _rootNode->addChild( _effectsCamera.get() );
    // Set a NULL Viewport. It's RenderStage gets the previous RenderStage's Viewport (from top-level Camera, e.g.)
    _effectsCamera->setViewport( NULL );
    if( _sceneData.valid() )
        _effectsCamera->addChild( _sceneData.get() );

    // Note that RenderingEffects is always present, even if there are no effects.
    // In that case, it just splats its input.
    _rootNode->addChild( _renderFX.get() );

    // Set the current debugging mode.
    _skyDome->setDebugMode( _dm );
    _shadowMap->setDebugMode( _dm );
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

    // Determine how clearing is done. If we have a Skydome, no
    // clearing is necessary (it paints every pixel). Without a
    // SkyDome, DepthPartition should clear.
    if( featureFlags & skyDome )
        _depthPart->setClearMode( backdropFX::BackdropCommon::CLEAR_OFF );
    else
        _depthPart->setClearMode( backdropFX::BackdropCommon::CLEAR );

    //
    // Wire up colorBufferA. This is usually the accumulated output of
    // SkyDOme, DepthPartition, and DepthPeel, used as input to
    // RenderingEffects.
    if( featureFlags & skyDome )
        _skyDome->setFBO( _colorBufferAFBO.get() );
    _depthPart->setFBO( _colorBufferAFBO.get() );

    // Wire up RenderingEffects output
    _renderFX->setFBO( _fbo.get() );


    // Rebuild shader modules.
    {
        // Need to save the effects camera's current node mask, and enable the glow
        // camera for this traversal so that shaders are built and ready to go
        // when the app (RenderingEffects) enables the effects camera.
        osg::Node::NodeMask mask = _effectsCamera->getNodeMask();
        _effectsCamera->setNodeMask( 0xffffffff );

        backdropFX::RebuildShaderModules rsm;
        _rootNode->accept( rsm );

        _effectsCamera->setNodeMask( mask );
    }

    resize();
}


#define __SET_OR_REMOVE( _name, _enable, _value, _ss ) \
    if( _enable ) \
    { \
        osg::Uniform* _u = new osg::Uniform( (_name), (_value) ); \
        (_ss)->addUniform( _u ); \
    } \
    else \
    { \
        (_ss)->removeUniform( (_name) ); \
    }

void Manager::setSceneFogState( osg::Node* node )
{
    std::string vFileName, fFileName;
    osg::ref_ptr< osg::Shader >* vShader;
    osg::ref_ptr< osg::Shader >* fShader;
    if( _fogEnable )
    {
        vFileName = "shaders/gl2/ffp-fog-on.vs";
        vShader = &_fogOnVertex;
        fFileName = "shaders/gl2/ffp-fog-on.fs";
        fShader = &_fogOnFragment;
    }
    else
    {
        vFileName = "shaders/gl2/ffp-fog-off.vs";
        vShader = &_fogOffVertex;
        fFileName = "shaders/gl2/ffp-fog-off.fs";
        fShader = &_fogOffFragment;
    }

    if( !( vShader->valid() ) )
    {
        __LOAD_SHADER( (*vShader), osg::Shader::VERTEX, vFileName );
        UTIL_MEMORY_CHECK( (*vShader), "Manager setSceneFogState vertex", );
    }
    if( !( fShader->valid() ) )
    {
        __LOAD_SHADER( (*fShader), osg::Shader::FRAGMENT, fFileName );
        UTIL_MEMORY_CHECK( (*fShader), "Manager setSceneFogState fragment", );
    }

    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *node );
    smccb->setShader( getShaderSemantic( vFileName ), vShader->get() );
    smccb->setShader( getShaderSemantic( fFileName ), fShader->get() );

    osg::StateSet* stateSet = node->getOrCreateStateSet();
    osg::Fog* fog = getFog();
    bool linFog = ( fog->getMode() == osg::Fog::LINEAR );
    __SET_OR_REMOVE( "bdfx_fog.mode", _fogEnable, (int)(fog->getMode()), stateSet );
    __SET_OR_REMOVE( "bdfx_fog.color", _fogEnable, (const osg::Vec4f)(fog->getColor()), stateSet );
    __SET_OR_REMOVE( "bdfx_fog.density", ( _fogEnable && !linFog ), (float)(fog->getDensity()), stateSet );
    __SET_OR_REMOVE( "bdfx_fog.start", ( _fogEnable && linFog ), (float)(fog->getStart()), stateSet );
    __SET_OR_REMOVE( "bdfx_fog.end", ( _fogEnable && linFog ), (float)(fog->getEnd()), stateSet );
    __SET_OR_REMOVE( "bdfx_fog.scale", ( _fogEnable && linFog ),
        (float)( 1.f / ( fog->getEnd() - fog->getStart() ) ), stateSet );
}
void Manager::setSceneShadowState( bool shadowsEnabled )
{
    // Set the shadow shaders and uniforms.
    std::string vFileName, fFileName;
    osg::ref_ptr< osg::Shader >* vShader;
    osg::ref_ptr< osg::Shader >* fShader;
    if( shadowsEnabled )
    {
        vFileName = "shaders/gl2/shadowmap-texcoords-on.vs";
        vShader = &_shadowsOnVertex;
        fFileName = "shaders/gl2/shadowmap-depthtest-on.fs";
        fShader = &_shadowsOnFragment;
    }
    else
    {
        vFileName = "shaders/gl2/shadowmap-texcoords-off.vs";
        vShader = &_shadowsOffVertex;
        fFileName = "shaders/gl2/shadowmap-depthtest-off.fs";
        fShader = &_shadowsOffFragment;
    }

    if( !( vShader->valid() ) )
    {
        __LOAD_SHADER( (*vShader), osg::Shader::VERTEX, vFileName );
        UTIL_MEMORY_CHECK( (*vShader), "Manager setShadowShaders vertex", );
    }
    if( !( fShader->valid() ) )
    {
        __LOAD_SHADER( (*fShader), osg::Shader::FRAGMENT, fFileName );
        UTIL_MEMORY_CHECK( (*fShader), "Manager setShadowShaders fragment", );
    }

    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *_depthPart );
    smccb->setShader( getShaderSemantic( vFileName ), vShader->get() );
    smccb->setShader( getShaderSemantic( fFileName ), fShader->get() );
}
void Manager::setSceneLightState()
{
    unsigned int idx;
    std::string vFileName;
    osg::ref_ptr< osg::Shader >* vShader;
    if( _lightingEnable )
    {
        // Find optimal shaders for currently enabled lights.
        bool light0only = true;
        bool sunOnly = _lightInfoVec.size() > 8; // TBD hardcoded 8 for Sun.
        bool sun = false;
        osg::ref_ptr< LightInfo > activeLight;
        for( idx=0; idx<_lightInfoVec.size(); idx++ )
        {
            activeLight = _lightInfoVec[ idx ].get();
            if( !activeLight.valid() )
                continue;

            if( activeLight->_enable )
            {
                if( idx > 0 )
                    light0only = false;
                if( idx != 8 ) // TBD hardcoded 8 for Sun.
                    sunOnly = false;
                else
                    sun = true;
            }
        }
        if( light0only )
        {
            vFileName = getLightModelSimplified() ?
                "shaders/gl2/simpleffp-lighting-light0.vs" :
                "shaders/gl2/ffp-lighting-light0.vs";
            vShader = &_light0Vertex;
        }
        else if( sun )
        {
            if( sunOnly )
            {
                vFileName = getLightModelSimplified() ?
                    "shaders/gl2/simplebdfx-lighting-sunonly.vs" :
                    "shaders/gl2/bdfx-lighting-sunonly.vs";
                vShader = &_lightSunOnlyVertex;
            }
            else
            {
                vFileName = getLightModelSimplified() ?
                    "shaders/gl2/simplebdfx-lighting-on.vs" :
                    "shaders/gl2/bdfx-lighting-on.vs";
                vShader = &_lightSunVertex;
            }
        }
        else
        {
            vFileName = getLightModelSimplified() ?
                "shaders/gl2/simpleffp-lighting-on.vs" :
                "shaders/gl2/ffp-lighting-on.vs";
            vShader = &_lightOnVertex;
        }
    }
    else
    {
        vFileName = "shaders/gl2/ffp-lighting-off.vs";
        vShader = &_lightOffVertex;
    }

    if( !( vShader->valid() ) )
    {
        __LOAD_SHADER( (*vShader), osg::Shader::VERTEX, vFileName );
        UTIL_MEMORY_CHECK( (*vShader), "Manager setSceneLightState", );
    }

    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( *_depthPart );
    smccb->setShader( getShaderSemantic( vFileName ), vShader->get() );

    osg::StateSet* stateSet = _depthPart->getOrCreateStateSet();

    osg::ref_ptr< osg::Light > activeOSGLight;
    for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
    {
        bool lightOn = ( _lightingEnable &&
            ( _lightInfoVec.size() > idx ) &&
            ( _lightInfoVec[ idx ].valid() ) &&
            ( _lightInfoVec[ idx ]->_enable ) );

        activeOSGLight = 0;
        if( lightOn )
            activeOSGLight = _lightInfoVec[ idx ]->_light.get();

        __SET_OR_REMOVE( elementName( "bdfx_lightEnable", idx, "" ).c_str(),
            true, (int)(lightOn?1:0), stateSet );
        __SET_OR_REMOVE( elementName( "bdfx_lightSource[", idx, "].ambient" ).c_str(),
            lightOn, activeOSGLight->getAmbient(), stateSet );
        __SET_OR_REMOVE( elementName( "bdfx_lightSource[", idx, "].diffuse" ).c_str(),
            lightOn, activeOSGLight->getDiffuse(), stateSet );
        __SET_OR_REMOVE( elementName( "bdfx_lightSource[", idx, "].specular" ).c_str(),
            lightOn, activeOSGLight->getSpecular(), stateSet );
        __SET_OR_REMOVE( elementName( "bdfx_lightSource[", idx, "].position" ).c_str(),
            lightOn, activeOSGLight->getPosition(), stateSet );
        __SET_OR_REMOVE( elementName( "bdfx_lightSource[", idx, "].absolute" ).c_str(),
            lightOn, (float)(0.f), stateSet ); // TBD need support for 'absolute' (eye) lights.

        /*
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].halfVector" ).c_str(), _lights[ idx ]._halfVector );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotDirection" ).c_str(), _lights[ idx ]._spotDirection );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotExponent" ).c_str(), _lights[ idx ]._spotExponent );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotCutoff" ).c_str(), _lights[ idx ]._spotCutoff );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotCosCutoff" ).c_str(), cosf( _lights[ idx ]._spotCutoff ) );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].constantAttenuation" ).c_str(), _lights[ idx ]._constantAtt );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].linearAttenuation" ).c_str(), _lights[ idx ]._linearAtt );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].quadraticAttenuation" ).c_str(), _lights[ idx ]._quadAtt );
        */
    }

    // Light model
    __SET_OR_REMOVE( "bdfx_lightModel.ambient", _lightingEnable, osg::Vec4f( .1f, .1f, .1f, 1.f ), stateSet );
    __SET_OR_REMOVE( "bdfx_lightModel.localViewer", _lightingEnable, 0, stateSet );
    __SET_OR_REMOVE( "bdfx_lightModel.separateSpecular", _lightingEnable, 0, stateSet );
    __SET_OR_REMOVE( "bdfx_lightModel.twoSided", _lightingEnable, 0, stateSet );

    // Normalize.
    // Default is ON to match OSG behavior (OpenGL default is OFF).
    __SET_OR_REMOVE( "bdfx_normalize", _lightingEnable, 1, stateSet );

    // Color material
    //
    // Note OSG default is ON except when the scene graph specifies
    // a Material, then OSG turns it off. OpenGL default is OFF.
    //
    // ADD_UNIFORM macro will only set this if it's not already set in
    // initial or current state. This is exactly the behavior we want.
    // If initial or current state contained an FFP Material StateAttribute,
    // converting it to shaders would add this uniform with a value of 0.
    __SET_OR_REMOVE( "bdfx_colorMaterial", _lightingEnable, 1, stateSet );

    // Materials.
    // Currently, our lighting code support color material for ambient and diffuse only.
    // We need to set uniforms for default emissive, specular, and shininess.
    __SET_OR_REMOVE( "bdfx_frontMaterial.emissive", _lightingEnable && !getLightModelSimplified(),
        osg::Vec4f( 0.f, 0.f, 0.f, 1.f ), stateSet );
    __SET_OR_REMOVE( "bdfx_frontMaterial.specular", _lightingEnable, osg::Vec4f( 0.f, 0.f, 0.f, 1.f ), stateSet );

    __SET_OR_REMOVE( "bdfx_backMaterial.emissive", _lightingEnable && !getLightModelSimplified(),
        osg::Vec4f( 0.f, 0.f, 0.f, 1.f ), stateSet );
    __SET_OR_REMOVE( "bdfx_backMaterial.specular", _lightingEnable, osg::Vec4f( 0.f, 0.f, 0.f, 1.f ), stateSet );

    __SET_OR_REMOVE( "bdfx_frontBackShininess", _lightingEnable, osg::Vec2f( 0.f, 0.f ), stateSet );
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

    // Resize the effects camera buffers.
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

        _effectsCamera->attach( osg::Camera::DEPTH_BUFFER, depthBufferGlow );
    }

    // This is how we set the CameraRequiresSetUp flag in the effectsCamera's RenderStage.
    {
        osg::NodeCallback* nodecb = _effectsCamera->getCullCallback();
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

    if( _effectsCamera.valid() )
    {
        // Don't remove the first child, it clears the MRT buffers.
        _effectsCamera->removeChildren( 1, _effectsCamera->getNumChildren()-1 );
        _effectsCamera->addChild( _sceneData.get() );
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


SkyDome& Manager::getSkyDome()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _skyDome.get() ) );
}

ShadowMap& Manager::getShadowMap()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _shadowMap.get() ) );
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
Manager::getEffectsCamera()
{
    if( !_rootNode.valid() )
        internalInit();

    return( *( _effectsCamera.get() ) );
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
osg::Texture2D* Manager::getShadowDepthMap( unsigned int index )
{
    if( !_rootNode.valid() )
        internalInit();

    return( _shadowDepthMap.get() );
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



void Manager::setFogState( osg::Fog* fog, bool enable )
{
    if( !_rootNode.valid() )
        internalInit();

    _fog = fog;
    _fogEnable = enable;

    // TBD need to tell SkyDome about current fog settings.

    setSceneFogState( _depthPart.get() );
    setSceneFogState( _effectsCamera.get() );
    // TBD auto-run RSM?
}
osg::Fog* Manager::getFog()
{
    if( !_fog.valid() )
        _fog = new osg::Fog;
    return( _fog.get() );
}
bool Manager::getFogEnable() const
{
    return( _fogEnable );
}



void Manager::setLightingEnable( bool enable )
{
    _lightingEnable = enable;

    setSceneLightState();
    // TBD auto-run RSM?
}
bool Manager::getLightingEnable() const
{
    return( _lightingEnable );
}
void Manager::setLight( osg::Light* light, bool enable )
{
    if( light == NULL )
        return;

    if( !_shadowMap.valid() )
        internalInit();

    const unsigned int lightNum = light->getLightNum();

    if( _lightInfoVec.size() <= lightNum )
        _lightInfoVec.resize( lightNum+1 );

    if( !( _lightInfoVec[ lightNum ].valid() ) )
        _lightInfoVec[ lightNum ] = new backdropFX::LightInfo;

    osg::ref_ptr< LightInfo > activeLight = _lightInfoVec[ lightNum ].get();

    activeLight->_light = light;
    activeLight->_enable = enable;

    setSceneLightState();
    // TBD auto-run RSM?
}
void Manager::setLightPosition( unsigned int lightNum, const osg::Vec4& pos, bool enable )
{
    if( !_shadowMap.valid() )
        internalInit();

    if( _lightInfoVec.size() <= lightNum )
        _lightInfoVec.resize( lightNum+1 );

    if( !( _lightInfoVec[ lightNum ].valid() ) )
        _lightInfoVec[ lightNum ] = new backdropFX::LightInfo;

    osg::ref_ptr< LightInfo > activeLight = _lightInfoVec[ lightNum ].get();
    
    if( !( activeLight->_light.valid() ) )
        activeLight->_light = new osg::Light( lightNum );

    activeLight->_light->setPosition( pos );
    activeLight->_enable = enable;

    osg::StateSet* stateSet = _depthPart->getOrCreateStateSet();

    __SET_OR_REMOVE( elementName( "bdfx_lightSource[", lightNum, "].position" ).c_str(),
        enable, activeLight->_light->getPosition(), stateSet );
}
osg::Vec4 Manager::getLightPosition( unsigned int lightNum ) const
{
    if( _lightInfoVec.size() <= lightNum )
        return( osg::Vec4( 0., 0., 0., 0. ) );

    if( !( _lightInfoVec[ lightNum ].valid() ) )
        return( osg::Vec4( 0., 0., 0., 0. ) );

    osg::Light* light = _lightInfoVec[ lightNum ]->_light.get();

    return( light->getPosition() );
}
void Manager::setLightEnable( unsigned int lightNum, bool enable )
{
    if( !_shadowMap.valid() )
        internalInit();

    if( ( _lightInfoVec.size() > lightNum ) &&
        ( _lightInfoVec[ lightNum ].valid() ) )
    {
        _lightInfoVec[ lightNum ]->_enable = enable;

        setSceneLightState();
        // TBD auto-run RSM?
    }
}
bool Manager::getLightEnable( unsigned int lightNum ) const
{
    if( !_shadowMap.valid() )
        return( false );

    if( !( _lightInfoVec[ lightNum ].valid() ) )
        return( false );

    return( _lightInfoVec[ lightNum ]->_enable );
}

void Manager::setLightModelSimplified( bool enable )
{
    _lightModelSimplified = enable;
}
bool Manager::getLightModelSimplified() const
{
    return( _lightModelSimplified );
}
void Manager::setLightModelAmbient( const osg::Vec4& ambient )
{
    _lightModelAmbient = ambient;

    if( !_depthPart.valid() )
        internalInit();

    osg::StateSet* stateSet = _depthPart->getOrCreateStateSet();
    __SET_OR_REMOVE( "bdfx_lightModel.ambient", true, _lightModelAmbient, stateSet );
}
osg::Vec4 Manager::getLightModelAmbient() const
{
    return( _lightModelAmbient );
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


void Manager::addEffectsPassUniforms( Effect::UniformVector& uniforms )
{
    osg::StateSet* ss = _effectsCamera->getOrCreateStateSet();
    if( ss == NULL )
    {
        osg::notify( osg::WARN ) << "NULL StateSet in addEffectsPassUniforms" << std::endl;
        return;
    }

    Effect::UniformVector::iterator it;
    for( it=uniforms.begin(); it != uniforms.end(); it++ )
    {
        ss->addUniform( (*it).get() );
    }
}
void Manager::removeEffectsPassUniforms( Effect::UniformVector& uniforms )
{
    osg::StateSet* ss = _effectsCamera->getOrCreateStateSet();
    if( ss == NULL )
    {
        osg::notify( osg::WARN ) << "NULL StateSet in addEffectsPassUniforms" << std::endl;
        return;
    }

    Effect::UniformVector::iterator it;
    for( it=uniforms.begin(); it != uniforms.end(); it++ )
    {
        ss->removeUniform( (*it).get() );
    }
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
        _shadowMap->setDebugMode( _dm );
        _depthPart->setDebugMode( _dm );
        // DepthPeelBin pulls the debug mode from the Manager instance.
        _renderFX->setDebugMode( _dm );

        // Configure _effectsCamera post-draw callback to dump images.
        EffectsCameraImageDump* gcid = dynamic_cast<
            EffectsCameraImageDump* >(_effectsCamera->getPostDrawCallback() );
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
