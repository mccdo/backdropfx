// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

uniform vec3 up;

varying vec3 oTC;

varying float dayCoeff, duskCoeff, nightCoeff;

void main()
{
    // No need to draw the sky that is below the ground.
    const float horizLimit = 0.15;
    float dotUp = dot( oTC, up );
    // Mix between the zenith and horizon colors.
    float horizonMixControl = clamp( dotUp, 0.0, horizLimit ) / horizLimit;

    // Day colors
    // (High altitude; taken from photos of sky in Glenwood Springs, Colorado.)
    const vec4 dayZenith = vec4( 0.10980, 0.25490, 0.56862, 1.0 );
    const vec4 dayHorizon = vec4( 0.41176, 0.52156, 0.70588, 1.0 );
    // Dusk/Dawn colors
    const vec4 duskZenith = vec4( 0.03680, 0.08496, 0.18954, 1.0 );
    const vec4 duskHorizon = vec4( 0.31176, 0.15215, 0.30588, 1.0 );
    // Night colors
    const vec4 nightZenith = dayZenith * .003;
    const vec4 nightHorizon = dayHorizon * .003;
    
    vec4 zenithColor = dayZenith * dayCoeff +
        duskZenith * duskCoeff + nightZenith * nightCoeff;
    vec4 horizonColor = dayHorizon * dayCoeff +
        duskHorizon * duskCoeff + nightHorizon * nightCoeff;
    
    // TBD GL3
    vec4 color = mix( horizonColor, zenithColor, horizonMixControl );
    if( dotUp < -horizLimit )
        color.a = 0.0; // discard using alpha test
    gl_FragColor = color;
}
