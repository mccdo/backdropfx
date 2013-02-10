// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/Manager.h>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Node>
#include <osg/Geode>
#include <osg/ClipNode>
#include <osg/LightSource>
#include <osg/Light>
#include <osg/TexGenNode>
#include <osg/TexGen>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osg/TexEnv>
#include <osg/Fog>
#include <osg/BlendColor>
#include <osg/BlendFunc>
#include <osg/Uniform>
#include <osg/Notify>
#include <backdropFX/Utils.h>

#include <sstream>
#include <cmath>


namespace backdropFX
{



// TBD psh
ShaderModuleVisitor::LightSourceParameters::LightSourceParameters()
{
    // These are *not* OpenGL defaults, they are OSG defaults
    // taken from SceneView::setDefaults()
    _ambient = osg::Vec4f( 0.f, 0.f, 0.f, 1.f );
    _diffuse = osg::Vec4f( .8f, .8f, .8f, 1.f );
    _specular = osg::Vec4f( 1.f, 1.f, 1.f, 1.f );
    _position = osg::Vec4f( 0.f, 0.f, 1.f, 0.f );
    _halfVector = osg::Vec3f( 0.f, 0.f, 1.f );
    _spotDirection = osg::Vec3f( 0.f, 0.f, -1.f );
    _spotExponent = 0.f;
    _spotCutoff = 180.f;
    _constantAtt = 1.f;
    _linearAtt = 0.f;
    _quadAtt = 0.f;
    _absolute = 1.0;
}

// TBD psh
ShaderModuleVisitor::TexGenEyePlanes::TexGenEyePlanes()
{
    _planeS = osg::Vec4f( 1.f, 0.f, 0.f, 0.f );
    _planeT = osg::Vec4f( 0.f, 1.f, 0.f, 0.f );
    _planeR = osg::Vec4f( 0.f, 0.f, 1.f, 0.f ); // OSG default differs from OpenGL default
    _planeQ = osg::Vec4f( 0.f, 0.f, 0.f, 1.f ); // OSG default differs from OpenGL default
    _absolute = 0;
    _unit = 0;
}


unsigned int ShaderModuleVisitor::LightOff    ( 1u << 0 );
unsigned int ShaderModuleVisitor::LightOn     ( 1u << 1 );
unsigned int ShaderModuleVisitor::LightSun    ( 1u << 2 );
unsigned int ShaderModuleVisitor::LightSunOnly( 1u << 3 );
unsigned int ShaderModuleVisitor::LightLight0 ( 1u << 4 );

ShaderModuleVisitor::ShaderModuleVisitor( osg::NodeVisitor::TraversalMode tm )
  : osg::NodeVisitor( tm ),
    _attachMain( false ),
    _attachTransform( false ),
    _supportSunLighting( false ),
    _removeFFPState( true ),
    _convertSceneState( false ),
    _depth( 0 )
{
    __LOAD_SHADER(_vMain,osg::Shader::VERTEX,"shaders/gl2/ffp-main.vs")
    __LOAD_SHADER(_vInit,osg::Shader::VERTEX,"shaders/gl2/ffp-init.vs")
    __LOAD_SHADER(_vEyeCoordsOn,osg::Shader::VERTEX,"shaders/gl2/ffp-eyecoords-on.vs")
    __LOAD_SHADER(_vEyeCoordsOff,osg::Shader::VERTEX,"shaders/gl2/ffp-eyecoords-off.vs")
    __LOAD_SHADER(_vLightingOn,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-on.vs")
    __LOAD_SHADER(_vLightingLight0,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-light0.vs")
    __LOAD_SHADER(_vLightingOff,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-off.vs")
    __LOAD_SHADER(_vTransform,osg::Shader::VERTEX,"shaders/gl2/ffp-transform.vs")
    __LOAD_SHADER(_vFogOn,osg::Shader::VERTEX,"shaders/gl2/ffp-fog-on.vs")
    __LOAD_SHADER(_vFogOff,osg::Shader::VERTEX,"shaders/gl2/ffp-fog-off.vs")
    __LOAD_SHADER(_vFinalize,osg::Shader::VERTEX,"shaders/gl2/ffp-finalize.vs")

    __LOAD_SHADER(_fMain,osg::Shader::FRAGMENT,"shaders/gl2/ffp-main.fs")
    __LOAD_SHADER(_fInit,osg::Shader::FRAGMENT,"shaders/gl2/ffp-init.fs")
    __LOAD_SHADER(_fFinalize,osg::Shader::FRAGMENT,"shaders/gl2/ffp-finalize.fs")
    __LOAD_SHADER(_fFogOn,osg::Shader::FRAGMENT,"shaders/gl2/ffp-fog-on.fs")
    __LOAD_SHADER(_fFogOff,osg::Shader::FRAGMENT,"shaders/gl2/ffp-fog-off.fs")

    _lightShaders = 0;

    _initialState = NULL;

    int idx;
    for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
        _usesLightSource[ idx ] = ( idx == 0 ) ? true : false;
}
ShaderModuleVisitor::~ShaderModuleVisitor()
{
}

void ShaderModuleVisitor::setAttachMain( bool attachMain )
{
    _attachMain = attachMain;
}
void ShaderModuleVisitor::setAttachTransform( bool attachTransform )
{
    _attachTransform = attachTransform;
}
void ShaderModuleVisitor::setSupportSunLighting( bool supportSunLighting )
{
    _supportSunLighting = supportSunLighting;
    if( _supportSunLighting )
    {
        if( !_vLightingSun.valid() )
            __LOAD_SHADER(_vLightingSun,osg::Shader::VERTEX,"shaders/gl2/bdfx-lighting-on.vs")
        if( !_vLightingSunOnly.valid() )
            __LOAD_SHADER(_vLightingSunOnly,osg::Shader::VERTEX,"shaders/gl2/bdfx-lighting-sunonly.vs")
    }
}
void ShaderModuleVisitor::setRemoveFFPState( bool removeFFPState )
{
    _removeFFPState = removeFFPState;
}
void ShaderModuleVisitor::setConvertSceneState( bool convertSceneState )
{
    _convertSceneState = convertSceneState;
}


void ShaderModuleVisitor::apply( osg::Node& node )
{
    //osg::notify( osg::ALWAYS ) << "Node at depth: " << _depth << std::endl;

    pushStateSet( node.getStateSet() );
    convertStateSet( node.getStateSet() );

    _depth++;
    traverse( node );
    _depth--;

    popStateSet();
}

void ShaderModuleVisitor::apply( osg::Group& group )
{
    //osg::notify( osg::ALWAYS ) << "Node at depth: " << _depth << std::endl;

    pushStateSet( group.getStateSet() );
    convertStateSet( group );

    _depth++;
    traverse( group );
    _depth--;

    popStateSet();
}

void ShaderModuleVisitor::apply( osg::Geode& geode )
{
    //osg::notify( osg::ALWAYS ) << "Geode at depth: " << _depth << std::endl;

    pushStateSet( geode.getStateSet() );
    convertStateSet( geode.getStateSet() );

    unsigned int idx;
    for( idx=0; idx<geode.getNumDrawables(); idx++ )
    {
        osg::StateSet* ss = geode.getDrawable( idx )->getStateSet();
        if( ss != NULL )
        {
            pushStateSet( ss );
            convertStateSet( ss );
            popStateSet();
        }
    }

    popStateSet();
}
// TBD psh
void ShaderModuleVisitor::apply( osg::ClipNode& clipNode )
{
    osg::notify( osg::WARN ) << "backdropFX: ShaderModuleVisitor: Ignoring ClipNode." << std::endl;
}
// TBD psh
void ShaderModuleVisitor::apply( osg::LightSource& node )
{
    osg::Light* light = node.getLight();
    unsigned int lightNum = light->getLightNum();

    LightSourceParameters& lsp = _lights[ lightNum ];

    lsp._absolute = ( node.getReferenceFrame() == osg::LightSource::ABSOLUTE_RF ) ? 1.0 : 0.0;
    lsp._position = light->getPosition();
    if( lsp._absolute == 0.0 )
    {
        // Transform light position into world coordinates.
        osg::Matrix l2w = osg::computeLocalToWorld( getNodePath() );
        lsp._position = lsp._position * l2w;
    }

    lsp._ambient = light->getAmbient();
    lsp._diffuse = light->getDiffuse();
    lsp._specular = light->getSpecular();
    // TBD I'm not sure exactly how to compute this on the host.
    // Do we compute it in world coords on the host and transform
    // to eye coords on the shader? How do we support changing 
    // from local viewer true/false? For now, just compute in shader.
    // lsp._halfVector = ;
    lsp._spotDirection = light->getDirection();
    lsp._spotExponent = light->getSpotExponent();
    lsp._spotCutoff = light->getSpotCutoff();
    lsp._constantAtt = light->getConstantAttenuation();
    lsp._linearAtt = light->getLinearAttenuation();
    lsp._quadAtt = light->getQuadraticAttenuation();
}
// TBD psh
void ShaderModuleVisitor::apply( osg::TexGenNode& node )
{
    osg::TexGen* tg = node.getTexGen();
    _eyePlanes._unit = node.getTextureUnit();

    bool xform = ( node.getReferenceFrame() == osg::TexGenNode::RELATIVE_RF );
    _eyePlanes._absolute = 1; // TBD HACK
    if( xform )
    {
        // Transform eye planes into world coordinates.
        osg::Matrix l2w = osg::computeLocalToWorld( getNodePath() );
        osg::Plane p;
        p = tg->getPlane( osg::TexGen::S );
        p.transform( l2w );
        _eyePlanes._planeS = p.asVec4();
        p = tg->getPlane( osg::TexGen::T );
        p.transform( l2w );
        _eyePlanes._planeT = p.asVec4();
        p = tg->getPlane( osg::TexGen::R );
        p.transform( l2w );
        _eyePlanes._planeR = p.asVec4();
        p = tg->getPlane( osg::TexGen::Q );
        p.transform( l2w );
        _eyePlanes._planeQ = p.asVec4();
    }
    else
    {
        _eyePlanes._planeS = tg->getPlane( osg::TexGen::S ).asVec4();
        _eyePlanes._planeT = tg->getPlane( osg::TexGen::T ).asVec4();
        _eyePlanes._planeR = tg->getPlane( osg::TexGen::R ).asVec4();
        _eyePlanes._planeQ = tg->getPlane( osg::TexGen::Q ).asVec4();
    }
}


bool ShaderModuleVisitor::isSet( GLenum stateItem, osg::StateSet* ss )
{
    if( ss == NULL )
        return( false );

    // StateSet is not NULL. Query the mode.
    osg::StateAttribute::GLModeValue mode;
    mode = ss->getMode( stateItem );

    // The item is set if the mode is anything other than INHERIT.
    return( mode != osg::StateAttribute::INHERIT );
}
bool ShaderModuleVisitor::isEnabled( GLenum stateItem, osg::StateSet* ss )
{
    if( ss != NULL )
    {
        // StateSet is not NULL. Query the mode.
        osg::StateAttribute::GLModeValue mode;
        mode = ss->getMode( stateItem );

        if( mode & osg::StateAttribute::ON )
            // Item is enabled if its value is ON.
            return( true );
        else if(( mode & osg::StateAttribute::INHERIT ) == 0 )
            // Item is disabled if the INHERIT bit isn't set.
            return( false );
    }

    // If we get here, either the StateSet is NULL,
    // or the INHERIT bit is set (but not ON), so return the default.
    switch( stateItem )
    {
    case GL_LIGHTING:
    case GL_LIGHT0:
    case GL_COLOR_MATERIAL:
    case GL_DEPTH_TEST:
        return( true );
        break;
    default:
        return( false );
        break;
    }
}
bool ShaderModuleVisitor::isTextureSet( unsigned int unit, GLenum stateItem, osg::StateSet* ss )
{
    if( ss == NULL )
        return( false );

    // StateSet is not NULL. Query the mode.
    osg::StateAttribute::GLModeValue mode;
    mode = ss->getTextureMode( unit, stateItem );

    // The item is set if the mode is anything other than INHERIT.
    return( mode != osg::StateAttribute::INHERIT );
}
bool ShaderModuleVisitor::isTextureEnabled( unsigned int unit, GLenum stateItem, osg::StateSet* ss )
{
    if( ss != NULL )
    {
        // StateSet is not NULL. Query the mode.
        osg::StateAttribute::GLModeValue mode;
        mode = ss->getTextureMode( unit, stateItem );

        if( mode & osg::StateAttribute::ON )
            // Item is enabled if its value is ON.
            return( true );
        else if(( mode & osg::StateAttribute::INHERIT ) == 0 )
            // Item is disabled if the INHERIT bit isn't set.
            return( false );
    }
    return( false );
}

void ShaderModuleVisitor::convertStateSet( osg::Group& group )
{
    osg::StateSet* ss = group.getStateSet();
    if( ss == NULL )
        return;
    osg::StateSet* currentState = ( _stateStack.empty() ) ? NULL : _stateStack.back().get();

    // It might be the case that the input scene graph already has its own Program.
    // If so, do no further conversion.
    if( currentState->getAttribute( osg::StateAttribute::PROGRAM ) != NULL )
        return;

    ShaderModuleCullCallback* smccb = NULL;
    osg::Shader *shader = NULL, *shaderTwo = NULL;

    int idx;

    // Check GL_LIGHTING enable/disable
    const bool lightChanged = isSet( GL_LIGHTING, ss );
    osg::notify( osg::DEBUG_FP ) << "GL_LIGHTING changed? " << lightChanged << std::endl;
    if( _convertSceneState )
    {
        // Check GL_LIGHTi
        bool lightEnableChange( false );
        for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
        {
            if( isSet( GL_LIGHT0 + idx, ss ) )
            {
                ss->addUniform( new osg::Uniform( elementName( "bdfx_lightEnable", idx, "" ).c_str(),
                    isEnabled( GL_LIGHT0+idx, currentState ) ? 1 : 0 ) );

                // Mark that this scene graph uses this light source, so mergeDefaults
                // will add the light source paramter uniforms.
                _usesLightSource[ idx ] = _usesLightSource[ idx ] ||
                    isEnabled( GL_LIGHT0 + idx, ss );

                lightEnableChange = true;
            }
        }

        // Set the lighting shader if either GL_LIGHTING
        // or GL_LIGHTi changed state.
        if( lightChanged || lightEnableChange )
        {
            // Now determine which lighting shader module to use.
            osg::Shader* shader( NULL );
            if( isEnabled( GL_LIGHTING, currentState ) )
            {
                // TBD, use of the optimized shader should also consider GL_SEPARATE_SPECULAR. If
                // enabled, we can't use the optimized shader.

                const bool light0Enabled = isEnabled( GL_LIGHT0, currentState );
                bool useLight0( light0Enabled && !_supportSunLighting );
                bool useSunOnly( !light0Enabled && _supportSunLighting );
                for( idx=1; idx<BDFX_MAX_LIGHTS; idx++ )
                {
                    if( isEnabled( GL_LIGHT0+idx, currentState ) )
                    {
                        // Some other light is on, can't use optimized shader.
                        useLight0 = false;
                        useSunOnly = false;
                        break;
                    }
                }
                if( useLight0 )
                {
                    shader = _vLightingLight0.get();
                    _lightShaders |= LightLight0;
                }
                else if( useSunOnly )
                {
                    shader = _vLightingSunOnly.get();
                    _lightShaders |= LightSunOnly;
                }
                else if( _supportSunLighting )
                {
                    shader = _vLightingSun.get();
                    _lightShaders |= LightSun;
                }
                else
                {
                    shader = _vLightingOn.get();
                    _lightShaders |= LightOn;
                }
            }
            else
            {
                // Lighting is disabled.
                shader = _vLightingOff.get();
                _lightShaders |= LightOff;
            }

            osg::notify( osg::INFO ) << "Selected shader: " << shader->getName() << std::endl;

            if( smccb == NULL )
                smccb = getOrCreateShaderModuleCullCallback( group );
            smccb->setShader( getShaderSemantic( shader->getName() ), shader );
        }
    }
    else if( lightChanged )
    {
        // We're not converting scene state, but the scene graph
        // changes the value of the GL_LIGHTING mode.
        if( !( isEnabled( GL_LIGHTING, currentState ) ) )
        {
            osg::notify( osg::INFO ) << "bdfx: SMV: Explicitly disabling GL_LIGHTING." << std::endl;

            if( smccb == NULL )
                smccb = getOrCreateShaderModuleCullCallback( group );
            smccb->setShader( getShaderSemantic( _vLightingOff->getName() ), _vLightingOff.get() );
            _lightShaders |= LightOff;
        }
    }

    if( _removeFFPState )
    {
        ss->removeMode( GL_LIGHTING );
        for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
            ss->removeMode( GL_LIGHT0+idx );
    }

    // Fog
    if( isSet( GL_FOG, ss ) )
    {
        if( _convertSceneState )
        {
            if( isEnabled( GL_FOG, ss ) )
            {
                shader = _vFogOn.get();
                shaderTwo = _fFogOn.get();
            }
            else
            {
                shader = _vFogOff.get();
                shaderTwo = _fFogOff.get();
            }

            if( smccb == NULL )
                smccb = getOrCreateShaderModuleCullCallback( group );
            smccb->setShader( getShaderSemantic( shader->getName() ), shader );
            smccb->setShader( getShaderSemantic( shaderTwo->getName() ), shaderTwo );
        }
        else
        {
            // We're not converting scene state, but the scene graph
            // changes the value of the GL_FOG mode.
            if( !( isEnabled( GL_FOG, ss ) ) )
            {
                osg::notify( osg::INFO ) << "bdfx: SMV: Explicitly disabling GL_FOG." << std::endl;

                if( smccb == NULL )
                    smccb = getOrCreateShaderModuleCullCallback( group );
                smccb->setShader( getShaderSemantic( _vFogOff->getName() ), _vFogOff.get() );
                smccb->setShader( getShaderSemantic( _fFogOff->getName() ), _fFogOff.get() );
            }
        }

        if( _removeFFPState )
            ss->removeMode( GL_FOG );
    }

    convertStateSet( ss );
}


void ShaderModuleVisitor::convertMaterial( osg::Material* mat )
{
    UniformList ul;
    osg::ref_ptr< osg::Uniform > material;

    material = new osg::Uniform( "bdfx_colorMaterial", 0 );
    ul.push_back( material );

    bool lightModelSimple = backdropFX::Manager::instance()->getLightModelSimplified();

    if( !lightModelSimple )
    {
        material = new osg::Uniform( "bdfx_frontMaterial.emissive",
            mat->getEmission( osg::Material::FRONT ) );
        ul.push_back( material );
        material = new osg::Uniform( "bdfx_frontMaterial.ambient",
            mat->getAmbient( osg::Material::FRONT ) );
        ul.push_back( material );
    }
    material = new osg::Uniform( "bdfx_frontMaterial.diffuse",
        mat->getDiffuse( osg::Material::FRONT ) );
    ul.push_back( material );
    material = new osg::Uniform( "bdfx_frontMaterial.specular",
        mat->getSpecular( osg::Material::FRONT ) );
    ul.push_back( material );

    if( !lightModelSimple )
    {
        material = new osg::Uniform( "bdfx_backMaterial.emissive",
            mat->getEmission( osg::Material::BACK ) );
        ul.push_back( material );
        material = new osg::Uniform( "bdfx_backMaterial.ambient",
            mat->getAmbient( osg::Material::BACK ) );
        ul.push_back( material );
    }
    material = new osg::Uniform( "bdfx_backMaterial.diffuse",
        mat->getDiffuse( osg::Material::BACK ) );
    ul.push_back( material );
    material = new osg::Uniform( "bdfx_backMaterial.specular",
        mat->getSpecular( osg::Material::BACK ) );
    ul.push_back( material );

    material = new osg::Uniform( "bdfx_frontBackShininess",
        osg::Vec2f( mat->getShininess( osg::Material::FRONT ),
            mat->getShininess( osg::Material::BACK ) ) );
    ul.push_back( material );

    _attrMap[ mat ] = ul;
}

void ShaderModuleVisitor::convertTexture( osg::Texture2D* tex, AttributeUniformMap& am )
{
    // TBD nothing to do here? Remove this?
}

void ShaderModuleVisitor::convertTexEnv( osg::TexEnv* texEnv, unsigned int unit, AttributeUniformMap& am )
{
    UniformList ul;
    osg::ref_ptr< osg::Uniform > u;

    const char *envUnitNames[] = {"bdfx_textureEnvMode0", "bdfx_textureEnvMode1", "bdfx_textureEnvMode2", "bdfx_textureEnvMode3"}; // out to BDFX_MAX_TEXTURE_UNITS
    u = new osg::Uniform( envUnitNames[ unit ], texEnv->getMode() );
    ul.push_back( u );

    am[ texEnv ] = ul;
}

void ShaderModuleVisitor::convertTexGen( osg::TexGen* texGen, unsigned int unit, AttributeUniformMap& am )
{
    UniformList ul;
    osg::ref_ptr< osg::Uniform > u;

    u = new osg::Uniform( elementName( "bdfx_texGen", unit, "" ).c_str(), texGen->getMode() );
    ul.push_back( u );

    // setup per-mode uniforms for each texcoord unit
    // TBD we're caching per unit. We need to not set entire arrays, just the
    // element we need.
    osg::Vec4 planeS[BDFX_MAX_TEXTURE_COORDS], planeT[BDFX_MAX_TEXTURE_COORDS], planeR[BDFX_MAX_TEXTURE_COORDS], planeQ[BDFX_MAX_TEXTURE_COORDS];
    switch( texGen->getMode() )
    {
    // osg::TexGen shares the Plane members between object and eye linear
    case osg::TexGen::OBJECT_LINEAR:
    {
        planeS[unit] = texGen->getPlane(osg::TexGen::S).asVec4();
        planeT[unit] = texGen->getPlane(osg::TexGen::T).asVec4();
        planeR[unit] = texGen->getPlane(osg::TexGen::R).asVec4();
        planeQ[unit] = texGen->getPlane(osg::TexGen::Q).asVec4();
        break;
    } // object/eye linear
    case osg::TexGen::EYE_LINEAR:
    {
        // Assumes usage of TexGenNode, which handles plane transform.
        planeS[ unit ] = _eyePlanes._planeS;
        planeT[ unit ] = _eyePlanes._planeT;
        planeR[ unit ] = _eyePlanes._planeR;
        planeQ[ unit ] = _eyePlanes._planeQ;

        u = new osg::Uniform( "bdfx_eyeAbsolute", _eyePlanes._absolute );
        ul.push_back( u );
        break;
    } // object/eye linear
    default:
        break;
    }

    {
        // setup bdfx_eyePlane*
        osg::Uniform* texGenEPlaneS = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_eyePlaneS", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenEPlaneT = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_eyePlaneT", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenEPlaneR = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_eyePlaneR", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenEPlaneQ = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_eyePlaneQ", BDFX_MAX_TEXTURE_COORDS );

        // setup bdfx_objectPlane*
        osg::Uniform* texGenOPlaneS = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_objectPlaneS", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenOPlaneT = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_objectPlaneT", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenOPlaneR = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_objectPlaneR", BDFX_MAX_TEXTURE_COORDS );
        osg::Uniform* texGenOPlaneQ = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_objectPlaneQ", BDFX_MAX_TEXTURE_COORDS );

        // transfer the array contents. osg::Uniform doesn't provide a setArray(Vec4) methods, so we have to use setElement in a loop
        texGenEPlaneS->setElement(unit, planeS[unit]);
        texGenEPlaneT->setElement(unit, planeT[unit]);
        texGenEPlaneR->setElement(unit, planeR[unit]);
        texGenEPlaneQ->setElement(unit, planeQ[unit]);

        texGenOPlaneS->setElement(unit, planeS[unit]);
        texGenOPlaneT->setElement(unit, planeT[unit]);
        texGenOPlaneR->setElement(unit, planeR[unit]);
        texGenOPlaneQ->setElement(unit, planeQ[unit]);

        // now add the populated Uniform to the StateSet
        ul.push_back( texGenEPlaneS );
        ul.push_back( texGenEPlaneT );
        ul.push_back( texGenEPlaneR );
        ul.push_back( texGenEPlaneQ );

        ul.push_back( texGenOPlaneS );
        ul.push_back( texGenOPlaneT );
        ul.push_back( texGenOPlaneR );
        ul.push_back( texGenOPlaneQ );
    } // if instantiateTXGUniforms

    am[ texGen ] = ul;
}

void ShaderModuleVisitor::convertFog( osg::Fog* fog )
{
    UniformList ul;
    osg::ref_ptr< osg::Uniform > u;

    u = new osg::Uniform( "bdfx_fog.mode", fog->getMode() );
    ul.push_back( u );
    u = new osg::Uniform( "bdfx_fog.color", fog->getColor() );
    ul.push_back( u );
    u = new osg::Uniform( "bdfx_fog.density", fog->getDensity() );
    ul.push_back( u );
    u = new osg::Uniform( "bdfx_fog.start", fog->getStart() );
    ul.push_back( u );
    u = new osg::Uniform( "bdfx_fog.end", fog->getEnd() );
    ul.push_back( u );

    if( fog->getMode() == osg::Fog::LINEAR )
    {
        float fogScale = 1.0 / ( fog->getEnd() - fog->getStart() );
        u = new osg::Uniform( "bdfx_fog.scale", fogScale ); // don't make start==end or /0 will bite you
    }
    else
    {
        u = new osg::Uniform( "bdfx_fog.scale", 0.0f ); // not used except in linear
    } // else
    ul.push_back( u );

    _attrMap[ fog ] = ul;
}

void ShaderModuleVisitor::convertBlendFunc( osg::BlendFunc* bf, osg::StateSet* ss )
{
    osg::StateAttribute* sa;
    int useAlpha( 0 );
    float alpha( 1.f );
    switch( bf->getSource() )
    {
    case GL_CONSTANT_ALPHA:
    {
        sa = ss->getAttribute( osg::StateAttribute::BLENDCOLOR );
        osg::BlendColor* bc = dynamic_cast< osg::BlendColor* >( sa );
        if( bc != NULL )
        {
            useAlpha = 1;
            alpha = bc->getConstantColor().a();
        }
        break;
    }
    case GL_ONE_MINUS_CONSTANT_ALPHA:
    {
        sa = ss->getAttribute( osg::StateAttribute::BLENDCOLOR );
        osg::BlendColor* bc = dynamic_cast< osg::BlendColor* >( sa );
        if( bc != NULL )
        {
            useAlpha = 1;
            alpha = 1.f - bc->getConstantColor().a();
        }
        break;
    }
    case GL_SRC_ALPHA:
        useAlpha = 0;
        break;
    case GL_ONE:
        useAlpha = 1;
        alpha = 1.f;
        break;
    case GL_ZERO:
        useAlpha = 1;
        alpha = 0.f;
        break;
    default:
        osg::notify( osg::WARN ) << "BDFX: ShaderModuleVisitor: Unsupported source blend function: " <<
            std::hex << bf->getSource() << std::endl;
        break;
    }

    UniformList ul;
    osg::ref_ptr< osg::Uniform > u;

    u = new osg::Uniform( "bdfx_depthPeelAlpha.useAlpha", useAlpha );
    ul.push_back( u );
    u = new osg::Uniform( "bdfx_depthPeelAlpha.alpha", alpha );
    ul.push_back( u );

    _attrMap[ bf ] = ul;
}

void ShaderModuleVisitor::convertStateSet( osg::StateSet* ss )
{
    if( !ss )
        return;
    osg::StateSet* currentState = ( _stateStack.empty() ) ? NULL : _stateStack.back().get();

    // It might be the case that the input scene graph already has its own Program.
    // If so, do no further conversion.
    if( currentState->getAttribute( osg::StateAttribute::PROGRAM ) != NULL )
        return;

    // Material
    osg::StateAttribute* sa;
    sa = ss->getAttribute( osg::StateAttribute::MATERIAL );
    osg::Material* mat = dynamic_cast< osg::Material* >( sa );
    if( mat != NULL )
    {
        if( _attrMap.find( mat ) == _attrMap.end() )
            convertMaterial( mat );

        UniformList& ul = _attrMap[ mat ];
        UniformList::iterator it;
        for( it=ul.begin(); it!=ul.end(); it++ )
            ss->addUniform( it->get() );

        if( _removeFFPState )
            ss->removeAttribute( osg::StateAttribute::MATERIAL );
    }

    // Texture
    const char *enableUnitNames[] = {"bdfx_texture2dEnable0", "bdfx_texture2dEnable1", "bdfx_texture2dEnable2", "bdfx_texture2dEnable3"}; // out to BDFX_MAX_TEXTURE_UNITS
    int unit;
    for( unit=0; unit < BDFX_MAX_TEXTURE_UNITS; unit++)
    {
        // TBD Note that currently we only convert use of GL_TEXTURE_2D.
        if( !isTextureSet( unit, GL_TEXTURE_2D, ss ) )
            continue;

        // Set the texture enable state for this unit.
        {
            bool enabled = isTextureEnabled( unit, GL_TEXTURE_2D, ss );
            ModeUniformMap* mm;
            if( enabled )
                mm = &( _texModeOnMap[ unit ] );
            else
                mm = &( _texModeOffMap[ unit ] );

            if( (*mm).find( GL_TEXTURE_2D ) == (*mm).end() )
            {
                osg::ref_ptr< osg::Uniform > u = new osg::Uniform( enableUnitNames[ unit ], enabled?1:0 );
                UniformList ul;
                ul.push_back( u );
                (*mm)[ GL_TEXTURE_2D ] = ul;
            }

            UniformList& ul = (*mm)[ GL_TEXTURE_2D ];
            UniformList::iterator it;
            for( it=ul.begin(); it!=ul.end(); it++ )
                ss->addUniform( it->get() );
        }

#if 0
        // TBD do we need this? Aleady set enable uniform, and sampler uniforms
        // are set on the root node, so nothing to do here...?

        sa = ss->getTextureAttribute( unit, osg::StateAttribute::TEXTURE );
        osg::Texture2D* texture = dynamic_cast< osg::Texture2D* >( sa );
        if( texture == NULL )
            continue;

        AttributeUniformMap& am = _texAttrMap[ unit ];
        if( am.find( texture ) == am.end() )
            convertTexture( texture, am );
#endif

        if( _removeFFPState )
        {
            ss->removeTextureMode( unit, GL_TEXTURE_2D );
            // DO NOT remove the Texture2D. It is needed for shader-based rendering.
        }
    }

    // Texture environment
    for( unit=0; unit < BDFX_MAX_TEXTURE_UNITS; unit++)
    {
        osg::StateAttribute* texEnvModeSA = ss->getTextureAttribute( unit, osg::StateAttribute::TEXENV );
        osg::TexEnv* textureEnv = dynamic_cast< osg::TexEnv* >( texEnvModeSA );
        if( textureEnv == NULL )
            continue;

        AttributeUniformMap& am = _texAttrMap[ unit ];
        if( am.find( textureEnv ) == am.end() )
            convertTexEnv( textureEnv, unit, am );

        UniformList& ul = am[ textureEnv ];
        UniformList::iterator it;
        for( it=ul.begin(); it!=ul.end(); it++ )
            ss->addUniform( it->get() );

        if( _removeFFPState )
            ss->removeTextureAttribute( unit, osg::StateAttribute::TEXENV );
    }

    // Texture coordinate generation
    for( unit = 0; unit < BDFX_MAX_TEXTURE_COORDS; unit++)
    {
        // Note that when TexGen is set to ON in OSG, OSG enables all four
        // OpenGL enums. So, we only need to check one of them.
        if( !isTextureSet( unit, GL_TEXTURE_GEN_S, ss ) )
            continue;

        sa = ss->getTextureAttribute( unit, osg::StateAttribute::TEXGEN );
        osg::TexGen* texgen = dynamic_cast< osg::TexGen* >( sa );
        if( ( texgen == NULL ) || !isTextureEnabled( unit, GL_TEXTURE_GEN_S, ss ) )
            continue;

        AttributeUniformMap& am = _texAttrMap[ unit ];
        if( am.find( texgen ) == am.end() )
            convertTexGen( texgen, unit, am );

        UniformList& ul = am[ texgen ];
        UniformList::iterator it;
        for( it=ul.begin(); it!=ul.end(); it++ )
            ss->addUniform( it->get() );

        if( _removeFFPState )
            ss->removeTextureAttribute( unit, osg::StateAttribute::TEXGEN );
    }

    // Fog
    sa = ss->getAttribute( osg::StateAttribute::FOG );
    osg::Fog* fog = dynamic_cast< osg::Fog* >( sa );
    if( fog != NULL )
    {
        if( _convertSceneState )
        {
            if( _attrMap.find( fog ) == _attrMap.end() )
                convertFog( fog );

            UniformList& ul = _attrMap[ fog ];
            UniformList::iterator it;
            for( it=ul.begin(); it!=ul.end(); it++ )
                ss->addUniform( it->get() );
        }

        if( _removeFFPState )
            ss->removeAttribute( osg::StateAttribute::FOG );
    }

    // Alpha uniform for depth peeling.
    sa = ss->getAttribute( osg::StateAttribute::BLENDFUNC );
    osg::BlendFunc* bf = dynamic_cast< osg::BlendFunc* >( sa );
    if( bf != NULL )
    {
        if( _attrMap.find( bf ) == _attrMap.end() )
            convertBlendFunc( bf, ss );

        UniformList& ul = _attrMap[ bf ];
        UniformList::iterator it;
        for( it=ul.begin(); it!=ul.end(); it++ )
            ss->addUniform( it->get() );

#if 0
        // Oops. Don't remove BlendFunc because it is used by OpenGL *after*
        // the fragment shader executes.
        if( _removeFFPState )
        {
            ss->removeAttribute( osg::StateAttribute::BLENDFUNC );
            ss->removeAttribute( osg::StateAttribute::BLENDCOLOR );
        }
#endif
    }

    // Normalize and rescale normal
    if( isSet( GL_NORMALIZE, ss ) || isSet( GL_RESCALE_NORMAL, ss ) )
    {
        const bool enabled = ( isEnabled( GL_NORMALIZE, currentState ) ||
            isEnabled( GL_RESCALE_NORMAL, currentState ) );

        ModeUniformMap* mm;
        if( enabled )
            mm = &( _modeOnMap );
        else
            mm = &( _modeOffMap );

        if( (*mm).find( GL_NORMALIZE ) == (*mm).end() )
        {
            osg::ref_ptr< osg::Uniform > u = new osg::Uniform( "bdfx_normalize", enabled ? 1 : 0 );
            UniformList ul;
            ul.push_back( u );
            (*mm)[ GL_NORMALIZE ] = ul;
        }

        UniformList& ul = (*mm)[ GL_NORMALIZE ];
        UniformList::iterator it;
        for( it=ul.begin(); it!=ul.end(); it++ )
            ss->addUniform( it->get() );

        if( _removeFFPState )
        {
            ss->removeMode( GL_NORMALIZE );
            ss->removeMode( GL_RESCALE_NORMAL );
        }
    }



#if 0
    // Point sprites appear to be mutually exclusive with depth peeling,
    // which requires depth texture coords interpolated over the primitive.
    // Points don't interpolate varyings. gl_PointCoord could probably
    // be used to fake this, but for now just punt.
    // Point sprite
    {
        osg::StateAttribute::GLModeValue mode;
        mode = ss->getMode( GL_POINT_SPRITE );
        osg::Uniform* pointSprite( NULL );
        if( mode & osg::StateAttribute::ON )
            pointSprite = new osg::Uniform( "bdfx_pointSprite", 1 );
        else if( ( mode & osg::StateAttribute::INHERIT ) == 0 )
            pointSprite = new osg::Uniform( "bdfx_pointSprite", 0 );
        if( pointSprite )
            ss->addUniform( pointSprite );
    }
#else
    if( isSet( GL_POINT_SPRITE, ss ) &&
        isEnabled( GL_POINT_SPRITE, currentState ) )
    {
        osg::notify( osg::WARN ) << "ShaderModuleVisitor: Point sprites not supported." << std::endl;
    }
#endif
}


// This function assumes lighting is enabled. There's not much point in trying
// to optimize for the unlit case.
// If lighting is disabled in the initial state, but some child node of the
// current scene graph enables it, then SMV will insert the necessary shader
// on the child node. This function ensures that the light model and other
// uniforms are properly set. This isn't necessary for the parts of the scene
// graph that don't use lighting, but optimizing for such cases is complex
// and is non corner-case that the results aren't worth the cost.
void ShaderModuleVisitor::mergeDefaultsLighting( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet )
{
    unsigned int lightShaders( _lightShaders );

    // if _initialState doesn't have shader and
    //     current state doesn't have shader
    //   create shader and attach.

    std::string semantic;
    osg::Shader::Type type;
    semantic = getShaderSemantic( _vLightingOff->getName() );
    type = _vLightingOff->getType();

    bool initialStateHasLightShader( false );
    if( _initialSmccb.valid() )
        initialStateHasLightShader = ( _initialSmccb->getShader( semantic, type ) != NULL );
    if( ( smccb->getShader( semantic, type ) == NULL ) &&
        !initialStateHasLightShader )
    {
        if( _supportSunLighting )
        {
            smccb->setShader( semantic, _vLightingSun.get() );
        }
        else
        {
            // If Sun disabled, initial shader is the "light 0 only" optimization.
            smccb->setShader( semantic, _vLightingLight0.get() );
        }
        if( _lightShaders == 0 )
        {
            // Scene graph never changed any lighting state, so we never added bits
            // to the _lightShaders variable. Set it to a default now.
            if( _supportSunLighting )
                lightShaders = LightSun;
            else
                // If Sun disabled, initial shader is the "light 0 only" optimization.
                lightShaders = LightLight0;
        }
    }

    // Basic logic:
    //   If the uniform isn't already in the state set &&
    //         the uniform isn't already in _initialState
    //     create the uniform with the default value and add it to the state set
#define ADD_UNIFORM(_NAME,_VALUE) \
    { \
        std::string uniformName( _NAME ); \
        if( ( findUniform( uniformName, stateSet ) == NULL ) && \
                ( findUniform( uniformName, _initialState ) == NULL ) ) \
            stateSet->addUniform( new osg::Uniform( _NAME, _VALUE ) ); \
    }

    // Light source enables
    if( ( lightShaders & ~( LightLight0 | LightSunOnly ) ) != 0 )
    {
        // Light0 and SunOnly shaders never reference this uniform.
        ADD_UNIFORM( "bdfx_lightEnable0", 1 );
    }
    if( ( lightShaders & ( LightOn | LightSun ) ) != 0 )
    {
        // General purpose shaders for lighting support up to 8 FFP light sources.
        // Set them to initially disabled (if the uniforms aren't already present
        // in the initial or current state).
        ADD_UNIFORM( "bdfx_lightEnable1", 0 );
        ADD_UNIFORM( "bdfx_lightEnable2", 0 );
        ADD_UNIFORM( "bdfx_lightEnable3", 0 );
        ADD_UNIFORM( "bdfx_lightEnable4", 0 );
        ADD_UNIFORM( "bdfx_lightEnable5", 0 );
        ADD_UNIFORM( "bdfx_lightEnable6", 0 );
        ADD_UNIFORM( "bdfx_lightEnable7", 0 );
    }

    // Light sources.
    int idx;
    for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
    {
        if( ( idx > 0 ) && ( ( _lightShaders & ( LightOn | LightSun ) ) == 0 ) )
            // Shaders don't use anything other than light 0, so we're done.
            // I know, I know: for the SunOnly shader, we didn't even need to
            // specify the GL_LIGHT0 parameters. Save that for a future optimization.
            break;

        if( !( _usesLightSource[ idx ] ) )
            // Not using this light source. DOn't set default light source
            // parameter uniforms that would go unused.
            continue;

        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].ambient" ).c_str(), _lights[ idx ]._ambient );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].diffuse" ).c_str(), _lights[ idx ]._diffuse );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].specular" ).c_str(), _lights[ idx ]._specular );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].position" ).c_str(), _lights[ idx ]._position );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].halfVector" ).c_str(), _lights[ idx ]._halfVector );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotDirection" ).c_str(), _lights[ idx ]._spotDirection );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotExponent" ).c_str(), _lights[ idx ]._spotExponent );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotCutoff" ).c_str(), _lights[ idx ]._spotCutoff );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].spotCosCutoff" ).c_str(), cosf( _lights[ idx ]._spotCutoff ) );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].constantAttenuation" ).c_str(), _lights[ idx ]._constantAtt );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].linearAttenuation" ).c_str(), _lights[ idx ]._linearAtt );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].quadraticAttenuation" ).c_str(), _lights[ idx ]._quadAtt );
        ADD_UNIFORM( elementName( "bdfx_lightSource[", idx, "].absolute" ).c_str(), _lights[ idx ]._absolute );
    }

    // Light model
    ADD_UNIFORM( "bdfx_lightModel.ambient", osg::Vec4f( .1f, .1f, .1f, 1.f ) );
    ADD_UNIFORM( "bdfx_lightModel.localViewer", 0 );
    ADD_UNIFORM( "bdfx_lightModel.separateSpecular", 0 );
    ADD_UNIFORM( "bdfx_lightModel.twoSided", 0 );

    // Normalize.
    // Default is ON to match OSG behavior (OpenGL default is OFF).
    ADD_UNIFORM( "bdfx_normalize", 1 );

    // Color material
    //
    // Note OSG default is ON except when the scene graph specifies
    // a Material, then OSG turns it off. OpenGL default is OFF.
    //
    // ADD_UNIFORM macro will only set this if it's not already set in
    // initial or current state. This is exactly the behavior we want.
    // If initial or current state contained an FFP Material StateAttribute,
    // converting it to shaders would add this uniform with a value of 0.
    ADD_UNIFORM( "bdfx_colorMaterial", 1 );

    // Materials.
    // Currently, our lighting code support color material for ambient and diffuse only.
    // We need to set uniforms for default emissive, specular, and shininess.
    ADD_UNIFORM( "bdfx_frontMaterial.emissive", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
    ADD_UNIFORM( "bdfx_frontMaterial.specular", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );

    ADD_UNIFORM( "bdfx_backMaterial.emissive", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
    ADD_UNIFORM( "bdfx_backMaterial.specular", osg::Vec4f( 0.f, 1.f, 0.f, 1.f ) );

    ADD_UNIFORM( "bdfx_frontBackShininess", osg::Vec2f( 0.f, 0.f ) );
}

void ShaderModuleVisitor::mergeDefaultsTexture( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet )
{
    // Texture
    int unit;
    for( unit = 0; unit < BDFX_MAX_TEXTURE_UNITS; unit++ )
    {
        ADD_UNIFORM( elementName( "texture2dSampler", unit, "" ).c_str(), unit );
        ADD_UNIFORM( elementName( "bdfx_texture2dEnable", unit, "" ).c_str(), 0 );

        ADD_UNIFORM( elementName( "bdfx_textureEnvMode", unit, "" ).c_str(), 0 );

        ADD_UNIFORM( elementName( "bdfx_texGen", unit, "" ).c_str(), 0 );
    } // for
}


void ShaderModuleVisitor::mergeDefaultsPointSprite( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet )
{
    // Point sprite
    // When on, use gl_PointCoord in fragment shader to look up
    // texture values. When off, do the normal texture lookup.
    ADD_UNIFORM( "bdfx_pointSprite", 0 );
}

void ShaderModuleVisitor::mergeDefaults( osg::Node& node )
{
    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( node );
    osg::StateSet* stateSet = node.getOrCreateStateSet();

    // Load default shaders.
    std::string semantic;
    osg::Shader::Type type;
    semantic = getShaderSemantic( _vMain->getName() );
    type = _vMain->getType();
    if( _attachMain && !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( getShaderSemantic( _vMain->getName() ), _vMain.get() );

    semantic = getShaderSemantic( _vInit->getName() );
    type = _vInit->getType();
    if( !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( semantic, _vInit.get() );

    // Leave on globally, but really, we could disable this if
    // lighting and tex gen eye linear are disabled.
    semantic = getShaderSemantic( _vEyeCoordsOn->getName() );
    type = _vEyeCoordsOn->getType();
    if( !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( semantic, _vEyeCoordsOn.get() );

    semantic = getShaderSemantic( _vTransform->getName() );
    type = _vTransform->getType();
    if( _attachTransform && ( smccb->getShader( semantic, type ) == NULL ) )
        smccb->setShader( semantic, _vTransform.get() );

    if( _convertSceneState )
    {
        // Don't set the fog shader module if it's already set.
        semantic = getShaderSemantic( _vFogOff->getName() );
        type = _vFogOff->getType();
        if( smccb->getShader( semantic, type ) == NULL )
        {
            // fog defaults to off
            smccb->setShader( getShaderSemantic( _vFogOff->getName() ), _vFogOff.get() );
            smccb->setShader( getShaderSemantic( _fFogOff->getName() ), _fFogOff.get() );
        }
    }

    semantic = getShaderSemantic( _vFinalize->getName() );
    type = _vFinalize->getType();
    if( !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( semantic, _vFinalize.get() );


    semantic = getShaderSemantic( _fMain->getName() );
    type = _fMain->getType();
    if( _attachMain && !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( getShaderSemantic( _fMain->getName() ), _fMain.get() );

    semantic = getShaderSemantic( _fInit->getName() );
    type = _fInit->getType();
    if( !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( semantic, _fInit.get() );

    semantic = getShaderSemantic( _fFinalize->getName() );
    type = _fFinalize->getType();
    if( !( smccb->getShader( semantic, type ) ) )
        smccb->setShader( semantic, _fFinalize.get() );


    if( _convertSceneState )
        mergeDefaultsLighting( smccb, stateSet );
    mergeDefaultsTexture( smccb, stateSet );
    mergeDefaultsPointSprite( smccb, stateSet );


    // Controls how depth peeling selects an alpha value.
    // TBD make this a single vec2 uniform.
    ADD_UNIFORM( "bdfx_depthPeelAlpha.useAlpha", 0 );
    ADD_UNIFORM( "bdfx_depthPeelAlpha.alpha", 1.f );
}

osg::Uniform* ShaderModuleVisitor::findUniform( const std::string& name, const osg::StateSet* stateSet )
{
    if( stateSet == NULL )
        return( NULL );

    const osg::StateSet::UniformList ul = stateSet->getUniformList();
    osg::StateSet::UniformList::const_iterator it = ul.find( name );
    if( it == ul.end() )
        return( NULL );
    else
        return( it->second.first.get() );
}

void ShaderModuleVisitor::setInitialStateSet( osg::StateSet* stateSet, const ShaderModuleCullCallback::ShaderMap& shaders )
{
    _initialState = stateSet;

    // Store the shaders in an SMCCB to facilitate quick searches.
    if( !( _initialSmccb.valid() ) )
        _initialSmccb = new ShaderModuleCullCallback();

    ShaderModuleCullCallback::ShaderMap::const_iterator it;
    for( it=shaders.begin(); it != shaders.end(); it++ )
    {
        _initialSmccb->setShader( it->first, it->second.get() );
    }

    // Extract initial light source usage.
    int idx;
    for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
    {
        std::string name = elementName( "bdfx_lightEnable", idx, "" );
        osg::Uniform* u = findUniform( name, stateSet );
        if( u != NULL )
        {
            if( u->getType() == osg::Uniform::INT )
            {
                int v;
                u->get( v );
                _usesLightSource[ idx ] = ( v == 1 );
            }
        }
    }
}
void ShaderModuleVisitor::pushStateSet( osg::StateSet* ss )
{
    if( ss == NULL )
        ss = new osg::StateSet;

    if( _stateStack.size() > 0 )
    {
        osg::StateSet* oldTop = _stateStack.back().get();
        osg::StateSet* newTop = new osg::StateSet( *oldTop );
        newTop->merge( *ss );
        _stateStack.push_back( newTop );
    }
    else
    {
        _stateStack.push_back( ss );
    }
}
void ShaderModuleVisitor::popStateSet()
{
    if( _stateStack.size() > 0 )
        _stateStack.pop_back();
    else
        osg::notify( osg::WARN ) << "bdfx: ShaderModuleVisitor: State stack underflow." << std::endl;
}


// namespace backdropFX
}
