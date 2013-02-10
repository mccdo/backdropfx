
uniform sampler2D tex;

varying vec2 oTC;

void main()
{
    gl_FragData[0] = texture2D( tex, oTC );
    //gl_FragData[0] = vec4( 1., 0., 0., 1. );
}
