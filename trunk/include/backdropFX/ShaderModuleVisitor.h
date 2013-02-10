// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADER_MODULE_VISITOR_H__
#define __BACKDROPFX_SHADER_MODULE_VISITOR_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderLibraryConstants.h>
#include <osg/NodeVisitor>

#include <deque>
#include <vector>
#include <map>


// forward
namespace osg {
    class Material;
    class Texture2D;
    class TexEnv;
    class TexGen;
    class Fog;
    class BlendFunc;
}


namespace backdropFX {


// forward
class ShaderModuleCullCallback;


/** \class backdropFX::ShaderModuleVisitor ShaderModuleVisitor.h backdropFX/ShaderModuleVisitor.h

\brief Replaces FFP state in a scene graph with equivalent shader modules.

OpenGL FFP has a concept of "default state". However, there is no
analogous concept in GL3+. GL3+ has no default shader. To convert from
FFP to shader-based, this visitor inserts default shaders at the root
node of the visited scene graph. The defaults match the OSG defaults,
not the OpenGL defaults. For example, OSG enables lighting by default, 
so this visitor attaches a shader module that renders with lighting enabled.

After running this visitor, you must use the RebuildShaderModules visitor
to create complete programs from the shader modules.
*/
class BACKDROPFX_EXPORT ShaderModuleVisitor : public osg::NodeVisitor
{
public:
    ShaderModuleVisitor( osg::NodeVisitor::TraversalMode tm=TRAVERSE_ALL_CHILDREN );
    ~ShaderModuleVisitor();

    /** Control whether mergeDefaults() attaches main() vertex and
    fragment shader modules to the top level node.
    Default: Don't attach main (DepthPeel specifies main modules).
    */
    void setAttachMain( bool attachMain );
    /** Likewise for attaching a computeTransform function.
    Default: Don't attach transform (DepthPartition specifies a transform module).
    */
    void setAttachTransform( bool attachTransform );

    /** Control whether mergeDefaults inserts lighting shader modules 
    the support using the Sun as a light source. Default
    is false, but apps should set to true for use with backdropFX.

    NOTE: This function is only valid if setConvertSceneState is true.
    When false, this visitor doesn't add any lighting shaders to the
    scene graph.
    */
    void setSupportSunLighting( bool supportSunLighting );

    /** Control whether this visitor removes OSG StateAttributes and modes
    after it adds equivalent uniforms to support shader-based rendering.
    Default is true, because the OSG StateAttributes and modes are no
    longer necessary for correct rendering, and their presence will increase
    OSG draw time. Applications that require the original state information
    should set this to true. */
    void setRemoveFFPState( bool removeFFPState );

    /** If true, fog and light state are also converted to shader-based, though
    this is known to be a complex problem and there are limitations on SMV's
    ability to handle this correctly. This feature is disabled by default, and
    when used in the context of backdropFX, the Manager provides a top-level
    interface for controling fog and light state (as well as linking shadows to
    lights).

    Regardless of how setConvertSceneState is set, SMV still removes fog and
    light state if setRemoveFFPState is true. */
    void setConvertSceneState( bool convertSceneState );

    virtual void apply( osg::Node& node );
    virtual void apply( osg::Group& node );
    virtual void apply( osg::Geode& node );
    virtual void apply( osg::ClipNode& node );
    virtual void apply( osg::LightSource& node );
    virtual void apply( osg::TexGenNode& node );

    /** Convert a Group StateSet from FFP to shader-based.
    Because this is a Group, we can attach a ShaderModuleCullCallback
    and add shader modules. */
    void convertStateSet( osg::Group& node );
    /** Convert a StateSet from FFP to shader-based. Because we
    don't have the owning Group (StateSet belongs to a Drawable),
    it limits us to adding uniforms. */
    void convertStateSet( osg::StateSet* ss );

    /** Allows applications to set their own initial state. For example,
    if running SMV on a loaded model to be attached into a parent scene graph,
    use osgwTools::accumulateStateSets() to get the StateSet at the attachment
    point, then pass that to setInitialStateSet(). The visitor uses the iniotial
    state as the basis for default uniform values. */
    void setInitialStateSet( osg::StateSet* stateSet, const ShaderModuleCullCallback::ShaderMap& shaders );

    /** After this visitor completes traversal, the calling code should call this
    function to merge default uniforms and shader modules to the root node
    of the traversed scene graph. Thus function merges default state with existing
    state, and therefore doesn't overwrite existing uniforms with the same name or
    shader modules with the same semantic and type. */
    void mergeDefaults( osg::Node& node );

protected:
    bool _attachMain;
    bool _attachTransform;
    bool _supportSunLighting;
    bool _removeFFPState;
    bool _convertSceneState;

