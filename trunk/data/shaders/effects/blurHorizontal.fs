
BDFX INCLUDE shaders/effects/declarations.common

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// effects/glow0a.fs


const float delta = 0.008;
const int numSamples = 8;
const float fNumSamples = float( numSamples );

float getIndex( in int n, in float x )
{
    float minValue = x - ( (( fNumSamples * 0.5 )-0.5) * delta );
    return( minValue + ( float(n) * delta ) );
}

void main( void )
{
    const float centerWeight = 2.;
    vec4 result = texture2D( inputTexture0, oTC ) * centerWeight;

    int idx;
    for( idx=0; idx<numSamples; idx++ )
    {
        result += texture2D( inputTexture0,
            vec2( getIndex( idx, oTC.x ), oTC.y ) );
    }

    gl_FragData[0] = result / ( fNumSamples + centerWeight );
}
