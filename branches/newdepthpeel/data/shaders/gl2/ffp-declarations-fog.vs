
// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-declarations-fog.vs


//
// Input attributes.
// Use #define so that we can use consistent internal names
// to minimize code changes between GL2 and GL3.
#define bdfx_fogCoord gl_FogCoord // not supported yet


//
// Functions
void computeFogVertex();

// END gl2/ffp-declarations-fog.vs

