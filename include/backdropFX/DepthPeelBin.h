// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __BACKDROPFX_DEPTH_PEEL_BIN_H__
#define __BACKDROPFX_DEPTH_PEEL_BIN_H__ 1


#include <osgUtil/RenderBin>
#include <osg/FrameBufferObject>
#include <osg/GL2Extensions>
#include <osg/Geometry>
#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/Program>
#include <osg/Uniform>


namespace backdropFX
{


/** \brief A custom RenderBin that performs depth peeling on its children.

(This is the second implementation of depth peeling in backdropFX.
For details on the original version and how we redesigned it, see
DepthPeel Redesign.

DepthPeelBin is a custom RenderBin you can associate with any node in
your scene graph. When the OSG CullVisitor encounters a Node configured for depth 
peeling, it inserts an instance of DepthPeelBin into the render graph. OSG uses 
DepthPeelBin during draw to render geometry in child bins with a bin number greater than zero.

The backdropFX Manager class registers DepthPeelBin with the osgUtil library in the
Manager constructor. Manager also configures a standard osg::Group node for use with
depth peeling. A set of utility functions in DepthPeelUtils.h makes it easy for applications
to use DepthPeelBin with an arbitrary osg::Group and to enable and disable depth peeling
at run time.

DepthPeelBin uses standard osgUtil::RenderBin internal code to process child bins
up to bin number 0, as well as child RenderLeaf objects in an opaque pass. Child bins with
a bin number greater than 0 are considered potentially transparent. DepthPeelBin renders them
multiple times to create successive layers in back-to-front order. DepthPeelBin uses
OpenGL occlusion query to determine when to stop creating layers.

DepthPeelBin uses OpenGL framebuffer objects to render each layer to an internal texture image.
For color buffers, DepthPeelBin immediately composites each layer into the output framebuffer object.
(In backdropFX, this is color buffer A.) For depth buffers, DepthPeelBin reuses the last layer 
depth buffer as an input texture for creating the next layer. DepthPeelBin uses one color buffer
for all layers. 

DepthPeelBin uses three depth buffers: 

\li One for the opaque pass as input toall transparent layers 
\li Two depth buffers alternatively as an input texture or for depth destination 
\li DepthPeelBin stored framebuffer objects and textures in the PerContextInfo
struct with one instance for every unique OSG context identifier.

DepthPeelBin assumes it always renders into the lower-left corner of a texture
image. (backdropFX facilitates this by using RTTViewport, which always has xy=(0,0).)
DepthPeelBin identifies window size changes when the incoming viewport width and height
are larger than the previously stored width and height. When DepthPeelBin detects a resize,
it deletes all OpenGL objects and creates new ones. The internal texture width
and height is always as large as the largest viewport the application specifies.
DepthPeelBin never resizes textures downward. See Manager::setTextureWidthHeight().

During rendering, DepthPeelBin composites the current layer to the
output framebuffer by rendering a textured triangle pair. The internal texture
size might be larger than the viewport, so using (0,0)-(1,1) texture coordinates
for the corners is incorrect. Instead, the depth peel code uses the 
uniform variable \c depthPeelTexturePercent to specify the texture coordinate of
the upper right corner, so the triangle pair displays only the valid lower-left
corner of the texture image. See data/shaders/DepthPeelDisplay.vs.

To switch between rendering to an internal FrameBufferObject and back to the output
framebuffer, DepthPeelBin uses a class called FBOSaveRestoreHelper. This class 
constructor queries OpenGL for the current framebuffer object and saves its ID, and
the destructor binds that ID. Calling code can also rebind the ID with the restore()
method.

*/
class DepthPeelBin : public osgUtil::RenderBin
{
public:
    DepthPeelBin();
    DepthPeelBin( osgUtil::RenderBin::SortMode mode );
    DepthPeelBin( const DepthPeelBin& rhs, const osg::CopyOp& copyOp=osg::CopyOp::SHALLOW_COPY );
    ~DepthPeelBin();

