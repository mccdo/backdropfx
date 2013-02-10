
uniform sampler2D tex;
varying vec2 oTC;

void main()
{
    vec4 color = texture2D( tex, oTC );
    gl_FragData[ 0 ] = color;
}
