// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADER_MODULE_VISITOR_H__
#define __BACKDROPFX_SHADER_MODULE_VISITOR_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/ShaderLibraryConstants.h>
#include <osg/NodeVisitor>

#include <deque>


namespace backdropFX {


/** Replaces FFP state in a scene graph with equivalent shader modules.

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

    /** Control adding default uniforms and shader modules to the root of the visited scene graph.
    Default is true. The default shader modules to add can be further refined with the
    setAdttachMain(), setAttachTransform(), and setSupportSunLighting() calls. When set to
    false, it ignores setAdttachMain(), setAttachTransform(), and setSupportSunLighting(), as
    it adds no default shader modules to the root of the visited scene graph.

    An application typically setAddDefaults to true when running the visitor on its main
    scene graph, but set it to false when visiting a subgraph loaded at runtime
    and added as a child to the main scene graph.
    */
    void setAddDefaults( bool addDefaults );
    /** Control whether this visitor attaches main() vertex and
    fragment shader modules to the top level node.
    Default: Don't attach main (DepthPeel specifies main modules).
    Note: The visitor ignores this call if setAddDefaults is false. */
    void setAttachMain( bool attachMain );
    /** Likewise for attaching a computeTransform function.
    Default: Don't attach transform (DepthPartition specifies a transform module).
    Note: The visitor ignores this call if setAddDefaults is false. */
    void setAttachTransform( bool attachTransform );
    /** Control whether this visitor inserts lighting shader modules 
    the support using the Sun as a light source. Default
    is false, but apps should set to true for use with backdropFX.
    Note: The visitor ignores this call if setAddDefaults is false. */
    void setSupportSunLighting( bool supportSunLighting );

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
    void convertStateSet( osg::Node& node );
    /** Convert a StateSet from FFP to shader-based. Because we
    don't have the owning Group (StateSet belongs to a Drawable)
    it limits us to adding uniforms. */
    void convertStateSet( osg::StateSet* ss );

    void setDefaults( osg::Node& node );

    /** Used internally to track state during traversal. This is a
    public method so that apps can specify their own default state. */
    void pushStateSet( osg::StateSet* ss );
    void popStateSet();

protected:
    std::string elementName( const std::string& prefix, int element, const std::string& suffix );

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

    bool _addDefaults;
    bool _attachMain;
    bool _attachTransform;
    bool _supportSunLighting;

    unsigned int _depth;


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
        int _absolute;
    };

    LightSourceParameters _lights[ BDFX_MAX_LIGHTS ];

    std::deque< osg::ref_ptr< osg::StateSet > > _stateStack;

    bool isSet( GLenum stateItem, osg::StateSet* ss );
    bool isEnabled( GLenum stateItem, osg::StateSet* ss );
};


// namespace backdropFX
}


// __BACKDROPFX_SHADER_MODULE_VISITOR_H__
#endif