    virtual osg::Object* cloneType() const { return new DepthPeelBin(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new DepthPeelBin(*this,copyop); } // note only implements a clone of type.
    virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const DepthPeelBin*>(obj)!=0L; }
    virtual const char* libraryName() const { return "backdropFX"; }
    virtual const char* className() const { return "DepthPeelBin"; }

    virtual void drawImplementation( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );

    virtual unsigned int drawInit( osg::State& state, osgUtil::RenderLeaf*& previous );
    virtual int drawOpaque( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );
    virtual void drawTransparent( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );
    virtual void drawComplete( osg::State& state, unsigned int insertStateSetPosition );

    /** DepthPeelBin encodes this into the debug image dumps file name. */
    void setPartitionNumber( unsigned int partitionNumber ) { _partitionNumber = partitionNumber; }

    /** DepthPeelBin uses two texture units: \c s_textureUnit, and \c s_textureUnit+1.
    These units reference two depth maps (one from the opaque pass and one from the
    previous layer), used to create each successive depth peel layer.

    The texture unit is static and is therefore the same for all DepthPeelBins (though
    usually there is only one DepthPeelBin in the render graph). Currently there is no
    setter for this value, but this can be added later if needed. Currently the value
    of s_textureUnit is 14, which means DepthPeelBin uses both units 14 and 15. */
    static GLuint getTextureUnit() { return( s_textureUnit ); }

protected:
    void internalInit();


    static GLuint s_textureUnit;
    int _minPixels;
    unsigned int _maxPasses;

    std::string createFileName( osg::State& state, int pass=-1, bool depth=false );
    unsigned int _partitionNumber;

    osg::ref_ptr< osg::Depth > _opaqueDepth;
    osg::ref_ptr< osg::Depth > _transparentDepth;


    /** \brief Context-specific data
    */
    struct PerContextInfo
    {
        PerContextInfo();

        void init( const osg::State& state, const GLsizei width, const GLsizei height );
        void cleanup( const osg::State& state );
        bool _init;
        GLsizei _width, _height;

        GLuint _fbo;
        GLuint _depthTex[ 3 ];
        GLuint _colorTex;
        GLuint _queryID;


        typedef void ( APIENTRY * GenQueriesProc )( GLsizei n, GLuint *ids );
        typedef void ( APIENTRY * DeleteQueriesProc )( GLsizei n, const GLuint *ids );
        typedef void ( APIENTRY * BeginQueryProc )( GLenum target, GLuint id );
        typedef void ( APIENTRY * EndQueryProc )( GLenum target );
        typedef void ( APIENTRY * GetQueryObjectivProc )( GLuint id, GLenum pname, GLint *params );
        typedef GLenum ( APIENTRY * GetFramebufferAttachmentParameterivProc )( GLenum, GLenum, GLenum, GLint* );

        GenQueriesProc _glGenQueries;
        DeleteQueriesProc _glDeleteQueries;
        BeginQueryProc _glBeginQuery;
        EndQueryProc _glEndQuery;
        GetQueryObjectivProc _glGetQueryObjectiv;
        GetFramebufferAttachmentParameterivProc _glGetFramebufferAttachmentParameteriv;
    };
    static osg::buffered_object< PerContextInfo > s_contextInfo;


    // Do not convert this comment block to Doxygen.
    // Making these regular member variables should be OK. OSG uses the
    // DepthPeelBin copy constructor each frame to create a new bin copied
    // from the template. Only one template is created. This means each
    // frame's bin will get a reference to the actual instantiations of
    // these objects.
    osg::ref_ptr< osg::Geometry > _fstp;
    osg::ref_ptr< osg::Program > _fstpProgram;
    osg::ref_ptr< osg::BlendFunc > _fstpBlendFunc;
    osg::ref_ptr< osg::Uniform > _fstpUniform;
    osg::ref_ptr< osg::Uniform > _texturePercentUniform;
    virtual void drawFSTP( osg::RenderInfo& renderInfo, osg::State& state, osg::GL2Extensions* ext, PerContextInfo& pci,
        GLint& fstpLoc, GLint& texturePercentLoc );


