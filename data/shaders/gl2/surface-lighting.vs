
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2011 Skew Matrix Software. All rights reserved.
// gl2/surface-lighting.vs


// TBD needs to change to support multiple lights.
// TBD needs to use bdfx lighting interface.
uniform vec3 fvLightPosition;

// TBD needs to go in .common file for access by frag shader.
varying vec3 LightDirection;

// TBD how to do generic vertex attributes with shader modules?   
attribute vec3 rm_Binormal;
attribute vec3 rm_Tangent;

void computeLighting( void )
{
    bdfx_outTexCoord0 = bdfx_multiTexCoord0;

    vec4 vpli = osg_ViewMatrix * bdfx_lightSource[ 0 ].position;
    vec3 fvLightDirection = vpli.xyz - bdfx_eyeVertex.xyz;
     
    vec3 fvNormal         = bdfx_eyeNormal;
    vec3 fvBinormal       = bdfx_normalMatrix * rm_Binormal;
    vec3 fvTangent        = bdfx_normalMatrix * rm_Tangent;
      
    LightDirection.x  = dot( fvTangent, fvLightDirection.xyz );
    LightDirection.y  = dot( fvBinormal, fvLightDirection.xyz );
    LightDirection.z  = dot( fvNormal, fvLightDirection.xyz );
}

// END gl2/surface-lighting.vs

