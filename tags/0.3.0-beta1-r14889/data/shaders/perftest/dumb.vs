
varying vec4 dumb_outColor;

void main()
{
    gl_Position = ftransform();//gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    dumb_outColor = gl_Color;
}
