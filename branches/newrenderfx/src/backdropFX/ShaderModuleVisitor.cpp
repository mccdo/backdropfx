// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModule.h>
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

#include <sstream>
#include <cmath>


namespace backdropFX
{



ShaderModuleVisitor::LightSourceParameters::LightSourceParameters()
{
    _ambient = osg::Vec4f( 0.f, 0.f, 0.f, 1.f );
    _diffuse = osg::Vec4f( .8f, .8f, .8f, 1.f );
    _specular = osg::Vec4f( 1.f, 1.f, 1.f, 1.f );
    _position = osg::Vec4f( 0.f, 0.f, 1.f, 0.f );
    _halfVector = osg::Vec3f( 0.f, 0.f, 1.f );
    _spotDirection = osg::Vec3f( 0.f, 0.f, 1.f );
    _spotExponent = 0.f;
    _spotCutoff = 180.f;
    _constantAtt = 1.f;
    _linearAtt = 0.f;
    _quadAtt = 0.f;
    _absolute = 1;
}


ShaderModuleVisitor::TexGenEyePlanes::TexGenEyePlanes()
{
    _planeS = osg::Vec4f( 1.f, 0.f, 0.f, 0.f );
    _planeT = osg::Vec4f( 0.f, 1.f, 0.f, 0.f );
    _planeR = osg::Vec4f( 0.f, 0.f, 1.f, 0.f ); // OSG default differs from OpenGL default
    _planeQ = osg::Vec4f( 0.f, 0.f, 0.f, 1.f ); // OSG default differs from OpenGL default
    _absolute = 0;
    _unit = 0;
}


ShaderModuleVisitor::ShaderModuleVisitor( osg::NodeVisitor::TraversalMode tm )
  : osg::NodeVisitor( tm ),
    _addDefaults( true ),
    _attachMain( false ),
    _attachTransform( false ),
    _supportSunLighting( false ),
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
}
ShaderModuleVisitor::~ShaderModuleVisitor()
{
}

void
ShaderModuleVisitor::setAddDefaults( bool addDefaults )
{
    _addDefaults = addDefaults;
}
void
ShaderModuleVisitor::setAttachMain( bool attachMain )
{
    _attachMain = attachMain;
}
void
ShaderModuleVisitor::setAttachTransform( bool attachTransform )
{
    _attachTransform = attachTransform;
}
void
ShaderModuleVisitor::setSupportSunLighting( bool supportSunLighting )
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

void
ShaderModuleVisitor::apply( osg::Node& node )
{
    //osg::notify( osg::ALWAYS ) << "Node at depth: " << _depth << std::endl;

    pushStateSet( node.getStateSet() );
    convertStateSet( node.getStateSet() );

    _depth++;
    traverse( node );
    _depth--;

    popStateSet();

    if( ( _depth==0 ) && _addDefaults )
        setDefaults( node );
}

void
ShaderModuleVisitor::apply( osg::Group& group )
{
    //osg::notify( osg::ALWAYS ) << "Node at depth: " << _depth << std::endl;

    pushStateSet( group.getStateSet() );
    convertStateSet( group );

    _depth++;
    traverse( group );
    _depth--;

    popStateSet();

    if( ( _depth==0 ) && _addDefaults )
        setDefaults( group );
}

