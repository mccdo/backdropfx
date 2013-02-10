#version 120

BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// effects/heat-texture.fs


varying vec2 offsetTC;

void main( void )
{
    vec4 normalizedDepthTexel = texture2D( inputTexture2, oTC );
    float offsetScale = .5 * normalizedDepthTexel.g;
    float offsetBias = offsetScale * -.5;

    vec4 noiseOffset = texture2D( inputTexture1, offsetTC ) * offsetScale + offsetBias;

    // We add the vanilla tex coord to the noise value to get a distortion
    // tex coord. However, if normalizedDepthTexel indicates a far distance,
    // and therefore lots of distortion, there's our distorted tex coord could
    // look up a foreground object. We account for that possibility here, by
    // removing the noiseOffset if we are about to sample into a foreground object.
    vec2 distortOffset = oTC + noiseOffset.st;
    vec4 distortedDepthTexel = texture2D( inputTexture2, distortOffset );
    if( distortedDepthTexel.g < normalizedDepthTexel.g )
        distortOffset = oTC;

    vec4 color = texture2D( inputTexture0, distortOffset );

    gl_FragColor = color;
}

// effects/heat-texture.fs


