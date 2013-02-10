
BDFX INCLUDE shaders/effects/declarations.common

//This could be uniform
const float glowStrength = 6.0;

void main( void )
{
    vec3 base = texture2D( inputTexture0, oTC ).rgb;
    vec3 glow = texture2D( inputTexture1, oTC ).rgb;
    glow *= glowStrength;

    vec3 stencil = texture2D( inputTexture2, oTC ).rgb;
    float stencilGlowValue = clamp( length( stencil ), 0.0, 1.0 ) * 0.95;
    glow = ( 1.0 - stencilGlowValue ) * glow;

    gl_FragColor = vec4( base + glow, 1.0 );
}
