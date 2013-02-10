
// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-declarations.vs


//
// Input attributes.
// Use #define so that we can use consistent internal names
// to minimize code changes between GL2 and GL3.
#define bdfx_vertex gl_Vertex
#define bdfx_normal gl_Normal
#define bdfx_color gl_Color
#define bdfx_secondaryColor gl_SecondaryColor
#define bdfx_multiTexCoord0 gl_MultiTexCoord0
#define bdfx_multiTexCoord1 gl_MultiTexCoord1
#define bdfx_multiTexCoord2 gl_MultiTexCoord2
#define bdfx_multiTexCoord3 gl_MultiTexCoord3


//
// Commonly used intermediate values
vec4 bdfx_eyeVertex;
vec3 bdfx_eyeNormal;
vec4 bdfx_processedColor;



//
// Functions
void init();
void computeEyeCoords();
void computeLighting();
void computeTransform();
void finalize();

// END gl2/ffp-declarations.vs

