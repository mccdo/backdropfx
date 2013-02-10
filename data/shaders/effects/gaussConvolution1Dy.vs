
BDFX INCLUDE shaders/effects/declarations.common

uniform vec2 viewportSize;

varying vec4 TC[ 2 ];

void main()
{
    oTC = ( gl_Vertex.xy + 1.0 ) * 0.5;
    oTC *= texturePercent;

    float texelIncrement = 1.0 / viewportSize.t;
    TC[ 0 ] = vec4( oTC.t + texelIncrement * 1.0,
                    oTC.t - texelIncrement * 1.0,
                    oTC.t + texelIncrement * 2.0,
                    oTC.t - texelIncrement * 2.0 );
    TC[ 1 ] = vec4( oTC.t + texelIncrement * 3.0,
                    oTC.t - texelIncrement * 3.0,
                    oTC.t + texelIncrement * 4.0,
                    oTC.t - texelIncrement * 4.0 );

    gl_Position = gl_Vertex;
}
