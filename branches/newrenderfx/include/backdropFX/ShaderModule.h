// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADER_MODULE_H__
#define __BACKDROPFX_SHADER_MODULE_H__ 1

#include <backdropFX/Export.h>
#include <osg/StateAttribute>
#include <osg/Shader>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Node>
#include <OpenThreads/Mutex>

#include <string>
#include <vector>
#include <map>


namespace backdropFX {


class ShaderModule;

/** \class backdropFX::ShaderModuleCullCallback ShaderModuleCullCallback.h backdropFX/ShaderModuleCullCallback.h

\brief Stores individual shader modules on a Node, and specifies complete programs during cull.

At init time, store individual shader modules in a Node's ShaderModuleCullCallback.
A convenience routine, getOrCreateShaderModuleCullCallback(), creates a
ShaderModuleCullCallback if one doesn't exist.

Any time you change the shader modules in a ShaderModuleCullCallback, you should run
the RebuildShaderModules visitor on your scene graph to create complete programs from
shader modules.
*/
class BACKDROPFX_EXPORT ShaderModuleCullCallback : public osg::NodeCallback
{
public:
    ShaderModuleCullCallback();
    ShaderModuleCullCallback( const ShaderModuleCullCallback& smccb );

    /** Called during cull (by the CullVisitor). Uses the current NodePath as a key into the _stateSetMap
    to look up a StateSet containing the program for the given NodePath. This function
    then pushes the StateSet, traverses the subgraph, and pops the StateSet. */
    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv );

    /** Inheritance control bits. */
    static const unsigned int InheritanceDefault;
    static const unsigned int InheritanceOverride;

    /** Add a shader with the given semantic and inheritance control.
    \param shaderSemantic A key or type for the given shader module, "lighting", for example.
    \param shader Shader to add.
    \param inheritance Default if absent is normal inheritance. Specify \c InheritanceOverride
    to override child Node shader modules with the same semantic and shader type (VERTEX, FRAGMENT, etc). */
    void setShader( const std::string& shaderSemantic, osg::Shader* shader, const unsigned int inheritance = InheritanceDefault );
    osg::Shader* getShader( const std::string& shaderSemantic, osg::Shader::Type type );
    unsigned int getShaderInheritance( const std::string& shaderSemantic, osg::Shader::Type type ) const;

    /** Remove a shader module.
    If the shader is found, remove it and return true. Otherwise, return false. */
    bool removeShader( const std::string& shaderSemantic, osg::Shader::Type type );

    /** Inserts the specified StateSet into the _stateSetMap, using the given NodePath
    as a key. This function is public to allow calls from RebuildShaderModules,
    but it is not intended for direct application use. */
    void insertStateSet( osg::NodePath& np, osg::StateSet* ss );
    /** Called by RebuildShaderModules to start from a clean slate and generate new
    programs from shader modules. */
    void clearStateSetMap();


    typedef std::pair< std::string, osg::Shader::Type > ShaderKey;
    typedef std::map< ShaderKey, osg::ref_ptr< osg::Shader > > ShaderMap;
    typedef std::map< ShaderKey, unsigned int > InheritanceMap;

    typedef std::map< osg::NodePath, osg::ref_ptr< osg::StateSet > > StateSetMap;

protected:
    ~ShaderModuleCullCallback();

    ShaderMap _shaderMap;
    InheritanceMap _inheritanceMap;
    StateSetMap _stateSetMap;

    // TBD This allows RebuildShaderModules to
    // directly access _shaderMap and _inheritanceMap.
    // Need a cleaner way to do this.
    friend class RebuildShaderModules;
};



/** \brief Collects individual shader modules in a scene graph and creates entire programs.

\bold You must run this visitor on your scene graph any time you change, add, or remove
a shader. \endbold When using this visitor with backdropFX managed classes,
you should run the visitor on the managed root instead of only on
your scene data.

Note, it might not be possible for RebuildShaderModules to create a complete program.
Consider: Node A contains shader modules that do not create a complete program, but has a child 
Node B that contains other shader modules. When these modules combine with the modules from
parent Node A, they make a complete program. RebuildShaderModules \b can create a complete program
at Node B, but not at Node A. This is fine and doesn't cause an error, unless, for example,
Node A has another child, a Geode, that tries to render with the incomplete program.
*/
class BACKDROPFX_EXPORT RebuildShaderModules : public osg::NodeVisitor
{
public:
    RebuildShaderModules( osg::NodeVisitor::TraversalMode tm=TRAVERSE_ALL_CHILDREN );
    ~RebuildShaderModules();

    void reset();

