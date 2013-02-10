#version 120

uniform sampler2D noise;
uniform sampler2D testTex;

uniform float osg_SimulationTime;

varying vec2 modTC;
varying vec2 nomodTC;
varying float scaleFactor;

void main( void )
{
    float offsetScale = .5 * scaleFactor;
    float offsetBias = offsetScale * -.5;
    vec4 noiseOffset = texture2D( noise, modTC ) * offsetScale + offsetBias;
    vec4 color = texture2D( testTex, nomodTC + noiseOffset.st );
    
    gl_FragColor = color;
}