    /** \brief A scoped FBO save and restore object.
    Obtain the current draw FBO in the constructor, save the FBO ID,
    and in the destructor, restore it by setting the saved ID as the draw FBO.
    */
    class FBOSaveRestoreHelper
    {
    public:
        /** Query OpenGL for the currently bound framebuffer and save the FBO identifier in the \c _fboID member variable.
        The constructor doesn't change the FBO binding or any other OpenGL state.
        \param fboExt OSG FBO Extension object
        \param pci A PerContextInfo object containing FBO function pointers that are missing from the OSG FBO Extension Object
        \param fboTarget The FBO target to query. This is the \c target parameter to the glBindFramebuffer call. The default is \c GL_DRAW_FRAMEBUFFER_EXT.
        */
        FBOSaveRestoreHelper( osg::FBOExtensions* fboExt, DepthPeelBin::PerContextInfo& pci, GLenum fboTarget=GL_DRAW_FRAMEBUFFER_EXT );
        /** Binds the FBO that was queried and saved in the constructor.
        This function issues the following OpenGL command:
        \code
        glBindFramebuffer( _fboTarget, _fboID );
        \endcode
        */
        ~FBOSaveRestoreHelper();

        /** Returns the texture ID for the specified attachment.
        This function assumes the buffer at the specified attachment is a texture
        and generates an OpenGL error if it is actually a renderbuffer. This function
        calls glBindFramebuffer to bind \c _fboID (the saved/restored FBO).
        */
        GLuint getTextureID( GLenum attachment );

        /** Restore the saved FBO with a call to glBindFramebuffer as in the destructor.
        */
        void restore();

    protected:
        GLuint _fboID;
        GLenum _fboTarget;
        PerContextInfo& _pci;
        osg::FBOExtensions* _fboExt;
    };

};



/** \page depthpeelredesign DepthPeel Redesign

The depth peel implementation was rewritten in early 2009 and late 2010.
The original implementation had a number of deficiencies.

\li \b RenderStage-based The way Camera inserted a RenderStage into the render graph is the way 
the DepthPeel class inserted a DepthPeelStage into the render graph, and the draw() method implemented the
algorithm. Use of a class derived from RenderStage was inappropriate. The algorithm
merely needed to iteratively draw, but didn't need other capabilities of the RenderStage class.
Furthermore, state handling in RenderStage objects isn't well documented, resulting in an
overall inability of the class to reliably set and restore state.

\li \b Opaque \b Handling The algorithm originally included opaque objects in the 
depth peel layers creation. This was completely unnecessary and GPU intensive. But when we 
originally developed the code, we hadn't identified a way to render opaque objects only once.

\li \b Memory \b Usage The algorithm created layers in front-to-back order, stored each layer
in a separate texture, then composited them in back-to-front order. While functional, this
solution used an enormous amount of GPU memory.

Shortly after completion of the first version, we identified that replacing a custom 
RenderStage with a simpler RenderBin was a desirable enhancement and added it to the project task list. 
We hope this improves state handling.

Chris Hanson suggested an alternate implementation to create layers in back-to-front order.
This dramatically reduces the memory consumption, because we reuse the same
color buffer for each layer, compositing it (combining it with color buffer A) as we
render, rather than storing each layer in its own GPU memory buffer before compositing
all of them. The idea is to render opaque first and use the depth buffer from
the opaque pass as the depth map for the first layer. But consideration of this algorithm
reveals we need the opaque depth map to create all layers, not just
the first.

Finally, we came up with a way to only render opaque objects once. In standard depth peeling,
rendering back-to-front, we use a fragment shader with depth test set to GL_GREATER, testing
against the previous layer depth map and passing fragments with the greatest depth value that
is also less than the stored depth map value. This pure version of the algorithm works well
if everything is transparent or if opaque geometry is depth peeled as if it were
transparent. However, we use a modified version of this algorithm that uses two depth maps,
one from the previous layer and the other from the opaque pass. This algorithm produces
layers composed of fragments with the greatest z values that are also less than
the stored z values from both depth maps.

You can configure any Group node to use depth peeling on its children. For information
on how to configure a Group node for depth peeling, see DepthPeelBin.

*/



// backdropFX
}

// __BACKDROPFX_DEPTH_PEEL_BIN_H__
#endif
