// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Manager.h>
#include <backdropFX/Effect.h>
#include <backdropFX/EffectLibrary.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <osg/Program>
#include <backdropFX/Utils.h>

#include <string>
#include <sstream>




namespace backdropFX
{


osg::Program*
createEffectProgram( const std::string& baseName )
{
    std::string fileName;
    const std::string namePrefix( "shaders/effects/" );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".vs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > vertShader;
    __LOAD_SHADER( vertShader, osg::Shader::VERTEX, fileName );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".fs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > fragShader;
    __LOAD_SHADER( fragShader, osg::Shader::FRAGMENT, fileName );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->setName( "Effect " + baseName );
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );
    return( program.release() );
}




bool
configureGlowEffect( const bool enable )
{
    backdropFX::Manager& mgr = *( backdropFX::Manager::instance() );
    backdropFX::RenderingEffects& renderFX = mgr.getRenderingEffects();
    EffectVector& ev = renderFX.getEffectVector();
    osg::Camera& glowCamera = mgr.getGlowCamera();

    if( !enable )
    {
        // Disable the glow effect.

        bool deleted = renderFX.removeEffect(  "Glow-Blur" );
        if( !deleted )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: Glow-Blur not found." << std::endl;
            return( false );
        }
        deleted = renderFX.removeEffect(  "Glow-Combine" );
        if( !deleted )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: Glow-Combine not found." << std::endl;
            return( false );
        }

        // Disable the glow camera.
        // TBD In the future, the glow camera might also be generating the
        // DOF depth map, so we might not want to disable it.
        glowCamera.setNodeMask( 0 );
    }
    else
    {
        // Enable the glow effect.

        Effect* glowBlur = renderFX.getEffect( "Glow-Blur" );
        Effect* glowCombine = renderFX.getEffect( "Glow-Combine" );
        if( ( glowBlur != NULL ) || ( glowCombine != NULL ) )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: Glow already enabled." << std::endl;
            return( false );
        }

        glowCamera.setNodeMask( 0xffffffff );

        // Create and add the glow effect.
        glowBlur = new backdropFX::EffectSeperableBlur();
        UTIL_MEMORY_CHECK( glowBlur, "EffectLibraryUtil Glow EffectSeparableBlur", false );
        glowBlur->setName( "Glow-Blur" );
        glowBlur->addInput( 0, mgr.getColorBufferGlow() );

        unsigned int texW, texH;
        mgr.getTextureWidthHeight( texW, texH );
        glowBlur->setTextureWidthHeight( texW, texH );

        // TBD Need to be a litle smarter about the order of things,
        // once we start having more complex effect groupings.
        ev.push_back( glowBlur );

        glowCombine = new backdropFX::EffectCombine();
        UTIL_MEMORY_CHECK( glowBlur, "EffectLibraryUtil Glow EffectCombine", false );
        glowCombine->setName( "Glow-Combine" );
        glowCombine->addInput( 0, mgr.getColorBufferA() );
        glowBlur->attachOutputTo( glowCombine, 1 );

        mgr.getTextureWidthHeight( texW, texH );
        glowCombine->setTextureWidthHeight( texW, texH );

        ev.push_back( glowCombine );
    }

    // Return true to indicate success.
    return( true );
}


bool
configureDOFEffect( const bool enable )
{
    backdropFX::Manager& mgr = *( backdropFX::Manager::instance() );
    backdropFX::RenderingEffects& renderFX = mgr.getRenderingEffects();
    EffectVector& ev = renderFX.getEffectVector();
    osg::Camera& glowCamera = mgr.getGlowCamera();

    if( !enable )
    {
        // Disable the DOF effect.

        bool deleted = renderFX.removeEffect(  "DOF-Blur" );
        if( !deleted )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: DOF-Blur not found." << std::endl;
            return( false );
        }
        deleted = renderFX.removeEffect(  "DOF-AlphaBlend" );
        if( !deleted )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: DOF-AlphaBlend not found." << std::endl;
            return( false );
        }

        // Disable the glow camera.
        // TBD In the future, the glow camera might also be generating the
        // DOF depth map, so we might not want to disable it.
        glowCamera.setNodeMask( 0 );
    }
    else
    {
        // Enable the DOF effect.

        Effect* dofBlur = renderFX.getEffect( "DOF-Blur" );
        Effect* dofAlphaBlend = renderFX.getEffect( "DOF-AlphaBlend" );
        if( ( dofBlur != NULL ) || ( dofAlphaBlend != NULL ) )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: DOF already enabled." << std::endl;
            return( false );
        }

        glowCamera.setNodeMask( 0xffffffff );

        // Create and add the DOF effect.
        dofBlur = new backdropFX::EffectSeperableBlur();
        UTIL_MEMORY_CHECK( dofBlur, "EffectLibraryUtil DOF EffectSeparableBlur", false );
        dofBlur->setName( "DOF-Blur" );
        dofBlur->addInput( 0, mgr.getColorBufferA() );

        unsigned int texW, texH;
        mgr.getTextureWidthHeight( texW, texH );
        dofBlur->setTextureWidthHeight( texW, texH );

        // TBD Need to be a litle smarter about the order of things,
        // once we start having more complex effect groupings.
        ev.push_back( dofBlur );

        dofAlphaBlend = new backdropFX::EffectAlphaBlend();
        UTIL_MEMORY_CHECK( dofAlphaBlend, "EffectLibraryUtil DOF EffectAlphaBlend", false );
        dofAlphaBlend->setName( "DOF-AlphaBlend" );
        // Texture 0 is the blurred image. When alpha is 1.0, this image will be displayed.
        dofBlur->attachOutputTo( dofAlphaBlend, 0 );
        // Texture 1 is the sharp image. When alpha is 0.0, this image will be displayed.
        dofAlphaBlend->addInput( 1, mgr.getColorBufferA() );
        // Map of normalized focal distance values, 0.0 (for in-foxus) to 1.0 (for out-of-focus).
        dofAlphaBlend->addInput( 2, mgr.getDepthBuffer() );

        mgr.getTextureWidthHeight( texW, texH );
        dofAlphaBlend->setTextureWidthHeight( texW, texH );

        ev.push_back( dofAlphaBlend );
    }

    // Return true to indicate success.
    return( true );
}


// namespace backdropFX
}
