#version 120

BDFX INCLUDE shaders/gl2/bdfx-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-declarations.fs
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.fs

BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.common
BDFX INCLUDE shaders/gl2/bdfx-depthpeel-declarations.fs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-depthpeel-on.fs


void depthPeel()
{
    // TBD depth offset was NOT removed from OpenGL
    // so maybe there's a way we can use it instead
    // of trying to offset the z value of the depth
    // map texture coordinate.
    
    vec4 depthOffsetTC = vec4( ( bdfx_depthTC.xyz + bdfx_depthTC.w ) * 0.5, bdfx_depthTC.w );

    // Apply depth offset before depth compare
    const float r = bdfx_depthPeelOffsetR;
    float m = max( abs(dFdx( bdfx_depthTC.z )), abs(dFdy( bdfx_depthTC.z )) );

    // bdfx_depthPeelOffset contains 'factor' and 'units' in x and y.
    depthOffsetTC.z += ( bdfx_depthPeelOffset.x * r ) +
        ( bdfx_depthPeelOffset.y * m * r );

    if( bdfx_pointSprite == 1 )
    {
        // Additionally offset in s and t.
        // Default for point coord is origin upper left.
        vec2 pcOrient = vec2( gl_PointCoord.x, 1.0-gl_PointCoord.y );
        vec2 pcOffset = ( bdfx_depthTCBias * pcOrient ) - ( bdfx_depthTCBias * 0.5 );
        depthOffsetTC += vec4( pcOffset, 0.0, 0.0 );
    }

    // Incoming z value wins the depth test if it is greater than the depth buffer z value.
    // However, we want to discard it if it is not less than both the opaque map and
    // the previous layer's depth map.
    //
    // When rendering the opaque pass (to create the opaque map), the host code initializes
    // the opaque and previous layer maps to max z value (1.0, nmormalized). The host code
    // configures the depth test for GL_LESS.
    //
    // When rendering the first transparent layer, the host code initializes the previous 
    // layer map to max z value (1.0, normalized).
    
    // Depth peel compare fragment code
    vec4 opaqueResult = shadow2DProj( bdfx_depthPeelOpaqueDepthMap, depthOffsetTC );
    vec4 prevLayerResult = shadow2DProj( bdfx_depthPeelPreviousDepthMap, depthOffsetTC );
    if( ( opaqueResult.a == 0.0 ) || ( prevLayerResult.a == 0.0 ) )
    {
        bdfx_processedColor.a = 0.0; // discard using alpha test
    }
    else
    {
        // OpenGL blending supports source functions that use alpha from the primary color, but
        // also from BlendColor (CONSTANT_ALPHA, e.g.), so blending alpha can be different for
        // different geometry. However, we're going to blend the entire layer into the output buffer.
        // So how do we support different source functions in a single layer? We use host code
        // (ShaderModuleVisitor) to walk the scene graph and store the appropriate alpha value in
        // this uniform. If 'useAlpha' is 1, we probably have BlendColor, and the constant alpha value
        // gets stored in 'alpha'. This should all be configured in ShaderModuleVisitor.
        if( bdfx_depthPeelAlpha.useAlpha == 1 )
            bdfx_processedColor.a = bdfx_depthPeelAlpha.alpha;
    }
}

// END gl2/bdfx-depthpeel-on.fs

