
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/glow0a.fs


const float delta = 0.008;
const int numSamples = 8;
const float fNumSamples = numSamples;

float getIndex( in float n, in float x )
{
    float minValue = x - ( (( fNumSamples * 0.5 )-0.5) * delta );
    return( minValue + (n * delta) );
}

void main( void )
{
    const float centerWeight = 1.;
    vec4 result = texture2D( inputTextures[1], oTC ) * centerWeight;

    int idx;
    for( idx=0; idx<numSamples; idx++ )
    {
        result += texture2D( inputTextures[0],
            vec2( getIndex( idx, oTC.x ), oTC.y ) );
    }

    gl_FragData[0] = ( result * 1.2 ) / ( fNumSamples * centerWeight );
}
