#version 120


uniform mat4 shadowViewProj;

uniform mat4 osg_ViewMatrixInverse;

void main()
{
    mat4 modelMatrix = osg_ViewMatrixInverse * gl_ModelViewMatrix;
    gl_Position = shadowViewProj * modelMatrix * gl_Vertex;
}
