
uniform sampler2D tex;
varying vec2 oTC;

uniform float brightness;
uniform float contrast;

vec4 brightnessContrast( in vec4 c )
{
    vec3 bColor = c.xyz + brightness;
    float contrastRange = 1.0 - contrast;
    float contrastMin = 0.5 - ( contrastRange * 0.5 );
    vec3 cColor = ( bColor - contrastMin ) / contrastRange;
    return( vec4( cColor, c.a ) );
}

void main()
{
    vec4 color = texture2D( tex, oTC );
    gl_FragData[0] = brightnessContrast( color );
}