    osg::StateSet* _initialState;
    ShaderModuleCullCallback::ShaderMap* _initialShaders;
    osg::ref_ptr< ShaderModuleCullCallback > _initialSmccb;

    unsigned int _depth;

    osg::ref_ptr< osg::Shader > _vMain;
    osg::ref_ptr< osg::Shader > _vInit;
    osg::ref_ptr< osg::Shader > _vEyeCoordsOn;
    osg::ref_ptr< osg::Shader > _vEyeCoordsOff;
    osg::ref_ptr< osg::Shader > _vLightingOn;
    osg::ref_ptr< osg::Shader > _vLightingLight0;
    osg::ref_ptr< osg::Shader > _vLightingSun;
    osg::ref_ptr< osg::Shader > _vLightingSunOnly;
    osg::ref_ptr< osg::Shader > _vLightingOff;
    osg::ref_ptr< osg::Shader > _vTransform;
    osg::ref_ptr< osg::Shader > _vFogOn;
    osg::ref_ptr< osg::Shader > _vFogOff;
    osg::ref_ptr< osg::Shader > _vFinalize;

    osg::ref_ptr< osg::Shader > _fMain;
    osg::ref_ptr< osg::Shader > _fInit;
    osg::ref_ptr< osg::Shader > _fFinalize;
    osg::ref_ptr< osg::Shader > _fFogOn;
    osg::ref_ptr< osg::Shader > _fFogOff;



    // TBD psh
    struct LightSourceParameters
    {
        LightSourceParameters();
        osg::Vec4f _ambient;
        osg::Vec4f _diffuse;
        osg::Vec4f _specular;
        osg::Vec4f _position;
        osg::Vec3f _halfVector; // TBD not sure this is useful.
        osg::Vec3f _spotDirection;
        float _spotExponent;
        float _spotCutoff;
        float _constantAtt;
        float _linearAtt;
        float _quadAtt;
        float _absolute;
    };
    LightSourceParameters _lights[ BDFX_MAX_LIGHTS ];

    // TBD psh
    struct TexGenEyePlanes
    {
        TexGenEyePlanes();
        osg::Vec4f _planeS, _planeT, _planeR, _planeQ;
        int _absolute;
        unsigned int _unit;
    };
    TexGenEyePlanes _eyePlanes;

    std::deque< osg::ref_ptr< osg::StateSet > > _stateStack;

    bool isSet( GLenum stateItem, osg::StateSet* ss );
    bool isEnabled( GLenum stateItem, osg::StateSet* ss );
    bool isTextureSet( unsigned int unit, GLenum stateItem, osg::StateSet* ss );
    bool isTextureEnabled( unsigned int unit, GLenum stateItem, osg::StateSet* ss );


    static unsigned int LightOff;
    static unsigned int LightOn;
    static unsigned int LightSun;
    static unsigned int LightSunOnly;
    static unsigned int LightLight0;
    unsigned int _lightShaders;


    typedef std::vector< osg::ref_ptr< osg::Uniform > > UniformList;
    typedef std::map< GLenum, UniformList > ModeUniformMap;
    typedef std::map< osg::StateAttribute*, UniformList > AttributeUniformMap;

    ModeUniformMap _modeOnMap, _modeOffMap;
    AttributeUniformMap _attrMap;
    ModeUniformMap _texModeOnMap[ BDFX_MAX_TEXTURE_UNITS ], _texModeOffMap[ BDFX_MAX_TEXTURE_UNITS ];
    AttributeUniformMap _texAttrMap[ BDFX_MAX_TEXTURE_UNITS ];

    bool _usesLightSource[ BDFX_MAX_LIGHTS ];


    void convertMaterial( osg::Material* mat );
    void convertTexture( osg::Texture2D* tex, AttributeUniformMap& am );
    void convertTexEnv( osg::TexEnv* texEnv, unsigned int unit, AttributeUniformMap& am );
    void convertTexGen( osg::TexGen* texGen, unsigned int unit, AttributeUniformMap& am );
    void convertFog( osg::Fog* fog );
    void convertBlendFunc( osg::BlendFunc* bf, osg::StateSet* ss );

    void mergeDefaultsLighting( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet );
    void mergeDefaultsTexture( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet );
    void mergeDefaultsPointSprite( ShaderModuleCullCallback* smccb, osg::StateSet* stateSet );

    osg::Uniform* findUniform( const std::string& name, const osg::StateSet* stateSet );

    /** Used internally to track state during traversal. */
    void pushStateSet( osg::StateSet* ss );
    void popStateSet();
};


// namespace backdropFX
}


// __BACKDROPFX_SHADER_MODULE_VISITOR_H__
#endif
