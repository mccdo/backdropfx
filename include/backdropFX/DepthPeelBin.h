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

TBD DepthPeel
More documentation is TBD.
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
    virtual void drawOpaque( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );
    virtual void drawTransparent( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous );
    virtual void drawComplete( osg::State& state, unsigned int insertStateSetPosition );

    /** DepthPeelBin encodes this into the file name of debug image dumps. */
    void setPartitionNumber( unsigned int partitionNumber ) { _partitionNumber = partitionNumber; }

protected:
    void internalInit();


    GLuint _textureUnit;
    int _minPixels;
    unsigned int _maxPasses;

    std::string createFileName( osg::State& state, int pass=-1, bool depth=false );
    unsigned int _partitionNumber;

    osg::ref_ptr< osg::Depth > _opaqueDepth;
    osg::ref_ptr< osg::Depth > _transparentDepth;


    /** \brief Context-specific data.
    */
    struct PerContextInfo
    {
        PerContextInfo();

        void init( const osg::State& state, const osg::Viewport* vp );
        void cleanup( const osg::State& state );
        bool _init;
        osg::ref_ptr< osg::Viewport > _viewport;

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
    virtual void drawFSTP( osg::RenderInfo& renderInfo, osg::State& state, osg::GL2Extensions* ext, PerContextInfo& pci );


    /** \brief A scoped FBO save and restore object.
    Obtain the current draw FBO in the constructor, save the FBO ID,
    and restore it (set the saved ID as the draw FBO) in the destructor.
    */
    class FBOSaveRestoreHelper
    {
    public:
        /** Query OpenGL for the currently bound framebuffer and save the FBO identifier in the \c _fboID member variable.
        The constructor doesn't change the FBO binding or any other OpenGL state.
        \param fboExt OSG FBO Extension object.
        \param pci A PerContextInfo object containing FBO function pointers that are missing from the OSG FBO Extension Object.
        \param fboTarget The FBO target to query. This is the \c target parameter to the glBindFramebuffer call. Default is \c GL_DRAW_FRAMEBUFFER_EXT.
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
        and will generate an OpenGL error if it is actually a renderbuffer. This function
        calls glBindFramebuffer to bind \c _fboID (the FBO being saved/restored).
        */
        GLuint getTextureID( GLenum attachment );

        /** Restore the saved FBO with a call to glBindFramebuffer, as in the destructor.
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

Originally, the depth peeling implementation suffered from a number of deficiencies.

\li \b RenderStage-based Much like Camera inserts a RenderStage into the render graph, the DepthPeel class
inserted a DepthPeelStage into the render graph, and the draw() method implemented the
algorithm. Use of a class derived from RenderStage is really not appropriate here. The algorithm
merely needs to iteratively draw, but doesn't need any other capabilities of the RenderStage class.
Furthermore, state handling in RenderStage objects is not well-documented, resulting in an
overall inability of the class to reliably set and restore state.

\li \b Opaque \b Handling The algorithm originally included opaque objects in the creation of the
depth peel layers. This is completely unnecessary and also very GPU intensive. But when the code
was originally developed, we had not yet identified a way to render opaque objects only once.

\li \b Memory \b Usage The algorithm created layers in front-to-back order, storing each layer
in a separate texture, then composited them in back-to-front order. While functional, this
solution used an enormous amount of GPU memory.

Replacing the use of a custom RenderStage with a simpler RenderBin had been
identified shortly after completion of the first version, and added to the project task list
as a desirable enhancement. We hope this will improve state handling.

Chris Hanson suggested an alternate implementation to render in back-to-front order.
This would dramatically reduce the memory consumption because we would reuse the same
color buffer for each layer, compositing it (combining it with color buffer A) as we
render, rather than storing each layer in its own GPU memory buffer before compositing
all of them. The idea was to render opaque first, and use the depth buffer from
the opaque pass as the depth map for the first layer. But consideration of this algorithm
revealed that we would need the opaque z values in the depth map for _every_ layer, not just
the first.

Finally, we came up with a way to only render opaque objects once. In standard depth peeling,
rendering back-to-front, we use a fragment shader with depth test set to GL_GREATER, testing
against the previous layer's depth map and passing fragment with the greatest depth value that
is also less than the stored depth map value. This pure version of the algorithm works well
if everything is transparent, or if opaque geometry is depth peeled as if it were
transparent, However, we use a modified version of this algorithm that uses two depth maps:
one from the previous layer, and the other from the opaque pass. This algorithm produces
layers composed of fragments with the greatest z values that are also less than
the stored z values from both depth maps.

Any Group node can be configured to use depth peeling on its children. For information
on how to configure a Group node for depth peeling, see DepthPeelBin.

*/



// backdropFX
}

// __BACKDROPFX_DEPTH_PEEL_BIN_H__
#endif
