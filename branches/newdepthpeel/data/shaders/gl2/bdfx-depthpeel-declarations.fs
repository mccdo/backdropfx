
// Copyright (c) 2010 Skew Matrix Software. All rights reserved.
// gl2/bdfx-depthpeel-declarations.fs


// Toggle depth peeling on (1) or off (0).
uniform int bdfx_depthPeelEnable;

// TBD. Hm. Seems to work best if we treat the depth buffer as 16-bit...
// Maybe need to make this a host-configurable value.
const float bdfx_depthPeelOffsetR = 0.0000152590218966964; // 1.0 / (float) 0xffff
//const float bdfx_depthPeelOffsetR = 0.0000000596046483281; // 1.0 / (float) 0xffffff

uniform vec2 bdfx_depthPeelOffset;

uniform sampler2DShadow bdfx_depthPeelPreviousDepthMap;
uniform sampler2DShadow bdfx_depthPeelOpaqueDepthMap;

// Alpha control. We do not blend as we create each depth peel layer, but
// we do write the alpha value into the RGBA color buffer. (Blending is done
// when that layer is combined with the output buffer.) Unfortunately, fragment
// shader code can't access OpenGL blending state to determine the alpha value
// to write. ShaderModuleVisitor sets this uniform based on scene graph state.
//    if 'useAlpha' is 1, bdfx_processedColor.a is set to 'alpha'.
//    otherwise, bdfx_processedColor.a is unmodified.
struct bdfx_depthPeelAlphaParameters {
    int useAlpha;
    float alpha;
};
uniform bdfx_depthPeelAlphaParameters bdfx_depthPeelAlpha;


// END gl2/bdfx-depthpeel-declarations.fs

