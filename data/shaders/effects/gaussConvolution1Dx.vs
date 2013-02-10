
BDFX INCLUDE shaders/effects/declarations.common

uniform vec2 viewportSize;

varying vec4 TC[ 2 ];

void main()
{
    oTC = ( gl_Vertex.xy + 1.0 ) * 0.5;
    oTC *= texturePercent;

    float texelIncrement = 1.0 / ( viewportSize.s );
    TC[ 0 ] = vec4( oTC.s + texelIncrement * 1.0,
                    oTC.s - texelIncrement * 1.0,
                    oTC.s + texelIncrement * 2.0,
                    oTC.s - texelIncrement * 2.0 );
    TC[ 1 ] = vec4( oTC.s + texelIncrement * 3.0,
                    oTC.s - texelIncrement * 3.0,
                    oTC.s + texelIncrement * 4.0,
                    oTC.s - texelIncrement * 4.0 );

    gl_Position = gl_Vertex;
}
