
varying vec4 oColor;

vec4 computeLight( vec4 color, vec3 normal );

void main( void )
{
    vec3 nPrime = normalize( gl_NormalMatrix * gl_Normal );
    oColor = computeLight( gl_Color, nPrime );

    gl_Position = ftransform();
}
