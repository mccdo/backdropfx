#version 120

uniform float osg_SimulationTime;

varying vec2 modTC;
varying vec2 nomodTC;
varying float scaleFactor;

void main( void )
{
    // Starts at 1 (full effect), ends at 0 (no effect).
    scaleFactor = max( 0., ( 1. - ( osg_SimulationTime / 5. ) ) );
    scaleFactor = .2;

    float s = gl_MultiTexCoord0.s;
    float t = gl_MultiTexCoord0.t - ( osg_SimulationTime * .35 );
    modTC = vec2( s, t );
    nomodTC = gl_MultiTexCoord0.st;

    //gl_Position = gl_Vertex;
    gl_Position = ftransform();
}