void
ShaderModuleVisitor::apply( osg::Geode& geode )
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

    if( ( _depth==0 ) && _addDefaults )
        setDefaults( geode );
}
void
ShaderModuleVisitor::apply( osg::ClipNode& clipNode )
{
    osg::notify( osg::WARN ) << "backdropFX: ShaderModuleVisitor: Ignoring ClipNode." << std::endl;
}
void
ShaderModuleVisitor::apply( osg::LightSource& node )
{
    osg::Light* light = node.getLight();
    unsigned int lightNum = light->getLightNum();

    LightSourceParameters& lsp = _lights[ lightNum ];

    lsp._absolute = (int)( node.getReferenceFrame() == osg::LightSource::ABSOLUTE_RF );
    lsp._position = light->getPosition();
    if( lsp._absolute == 0 )
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
void
ShaderModuleVisitor::apply( osg::TexGenNode& node )
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


bool
ShaderModuleVisitor::isEnabled( GLenum stateItem, osg::StateSet* ss )
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
bool
ShaderModuleVisitor::isSet( GLenum stateItem, osg::StateSet* ss )
{
    if( ss == NULL )
        return( false );

    // StateSet is not NULL. Query the mode.
    osg::StateAttribute::GLModeValue mode;
    mode = ss->getMode( stateItem );

    // The item is set if the mode is anything other than INHERIT.
    return( mode != osg::StateAttribute::INHERIT );
}

void
ShaderModuleVisitor::convertStateSet( osg::Group& group )
{
    osg::StateSet* ss = group.getStateSet();
    if( ss == NULL )
        return;
    osg::StateSet* currentState = ( _stateStack.empty() ) ? NULL : _stateStack.back().get();

    ShaderModuleCullCallback* smccb = NULL;
    osg::Shader *shader = NULL, *shaderTwo = NULL;

    // Checl GL_LIGHTING enable/disable
    const bool lightChanged = isSet( GL_LIGHTING, ss );
    osg::notify( osg::DEBUG_FP ) << "GL_LIGHTING changed? " << lightChanged << std::endl;

    // Check GL_LIGHTi
    bool lightEnableChanged( false );
    int idx;
    for( idx=0; idx<BDFX_MAX_LIGHTS; idx++ )
    {
        lightEnableChanged = isSet( GL_LIGHT0 + idx, ss );
        if( lightEnableChanged )
            break;
    }

    // Set the lighting shader if either GL_LIGHTING
    // or GL_LIGHTi changed state.
    if( lightChanged || lightEnableChanged )
    {
        // Always set the light enable uniform.
        // unroll out to BDFX_MAX_LIGHTS
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable0", isEnabled( GL_LIGHT0+0, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable1", isEnabled( GL_LIGHT0+1, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable2", isEnabled( GL_LIGHT0+2, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable3", isEnabled( GL_LIGHT0+3, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable4", isEnabled( GL_LIGHT0+4, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable5", isEnabled( GL_LIGHT0+5, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable6", isEnabled( GL_LIGHT0+6, currentState ) ? 1 : 0 ) );
		ss->addUniform( new osg::Uniform( "bdfx_lightEnable7", isEnabled( GL_LIGHT0+7, currentState ) ? 1 : 0 ) );

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
                shader = _vLightingLight0.get();
            else if( useSunOnly )
                shader = _vLightingSunOnly.get();
            else if( _supportSunLighting )
                shader = _vLightingSun.get();
            else
                shader = _vLightingOn.get();
        }
        else
            // Lighting is disabled.
            shader = _vLightingOff.get();

        osg::notify( osg::INFO ) << "Selected shader: " << shader->getName() << std::endl;

        if( smccb == NULL )
            smccb = getOrCreateShaderModuleCullCallback( group );
        smccb->setShader( getShaderSemantic( shader->getName() ), shader );
    }

    // Fog
    osg::StateAttribute::GLModeValue mode = ss->getMode( GL_FOG );
    if( mode & osg::StateAttribute::ON) // took out " || fogChange "
    {
        shader = _vFogOn.get();
        shaderTwo = _fFogOn.get();
    }
    else if( ( mode & osg::StateAttribute::INHERIT ) == 0 )
    {
        shader = _vFogOff.get();
        shaderTwo = _fFogOff.get();
    } // else if
    if( shader != NULL )
    {
        if( smccb == NULL )
            smccb = getOrCreateShaderModuleCullCallback( group );
        smccb->setShader( getShaderSemantic( shader->getName() ), shader );
        shader = NULL;
        if(shaderTwo)
            smccb->setShader( getShaderSemantic( shaderTwo->getName() ), shaderTwo );
    } // fog

    convertStateSet( ss );
}

void
ShaderModuleVisitor::convertStateSet( osg::StateSet* ss )
{
    osg::StateSet* currentState = ( _stateStack.empty() ) ? NULL : _stateStack.back().get();
    if(!ss) return;


    // Material
    osg::StateAttribute* sa;
    sa = ss->getAttribute( osg::StateAttribute::MATERIAL );
    osg::Material* mat = dynamic_cast< osg::Material* >( sa );
    if( mat != NULL )
    {
        osg::Uniform* colorMaterial;
        colorMaterial = new osg::Uniform( "bdfx_colorMaterial", 0 );
        ss->addUniform( colorMaterial );

        osg::Uniform* material;
        material = new osg::Uniform( "bdfx_frontMaterial.emissive",
            mat->getEmission( osg::Material::FRONT ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.ambient",
            mat->getAmbient( osg::Material::FRONT ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.diffuse",
            mat->getDiffuse( osg::Material::FRONT ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.specular",
            mat->getSpecular( osg::Material::FRONT ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.shininess",
            mat->getShininess( osg::Material::FRONT ) );
        ss->addUniform( material );

        material = new osg::Uniform( "bdfx_backMaterial.emissive",
            mat->getEmission( osg::Material::BACK ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.ambient",
            mat->getAmbient( osg::Material::BACK ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.diffuse",
            mat->getDiffuse( osg::Material::BACK ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.specular",
            mat->getSpecular( osg::Material::BACK ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.shininess",
            mat->getShininess( osg::Material::BACK ) );
        ss->addUniform( material );
    }

    // Normalize and rescale normal
    if( isSet( GL_NORMALIZE, ss ) || isSet( GL_RESCALE_NORMAL, ss ) )
    {
        const bool enabled = ( isEnabled( GL_NORMALIZE, currentState ) ||
            isEnabled( GL_RESCALE_NORMAL, currentState ) );

        osg::Uniform* normalize = new osg::Uniform( "bdfx_normalize", enabled ? 1 : 0 );
        ss->addUniform( normalize );
    }

    // Texture
	for(int texUnit = 0; texUnit < BDFX_MAX_TEXTURE_UNITS; texUnit++)
	{
		sa = ss->getTextureAttribute( texUnit, osg::StateAttribute::TEXTURE );
		osg::Texture2D* texture = dynamic_cast< osg::Texture2D* >( sa );
		char *samplerUnitNames[] = {"texture2dSampler0", "texture2dSampler1", "texture2dSampler2", "texture2dSampler3"}; // out to BDFX_MAX_TEXTURE_UNITS
		char *enableUnitNames[] = {"bdfx_texture2dEnable0", "bdfx_texture2dEnable1", "bdfx_texture2dEnable2", "bdfx_texture2dEnable3"}; // out to BDFX_MAX_TEXTURE_UNITS
		char *envUnitNames[] = {"bdfx_textureEnvMode0", "bdfx_textureEnvMode1", "bdfx_textureEnvMode2", "bdfx_textureEnvMode3"}; // out to BDFX_MAX_TEXTURE_UNITS

		ss->addUniform( new osg::Uniform( enableUnitNames[texUnit], texture == NULL ? 0 : 1 ) );

		if( texture != NULL )
		{
			ss->addUniform( new osg::Uniform( samplerUnitNames[texUnit], texUnit ) );

			osg::StateAttribute* texEnvModeSA = ss->getTextureAttribute( texUnit, osg::StateAttribute::TEXENV );
			osg::TexEnv* textureEnv = dynamic_cast< osg::TexEnv* >( texEnvModeSA );

			if(textureEnv)
			{
				ss->addUniform( new osg::Uniform( envUnitNames[texUnit], textureEnv->getMode() ) );
			} // if
		}
	} // for

    // TexGen
	// Code refactored to understand TexGen in the presence of multitexturing.
	// cf. http://www.opengl.org/sdk/docs/man/xhtml/glTexGen.xml
	// "When the ARB_multitexture extension is supported, glTexGen sets the texture generation parameters for the currently active texture unit, selected with glActiveTexture."

	osg::Vec4 planeS[BDFX_MAX_TEXTURE_COORDS], planeT[BDFX_MAX_TEXTURE_COORDS], planeR[BDFX_MAX_TEXTURE_COORDS], planeQ[BDFX_MAX_TEXTURE_COORDS];
	bool instantiateTXGUniforms(false);
	char *texgenUnitNames[] = {"bdfx_texGen0", "bdfx_texGen1", "bdfx_texGen2", "bdfx_texGen3"}; // out to BDFX_MAX_TEXTURE_UNITS

	for(int texCoordUnit = 0; texCoordUnit < BDFX_MAX_TEXTURE_COORDS; texCoordUnit++)
	{
		sa = ss->getTextureAttribute( texCoordUnit, osg::StateAttribute::TEXGEN );
		osg::TexGen* texgen = dynamic_cast< osg::TexGen* >( sa );
		if( texgen != NULL )
		{
			// flag that at least one texture unit is using TexGen, so load the uniforms
			instantiateTXGUniforms = true;
			// the bdfx_texGen uniform this will be used to set defaults to 0 (texgen off)
			ss->addUniform( new osg::Uniform( texgenUnitNames[texCoordUnit], texgen->getMode() ) );

			// setup per-mode uniforms for each texcoord unit
			switch(texgen->getMode())
			{
				// osg::TexGen shares the Plane members between object and eye linear
				case osg::TexGen::OBJECT_LINEAR:
					{
						// setup plane*
						planeS[texCoordUnit] = texgen->getPlane(osg::TexGen::S).asVec4();
						planeT[texCoordUnit] = texgen->getPlane(osg::TexGen::T).asVec4();
						planeR[texCoordUnit] = texgen->getPlane(osg::TexGen::R).asVec4();
						planeQ[texCoordUnit] = texgen->getPlane(osg::TexGen::Q).asVec4();
						break;
					} // object/eye linear
				case osg::TexGen::EYE_LINEAR:
					{
						// setup plane*
                        // Assumes usage of TexGenNode, which handles plane transform.
                        unsigned int unit = _eyePlanes._unit;
						planeS[ unit ] = _eyePlanes._planeS;
						planeT[ unit ] = _eyePlanes._planeT;
						planeR[ unit ] = _eyePlanes._planeR;
						planeQ[ unit ] = _eyePlanes._planeQ;

                        osg::Uniform* eyeAbsolute = new osg::Uniform( "bdfx_eyeAbsolute", _eyePlanes._absolute );
                        ss->addUniform( eyeAbsolute );
						break;
					} // object/eye linear
				default: break;
			} // switch mode
		} // if
		else
		{ // set the uniform to 0
			ss->addUniform( new osg::Uniform( texgenUnitNames[texCoordUnit], 0 ) );
		} // else
	} // for

	if(instantiateTXGUniforms) // defer this until now in case we don't need to
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
		for(int texCoordUnit = 0; texCoordUnit < BDFX_MAX_TEXTURE_COORDS; texCoordUnit++)
		{
			texGenEPlaneS->setElement(texCoordUnit, planeS[texCoordUnit]);
			texGenEPlaneT->setElement(texCoordUnit, planeT[texCoordUnit]);
			texGenEPlaneR->setElement(texCoordUnit, planeR[texCoordUnit]);
			texGenEPlaneQ->setElement(texCoordUnit, planeQ[texCoordUnit]);

			texGenOPlaneS->setElement(texCoordUnit, planeS[texCoordUnit]);
			texGenOPlaneT->setElement(texCoordUnit, planeT[texCoordUnit]);
			texGenOPlaneR->setElement(texCoordUnit, planeR[texCoordUnit]);
			texGenOPlaneQ->setElement(texCoordUnit, planeQ[texCoordUnit]);
		} // for

		// now add the populated Uniform to the StateSet
		ss->addUniform( texGenEPlaneS );
		ss->addUniform( texGenEPlaneT );
		ss->addUniform( texGenEPlaneR );
		ss->addUniform( texGenEPlaneQ );

		ss->addUniform( texGenOPlaneS );
		ss->addUniform( texGenOPlaneT );
		ss->addUniform( texGenOPlaneR );
		ss->addUniform( texGenOPlaneQ );

	} // if instantiateTXGUniforms

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

#if 0
    sa = ss->getAttribute( osg::StateAttribute::MATERIAL );
    osg::Material* mat = dynamic_cast< osg::Material* >( sa );
    if( mat != NULL )
    {
        ss->removeAttribute( osg::StateAttribute::MATERIAL );
    }
#endif

    // Fog
    sa = ss->getAttribute( osg::StateAttribute::FOG );
    osg::Fog* fog = dynamic_cast< osg::Fog* >( sa );
    if( fog != NULL )
    {
        osg::Uniform* fogUniform;

        fogUniform = new osg::Uniform( "bdfx_fog.mode",
            fog->getMode() );
        ss->addUniform( fogUniform );
        fogUniform = new osg::Uniform( "bdfx_fog.color",
            fog->getColor() );
        ss->addUniform( fogUniform );
        fogUniform = new osg::Uniform( "bdfx_fog.density",
            fog->getDensity() );
        ss->addUniform( fogUniform );
        fogUniform = new osg::Uniform( "bdfx_fog.start",
            fog->getStart() );
        ss->addUniform( fogUniform );
        fogUniform = new osg::Uniform( "bdfx_fog.end",
            fog->getEnd() );
        ss->addUniform( fogUniform );
        if(fog->getMode() == osg::Fog::LINEAR)
        {
            float fogScale = 1.0 / (fog->getEnd() - fog->getStart());
            fogUniform = new osg::Uniform( "bdfx_fog.scale", fogScale ); // don't make start==end or /0 will bite you
        } // if
        else
        {
        fogUniform = new osg::Uniform( "bdfx_fog.scale",
            0.0f ); // not used except in linear
        } // else
        ss->addUniform( fogUniform );
    }


    // Alpha uniform for depth peeling.
    {
        sa = ss->getAttribute( osg::StateAttribute::BLENDFUNC );
        osg::BlendFunc* bf = dynamic_cast< osg::BlendFunc* >( sa );
        if( bf != NULL )
        {
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
            osg::Uniform* alphaUniform;
            alphaUniform = new osg::Uniform( "bdfx_depthPeelAlpha.useAlpha", useAlpha );
            ss->addUniform( alphaUniform );
            alphaUniform = new osg::Uniform( "bdfx_depthPeelAlpha.alpha", alpha );
            ss->addUniform( alphaUniform );
        }
    }
}

std::string
ShaderModuleVisitor::elementName( const std::string& prefix, int element, const std::string& suffix )
{
    std::ostringstream ostr;
    ostr << prefix << element << suffix;
    return( ostr.str() );
}

void
ShaderModuleVisitor::setDefaults( osg::Node& node )
{
    ShaderModuleCullCallback* smccb = getOrCreateShaderModuleCullCallback( node );
    osg::StateSet* stateSet = node.getStateSet();

    const bool lightingOff = ( isSet( GL_LIGHTING, stateSet ) && !( isEnabled( GL_LIGHTING, stateSet ) ) );

    // Load default shaders.
    // Lighting is on by default in OSG.
    if( _attachMain )
        smccb->setShader( getShaderSemantic( _vMain->getName() ), _vMain.get() );
    smccb->setShader( getShaderSemantic( _vInit->getName() ), _vInit.get() );

    // Leave on globally, but really, we could disable this if
    // lighting and tex gen eye linear are disabled.
    smccb->setShader( getShaderSemantic( _vEyeCoordsOn->getName() ), _vEyeCoordsOn.get() );

    if( lightingOff )
    {
        smccb->setShader( getShaderSemantic( _vLightingOff->getName() ), _vLightingOff.get() );
    }
    else
    {
        if( _supportSunLighting )
            smccb->setShader( getShaderSemantic( _vLightingSun->getName() ), _vLightingSun.get() );
        else
            smccb->setShader( getShaderSemantic( _vLightingLight0->getName() ), _vLightingLight0.get() );
    }
    if( _attachTransform )
        smccb->setShader( getShaderSemantic( _vTransform->getName() ), _vTransform.get() );

    // fog defaults to off
    smccb->setShader( getShaderSemantic( _vFogOff->getName() ), _vFogOff.get() );
    smccb->setShader( getShaderSemantic( _fFogOff->getName() ), _fFogOff.get() );

    smccb->setShader( getShaderSemantic( _vFinalize->getName() ), _vFinalize.get() );


    if( _attachMain )
        smccb->setShader( getShaderSemantic( _fMain->getName() ), _fMain.get() );
    smccb->setShader( getShaderSemantic( _fInit->getName() ), _fInit.get() );
    smccb->setShader( getShaderSemantic( _fFinalize->getName() ), _fFinalize.get() );


    osg::StateSet* ss = node.getOrCreateStateSet();

    // Light enables.
    // OSG has GL_LIGHT0 on be default, even though it's off by
    // default in OpenGL.
    {
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable0", 1 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable1", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable2", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable3", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable4", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable5", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable6", 0 ) );
        ss->addUniform( new osg::Uniform( "bdfx_lightEnable7", 0 ) );
    }

    // Light sources.
    {
        int idx;
        for( idx=0; idx<8; idx++ )
        {
            osg::Uniform* light;

            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].ambient" ).c_str(), _lights[ idx ]._ambient );
            ss->addUniform( light );
            // Setting the diffuse and specular defaults to match src/osg/View.cpp setLightingMode().
            // These are not the OSG defaults fo a Light, and they are not the OpenGL defaults.
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].diffuse" ).c_str(), _lights[ idx ]._diffuse );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].specular" ).c_str(), _lights[ idx ]._specular );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].position" ).c_str(), _lights[ idx ]._position );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].halfVector" ).c_str(), _lights[ idx ]._halfVector );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].spotDirection" ).c_str(), _lights[ idx ]._spotDirection );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].spotExponent" ).c_str(), _lights[ idx ]._spotExponent );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].spotCutoff" ).c_str(), _lights[ idx ]._spotCutoff );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].spotCosCutoff" ).c_str(), cosf( _lights[ idx ]._spotCutoff ) );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].constantAttenuation" ).c_str(), _lights[ idx ]._constantAtt );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].linearAttenuation" ).c_str(), _lights[ idx ]._linearAtt );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].quadraticAttenuation" ).c_str(), _lights[ idx ]._quadAtt );
            ss->addUniform( light );
            light = new osg::Uniform( elementName( "bdfx_lightSource[", idx, "].absolute" ).c_str(), _lights[ idx ]._absolute );
            ss->addUniform( light );
        }
    }

    // Light model
    {
        osg::Uniform* lightModel;
        lightModel = new osg::Uniform( "bdfx_frontMaterial.ambient", osg::Vec4f( .2f, .2f, .2f, 1.f ) );
        ss->addUniform( lightModel );
        lightModel = new osg::Uniform( "bdfx_frontMaterial.localViewer", 0 );
        ss->addUniform( lightModel );
        lightModel = new osg::Uniform( "bdfx_frontMaterial.separateSpecular", 0 );
        ss->addUniform( lightModel );
        lightModel = new osg::Uniform( "bdfx_frontMaterial.twoSided", 0 );
        ss->addUniform( lightModel );
    }

    // Materials
    {
        osg::Uniform* material;
        material = new osg::Uniform( "bdfx_frontMaterial.emissive", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.ambient", osg::Vec4f( .2f, .2f, .2f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.diffuse", osg::Vec4f( .8f, .8f, .8f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.specular", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_frontMaterial.shininess", 0.f );
        ss->addUniform( material );

        material = new osg::Uniform( "bdfx_backMaterial.emissive", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.ambient", osg::Vec4f( .2f, .2f, .2f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.diffuse", osg::Vec4f( .8f, .8f, .8f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.specular", osg::Vec4f( 0.f, 0.f, 0.f, 1.f ) );
        ss->addUniform( material );
        material = new osg::Uniform( "bdfx_backMaterial.shininess", 0.f );
        ss->addUniform( material );
    }

    // Color material
    // Note OSG default is ON except when the scene graph specifies
    // a Meterial, then OSG turns it off. OpenGL default is OFF.
    {
        osg::Uniform* colorMaterial;
        colorMaterial = new osg::Uniform( "bdfx_colorMaterial", 1 );
        ss->addUniform( colorMaterial );
    }

    // Normalize.
    // Default is ON to match OSG behavior (OpenGL default is OFF).
    {
        osg::Uniform* normalize;
        normalize = new osg::Uniform( "bdfx_normalize", 1 );
        ss->addUniform( normalize );
    }

    // Texture
    {
		char *samplerUnitNames[] = {"texture2dSampler0", "texture2dSampler1", "texture2dSampler2", "texture2dSampler3"}; // out to BDFX_MAX_TEXTURE_UNITS
		char *enableUnitNames[] = {"bdfx_texture2dEnable0", "bdfx_texture2dEnable1", "bdfx_texture2dEnable2", "bdfx_texture2dEnable3"}; // out to BDFX_MAX_TEXTURE_UNITS
		char *envUnitNames[] = {"bdfx_textureEnvMode0", "bdfx_textureEnvMode1", "bdfx_textureEnvMode2", "bdfx_textureEnvMode3"}; // out to BDFX_MAX_TEXTURE_UNITS
		for(int texUnit = 0; texUnit < BDFX_MAX_TEXTURE_UNITS; texUnit++)
		{
			ss->addUniform( new osg::Uniform( samplerUnitNames[texUnit], texUnit ) );
			ss->addUniform( new osg::Uniform( enableUnitNames[texUnit], 0 ) );
			ss->addUniform( new osg::Uniform( envUnitNames[texUnit], 0 ) );
		} // for

	}

    // TexGen
    {
        osg::Uniform* texGenEnable = new osg::Uniform( osg::Uniform::INT, "bdfx_texGen", 4 );
        int enables[] = { 0, 0, 1, 0 };
        texGenEnable->setArray( new osg::IntArray( 4, enables ) );
        ss->addUniform( texGenEnable );
    }

    // Point sprite
    // When on, use gl_PointCoord in fragment shader to look up
    // texture values. When off, do the normal texture lookup.
    {
        osg::Uniform* pointSprite;
        pointSprite = new osg::Uniform( "bdfx_pointSprite", 0 );
        ss->addUniform( pointSprite );
    }

    // fog default uniforms
    {
        osg::Uniform* fog;
        fog = new osg::Uniform( "bdfx_fog.mode", GL_EXP ); // http://www.opengl.org/sdk/docs/man/xhtml/glFog.xml "The initial fog mode is GL_EXP."
        ss->addUniform( fog );
        fog = new osg::Uniform( "bdfx_fog.color", osg::Vec4f( 0.f, 0.f, 0.f, 0.f ) ); // http://www.opengl.org/sdk/docs/man/xhtml/glFog.xml
        ss->addUniform( fog );
        fog = new osg::Uniform( "bdfx_fog.density", 1.f ); // http://www.opengl.org/sdk/docs/man/xhtml/glFog.xml "The initial fog density is 1."
        ss->addUniform( fog );
        fog = new osg::Uniform( "bdfx_fog.start", 0.f ); // http://www.opengl.org/sdk/docs/man/xhtml/glFog.xml "The initial near distance is 0."
        ss->addUniform( fog );
        fog = new osg::Uniform( "bdfx_fog.end", 1.f ); // http://www.opengl.org/sdk/docs/man/xhtml/glFog.xml "The initial far distance is 1."
        ss->addUniform( fog );
        fog = new osg::Uniform( "bdfx_fog.scale", 0.0f ); // gl_Fog.scale is don't-care for !GL_LINEAR
        ss->addUniform( fog );
    } // for

    // Controls how depth peeling selects an alpha value.
    {
        osg::Uniform* alpha;
        alpha = new osg::Uniform( "bdfx_depthPeelAlpha.useAlpha", 0 );
        ss->addUniform( alpha );
        alpha = new osg::Uniform( "bdfx_depthPeelAlpha.alpha", 1.f );
        ss->addUniform( alpha );
    }
}

void
ShaderModuleVisitor::pushStateSet( osg::StateSet* ss )
{
    if( _stateStack.size() > 0 )
    {
        osg::StateSet* oldTop = _stateStack.back().get();
        osg::StateSet* newTop = new osg::StateSet( *oldTop );
        if( ss != NULL )
            newTop->merge( *ss );
        _stateStack.push_back( newTop );
    }
    else if( ss != NULL )
    {
        _stateStack.push_back( ss );
    }
}
void
ShaderModuleVisitor::popStateSet()
{
    if( _stateStack.size() > 0 )
        _stateStack.pop_back();
}


// namespace backdropFX
}
