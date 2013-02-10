
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

BDFX INCLUDE shaders/gl2/glow-declarations.common


// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/glow-transform.vs


void computeTransform()
{
    //gl_Position = bdfx_projectionMatrix() * bdfx_modelViewMatrix * bdfx_Vertex;

    // TBD Make these uniforms
    float focalRange = 30.;
    float focalDistance = 40.;

    // TBD need the eyecoords shader.
    vec4 ecPos = bdfx_modelViewMatrix * bdfx_vertex;
    gl_Position = bdfx_projectionMatrix * ecPos;

    // Use ABS of distance from focal point, we don't care if the
    // distance difference is negative. Just want absolute distance.
    // Clamp to range 0 to 1 to normalize; normalized distance >= 1.0 is fully blurred.
    // Negate ecPos.z (z is negative in eye coords, but we need a positive value).
    float normFocalDistance = min( abs( ( -ecPos.z - focalDistance ) / focalRange ), 1.0 );
    bdfx_normalizedDistanceToFocus = normFocalDistance;
}

// END gl2/glow-transform.vs

