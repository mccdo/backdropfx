// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/DepthPeelUtils.h>
#include <backdropFX/DepthPeelBin.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/Utils.h>
#include <osg/AlphaFunc>


namespace backdropFX
{


void configureAsDepthPeel( osg::Group* group )
{
    // Sets required shader modules
    // Sets blending OFF | OVERRIDE
    // AlphaFunc passes if alpha > 0.0. Set to ON | OVERRIDE.
    // Specifies default values for depth peel uniforms:
    //  - opaque depth map texture unit
    //  - previous layer depth map texture unit
    //  - depth offset values
    //  - depth peeling enabled

    osg::StateSet* stateSet = group->getOrCreateStateSet();
    stateSet->setName( "DepthPeel" );
    stateSet->setRenderBinDetails( 0, "DepthPeelBin" );

    backdropFX::ShaderModuleCullCallback* smccb = backdropFX::getOrCreateShaderModuleCullCallback( *group );
    UTIL_MEMORY_CHECK( smccb, "DepthPeel SMCCB",  );

    // For depth peeling, use these shaders:
    //   bdfx-main.vs
    //   bdfx-main.fs
    //   bdfx-depthpeel.fs
    osg::ref_ptr< osg::Shader > shader;
    std::string fileName( "shaders/gl2/bdfx-main.vs" );
    __LOAD_SHADER( shader, osg::Shader::VERTEX, fileName );
    UTIL_MEMORY_CHECK( shader, "DepthPeel bdfx-main.vs",  );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );

    fileName = "shaders/gl2/bdfx-main.fs";
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "DepthPeel bdfx-main.fs",  );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );

    fileName = "shaders/gl2/bdfx-depthpeel.fs";
    __LOAD_SHADER( shader, osg::Shader::FRAGMENT, fileName );
    UTIL_MEMORY_CHECK( shader, "DepthPeel bdfx-depthpeel-on.fs",  );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader.get() );

    // When creating each layer, do not blend. Just write the alpha value into the color buffer.
    // When the FSTP is rendered to the output buffer, it will use blending at that time.
    stateSet->setMode( GL_BLEND, osg::StateAttribute::OFF |
        osg::StateAttribute::OVERRIDE );

    osg::AlphaFunc* af = new osg::AlphaFunc( osg::AlphaFunc::GREATER, 0.f );
    UTIL_MEMORY_CHECK( shader, "DepthPeel AlphaFunc",  );
    stateSet->setAttributeAndModes( af, osg::StateAttribute::ON |
        osg::StateAttribute::OVERRIDE );


    // Default depth peel shader module uniforms

    // Texture units for the depth maps.
    GLuint textureUnit = backdropFX::DepthPeelBin::getTextureUnit();
    osg::Uniform* depthMap;
    depthMap = new osg::Uniform( "bdfx_depthPeelOpaqueDepthMap", (int)( textureUnit ) );
    UTIL_MEMORY_CHECK( depthMap, "DepthPeel internalInit depthMap uniform",  );
    stateSet->addUniform( depthMap );
    depthMap = new osg::Uniform( "bdfx_depthPeelPreviousDepthMap", (int)( textureUnit+1 ) );
    UTIL_MEMORY_CHECK( depthMap, "DepthPeel internalInit depthMap uniform",  );
    stateSet->addUniform( depthMap );

    // Offset z lookup values using a depth offset variant.
    // Values are very sensitive. At one point, this was set to -0.01f, -10.f,
    // which resulted in pretty good visual appearance except for axis-aligned
    // cases where little or no offset would occur (causing incorrect rendering
    // and very poor performance). -2.f, -2.f should suffice for now.
    // Switched to 2.f, 2.f to support GEQUAL depth func and GL_LESS depth map compare.
    osg::Uniform* depthOffset;
    depthOffset = new osg::Uniform( "bdfx_depthPeelOffset",
        osg::Vec2f( 2.f, 2.f ) );
    UTIL_MEMORY_CHECK( depthOffset, "DepthPeel internalInit depthOffset uniform",  );
    stateSet->addUniform( depthOffset );

    // Depth peel shader module is always available, but bdfx-main.fs
    // conditionally calls it based on this uniform.
    osg::Uniform* depthPeelEnable;
    depthPeelEnable = new osg::Uniform( "bdfx_depthPeelEnable", 1 );
    UTIL_MEMORY_CHECK( depthPeelEnable, "DepthPeel depthPeelEnable uniform",  );
    stateSet->addUniform( depthPeelEnable );
}

void depthPeelEnable( osg::Group* group )
{
    osg::StateSet* stateSet = group->getOrCreateStateSet();
    stateSet->setRenderBinDetails( 0, "DepthPeelBin" );

    stateSet->setMode( GL_BLEND, osg::StateAttribute::OFF |
        osg::StateAttribute::OVERRIDE );

    stateSet->setMode( GL_ALPHA_TEST, osg::StateAttribute::ON |
        osg::StateAttribute::OVERRIDE );

    osg::Uniform* depthPeelEnable = stateSet->getUniform( "bdfx_depthPeelEnable" );
    if( depthPeelEnable != NULL )
        depthPeelEnable->set( 1 );
    else
        osg::notify( osg::WARN ) << "BDFX: depthPeelEnable could not find bdfx_depthPeelEnable uniform." << std::endl;
}
void depthPeelDisable( osg::Group* group )
{
    osg::StateSet* stateSet = group->getOrCreateStateSet();
    stateSet->setRenderBinMode( osg::StateSet::INHERIT_RENDERBIN_DETAILS );

    stateSet->removeMode( GL_BLEND );

    stateSet->setMode( GL_ALPHA_TEST, osg::StateAttribute::OFF );

    osg::Uniform* depthPeelEnable = stateSet->getUniform( "bdfx_depthPeelEnable" );
    if( depthPeelEnable != NULL )
        depthPeelEnable->set( 0 );
    else
        osg::notify( osg::WARN ) << "BDFX: depthPeelDisable could not find bdfx_depthPeelEnable uniform." << std::endl;
}


// backdropFX
}