    virtual void apply( osg::Node& node );

protected:
    typedef std::vector< osg::Shader* > ShaderList;
    typedef std::map< ShaderList, osg::ref_ptr< osg::StateSet > > ShaderStateSetMap;
    ShaderStateSetMap _shaderStateSetMap;

    unsigned int _depth;

    void rebuildSource( ShaderModuleCullCallback* smccb, osg::NodePath& np );
};


/** \brief Removes all ShaderModuleCullCallback instances from a scene graph.

This can be a useful benchmarking and performance tool, because it lets developers 
compare rendering times of a scene graph (using shader modules) against a scene graph
(using no shader modules).
*/
class BACKDROPFX_EXPORT RemoveShaderModules : public osg::NodeVisitor
{
public:
    RemoveShaderModules( osg::NodeVisitor::TraversalMode tm=TRAVERSE_ALL_CHILDREN );
    ~RemoveShaderModules();

    virtual void apply( osg::Node& node );

protected:
};


/** \page shadermodules The backdropFX Shader Composition System

\section sm-intro Overview

The shader module system lets developers specify shader functionality
in separately compiled shader modules, which are linked together to form
complete shader programs. The system uses the osg::Shader class to store
the source for each shader module. The ShaderModuleCullCallback
class lets developers associate one or more shader modules with any
Group node in the scene graph. The RebuildShaderModules visitor groups
shader modules into complete osg::Programs. During cull, the
ShaderModuleCullCallback uses the CullVisitor NodePath as a key to look up
a StateSet containing the appropriate Program, pushes it onto the stack,
continues traversal, and pops it.

\subsection sm-limitations Shader Module System Limitations

The shader module system is based on a cull callback, because the complete
shader program for a given node might be different depending on which shader
modules are attached to multiple parents. To say it another way, the complete
shader program for a node is based on the NodePath. The NodePath differs in
the presence of multiple parents, which can result in different complete programs
for a given node. The cull callback approach handles this elegantly by using the
CullVisitor NodePath as a key to look up the complete program, then pushing and popping it
around a \c traverse() call.

Unfortunately, using a cull callback to push and pop state implies that
shader modules can attach only to Group or Group-derived nodes and not to
Geodes or Drawables. (Geodes traverse their Drawables after execution of the cull
callback, which means the Geode's complete program is not in effect for
child Drawable rendering. This is arguably a bug in the OSG CullVisitor, and 
possibly a custom Geode could alleviate it.)

For this reason, scene graphs that change state per-Geode or per-Drawable 
might not be well-suited to the shader module system. There are a few workarounds 
and solutions you can pursue:

\li Write shader modules in a way that the program doesn't need to change
per-Geode or per-Drawable. For example, if a Drawable StateSet disables lighting, attach
a lighting shader module to a parent Group that supports both lighting on and off,
then use a uniform per-Drawable to toggle the state. (Note the ShaderModuleVisitor
would need to detect toggling state per-Drawable, so it can attach the appropriate
general-purpose shader to the parent Group.)

\li Reorganize the scene graph with a preprocessor to eliminate state per-Geode
and per-Drawable. After reorganization, run the ShaderModuleVisit as usual.

\subsection sm-sem Semantic Strings

When adding a shader module (osg::Shader) to a ShaderModuleCullCallback,
you must specify a semantic string. Think of the semantic as a slot for
each shader module descriptive of its functionality, such
as lighting, init, transform, depthpeel, and main.
RebuildShaderModules uses the semantic to control how shader modules inherit
during traversal. By default, a child node shader module overrides a parent
shader module with the same semantic (and shader type; see next paragraph).
You can control this by specifying an inheritance control when you add the
shader module to the ShaderModuleCullCallback.

Shader types further qualify semantic strings. This allows you to
specify different shader modules for vertex, geometry, and fragment shaders that
share the same semantic. For example, this is convenient for per-pixel lighting, 
where you typically specify both a vertex and a fragment shader module,
both with the lighting semantic string.

\subsection sm-contents Shader Module Contents

In the backdropFX shader modules library, each shader module usually contains
only one entry point, which is typically associated with the shader semantic. For
example, the shader file \c ffp-lighting-on.vs has semantic lighting, and
contains a single entry point, \c computeLighting(). Shaders with semantic main 
almost always reference the entry points. For example, \c ffp-main.vs
contains a call to the \c computeLighting() function. At link time, OpenGL resolves 
function calls between shader modules.

\subsection sm-sepcomp Separate Compilation and the Include Directive

Separate compilation of shader code behaves similarly to separate compilation 
of host C/C++ code. On the host, note
that a system of shared header files provides for unique declaration of entry
points and shared variables. Similarly, the shader module system supports
including common declarations in multiple separately compiled shader modules.
Use the \c BDFX \c INCLUDE preprocessor directive to include another file.
In the backdropFX shader module library, files with declarations in their
name are shared header files, which multiple shader modules include.

Here's an example of shader module code to include another shader source file.
In this example, the included file is in \c shaders/gl2/bdfx-declarations.common.
\code
BDFX INCLUDE shaders/gl2/bdfx-declarations.common
\endcode

Many shader composition systems stitch together segments of partial functions
to form complete functions or monolithic \c main() function bodies. While the
backdropFX shader module system doesn't presently employ this strategy, it might
be possible to use the preprocessor to provide this functionality in a future
enhancement.

\subsection sm-port Shader Portability

The shader module system predefines several uniforms and vertex attributes
for familiar concepts to insulte the project from changes in the
underlying GLSL version. Most of these uniforms and vertex attributes have 
a direct correlation to GLSL 1.20
variable. For example, \c bdfx_vertex and \c bdfx_projectionMatrix instead
of \c gl_Vertex and \c gl_ProjectionMatrix. (In addition to the prefix, note
the change in capitalization style.) In the future, shader module code will
execute in an OpenGL 3 / GLSL 1.30 environment, without the GLSL 1.20
uniforms and vertex attributes. If you've developed any custom
shader module code for your application, a future move to GLSL 1.30 will likely
require that you port your code. However, the shader module systems predefined
uniforms and vertex attributes should minimize the required changes in your code.

If you are porting your own shader code to backdropFX, you should consider
modifying the code to use the shader module system predefined uniform and
vertex attributes.


\section sm-example-simple Example: Toggling Lighting State with Shaders

\ref shaderffptest The \c shaderffp test lets you toggle lighting on
and off with the \c L key. There are many ways to toggle state in shader-based
rendering. For example, you can specify a uniform on the host, and check its
value conditionally in the shader.

The \c shaderffp example uses a different strategy. The shader module library
contains two shader modules for lighting, \c ffp-lighting-on.vs and \c
ffp-lighting-off.vs. Both shader modules define the \c computeLighting()
entry point. However, in \c ffp-lighting-off.vs, the entry point is a no-op. To
toggle lighting on and off, \c shaderffp attaches one or the other shader 
to the scene graph root node.

\dontinclude shaderffp.cpp

\c shaderffp declases an event handler to trap \c L/l key presses.
\skipline KbdEventHandler

The event handler constructor loads the two shader modules it needs.
\skipline __LOAD_SHADER
\until __LOAD_SHADER
Each of the variables, \c _lightingOff and \c _lightingOn, are reference pointers
to osg::Shader objects. \c __LOAD_SHADER is a convenience macro declared in
\c ShaderModuleUtils.h. It allocates a new osg::Shader, loads the source, and
runs the shader preprocessor.

When the user presses the \c L key, the event handler obtains the ShaderModuleCullCallback
from the shader scene graph root node.
\skipline ShaderModuleCullCallback
\until getOrCreateShaderModuleCullCallback

Based on an internal variable that tracks the current lighting state, the event
handler adds the appropriate shader to toggle the state, either \c ffp-lighting-off.vs
or \c ffp-lighting-on.vs.
\skipline setShader
\until ShaderModuleCullCallback

The ShaderModuleCullCallback::setShader() function takes three parameters.
\li The semantic, in this case \c lighting 
\li A pointer to the shader to add 
\li An optional inheritance control (In this example, \c shaderffp is
forcing lighting off for the entire subgraph, so it specifies \c InheritanceOverride.)

The attached lighting shader module code compiles separately, and implements
the \c computeLighting() entry point another shader module references (in this case,
\c ffp-main.vs). Note, your application doesn't need to create an osg::Program
object and attach it to the root node StateSet. In backdropFX, separately compiled
shader modules link into compute programs using the RebuildShaderModules visitor.
The \c shaderffp event handler invokes this NodeVisitor after adding the
lighting shader module.
\skipline RebuildShaderModules
\until accept

Your application must invoke this NodeVisitor after any change to shader modules: adding
modules, deleting modules, and changing the source for any module.

RebuildShaderModules looks for Group nodes with an attached ShaderModuleCullCallback. When 
it finds one, it combines the attached shader modules with what it collects 
during traversal into a single osg::Program, and adds the Program in a new StateSet. It
stores the StateSet in the ShaderModuleCullCallback object map, which the current NodePath
indexes. Later during cull, ShaderModuleCullCallback uses the NodePath as a key to look
up the StateSet, pushes it onto the CullVisitor stack, traverses, then pops the StateSet.

*/


// namespace backdropFX
}


// __BACKDROPFX_SHADER_MODULE_H__
#endif
