
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/alphaBlend.fs


void main( void )
{
    // Image 0
    vec4 color0 = texture2D( inputTexture0, oTC );
    // Image 1
    vec4 color1 = texture2D( inputTexture1, oTC );
    // Alpha
    vec4 alpha = texture2D( inputTexture2, oTC );
    float aVal = alpha.r;
    
    // TBD in the future, support other blend operations. For now:
    // mix (as in "source alpha, one minus source alpha")
    vec3 blend = mix( color1.rgb, color0.rgb, aVal );

    gl_FragColor = vec4( blend, 1.0 );
}
