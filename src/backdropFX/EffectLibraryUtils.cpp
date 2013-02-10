// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Manager.h>
#include <backdropFX/Effect.h>
#include <backdropFX/EffectLibrary.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <osg/Program>
#include <backdropFX/Utils.h>

#include <string>
#include <sstream>




namespace backdropFX
{


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

        bool deleted = renderFX.removeEffect(  "EffectGlow" );
        if( !deleted )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: EffectGlow not found." << std::endl;
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

        Effect* glowEffect = renderFX.getEffect( "EffectGlow" );
        if( glowEffect != NULL )
        {
            osg::notify( osg::WARN ) << "backdropFX: EffectLibraryUtils: EffectGlow already enabled." << std::endl;
            return( false );
        }

        glowCamera.setNodeMask( 0xffffffff );

        // Create and add the glow effect.
        EffectGlow* glow = new EffectGlow;
        UTIL_MEMORY_CHECK( glow, "EffectLibraryUtil adding EffectGlow", false )
        glow->setName( "EffectGlow" );
        glow->addInput( 0, mgr.getColorBufferA() );
        glow->addInput( 1, mgr.getColorBufferGlow() );
        glow->setProgram( createEffectProgram( "glow1" ) );

        // TBD Need to be a litle smarter about the order of things,
        // once we start having more complex effect groupings.
        ev.push_back( glow );
    }

    // Return true to indicate success.
    return( true );
}


// namespace backdropFX
}
