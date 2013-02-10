
BDFX INCLUDE shaders/gl2/ffp-declarations.common
BDFX INCLUDE shaders/gl2/ffp-declarations.vs

// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/ffp-transform.vs


void computeTransform()
{
    gl_Position = ftransform();
}

// END gl2/ffp-transform.vs

