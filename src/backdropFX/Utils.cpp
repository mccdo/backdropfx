// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/Utils.h>
#include <osg/GL>
#include <osg/FrameBufferObject>
#include <osg/GLExtensions>
#include <osg/Notify>
#include <osgwTools/Version.h>
#include <string>

// Workaround for RHEL/CentOS 5 and similar distros
// that define GL_EXT_framebuffer_multisample but not
// GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
#endif

#include <png.h>
//http://stackoverflow.com/questions/2442335/libpng-boostgil-png-infopp-null-not-found
#if PNG_LIBPNG_VER > 10210
#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL
#endif

#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/io/png_io.hpp>



namespace backdropFX
{


#if !defined( OSG_GL3_AVAILABLE )
// It's defined in gl3.h, but we can get this error using FBOs in GL2
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif

void
errorCheck( const std::string& msg )
{
    GLenum errorNo = glGetError();
    if( errorNo == GL_NO_ERROR )
        return;

    osg::notify( osg::WARN ) << "backdropFX: OpenGL error 0x" << std::hex << errorNo << std::dec << " ";
    switch( errorNo ) {
    case GL_INVALID_ENUM: osg::notify( osg::WARN ) << "GL_INVALID_ENUM"; break;
    case GL_INVALID_VALUE: osg::notify( osg::WARN ) << "GL_INVALID_VALUE"; break;
    case GL_INVALID_OPERATION: osg::notify( osg::WARN ) << "GL_INVALID_OPERATION"; break;
#if defined( OSG_GL1_AVAILABLE ) || defined( OSG_GL2_AVAILABLE ) || ( OSGWORKS_OSG_VERSION < 20900 )
    case GL_STACK_OVERFLOW: osg::notify( osg::WARN ) << "GL_STACK_OVERFLOW"; break;
    case GL_STACK_UNDERFLOW: osg::notify( osg::WARN ) << "GL_STACK_UNDERFLOW"; break;
#endif
    case GL_OUT_OF_MEMORY: osg::notify( osg::WARN ) << "GL_OUT_OF_MEMORY"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: osg::notify( osg::WARN ) << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
    default: osg::notify( osg::WARN ) << "Unknown"; break;
    }
    osg::notify( osg::WARN ) << " " << msg << std::endl;
}


void
fboErrorCheck( const std::string& msg, osg::FBOExtensions* fboExt )
{
#if( OSGWORKS_OSG_VERSION > 20907 )
    const GLenum fbStatus( fboExt->glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER_EXT ) );
#else
    const GLenum fbStatus( fboExt->glCheckFramebufferStatusEXT( GL_DRAW_FRAMEBUFFER_EXT ) );
#endif
    if( fbStatus == GL_FRAMEBUFFER_COMPLETE_EXT )
        return;

    osg::notify( osg::WARN ) << "backdropFX: OpenGL FBO status error 0x" << std::hex << fbStatus << std::dec << " ";
    switch( fbStatus ) {
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_UNSUPPORTED"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS"; break;
    default: osg::notify( osg::WARN ) << "Unknown"; break;
    }
    osg::notify( osg::WARN ) << " " << msg << std::endl;

    //
    // We had a problem with an FBO. Query the FBO and display info here.
    // TBD Future work: This could go into osgWorks.

    GLint drawFBO;
    glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING_EXT, &drawFBO );
    osg::notify( osg::WARN ) << "Current draw FBO: " << drawFBO << std::endl;


    typedef GLenum APIENTRY TglGetFramebufferAttachmentParameterivEXT(GLenum,GLenum,GLenum,GLint*);
    TglGetFramebufferAttachmentParameterivEXT* glGetFramebufferAttachmentParameterivEXT;
#define LOAD_FBO_EXT(name)    osg::setGLExtensionFuncPtr(name, (#name))
    LOAD_FBO_EXT(glGetFramebufferAttachmentParameterivEXT);
    if( !glGetFramebufferAttachmentParameterivEXT )
    {
        osg::notify( osg::WARN ) << "Can't load glGetFramebufferAttachmentParameterivEXT" << std::endl;
        return;
    }
    typedef GLenum APIENTRY TglGetRenderbufferParameterivEXT(GLenum,GLenum,GLint*);
    TglGetRenderbufferParameterivEXT* glGetRenderbufferParameterivEXT;
    LOAD_FBO_EXT(glGetRenderbufferParameterivEXT);
    if( !glGetFramebufferAttachmentParameterivEXT )
    {
        osg::notify( osg::WARN ) << "Can't load glGetFramebufferAttachmentParameterivEXT" << std::endl;
        return;
    }

#ifndef GL_FRAMEBUFFER_DEFAULT
#define GL_FRAMEBUFFER_DEFAULT 0x8218
#endif
    GLint color0Type;
    glGetFramebufferAttachmentParameterivEXT( GL_DRAW_FRAMEBUFFER_EXT,
        GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT, &color0Type );
    osg::notify( osg::WARN ) << "  Color 0 type: " << std::hex << color0Type << std::dec << " ";
    switch( color0Type ) {
    case GL_NONE: osg::notify( osg::WARN ) << "GL_NONE"; break;
    case GL_FRAMEBUFFER_DEFAULT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_DEFAULT"; break;
    case GL_TEXTURE: osg::notify( osg::WARN ) << "GL_TEXTURE"; break;
    case GL_RENDERBUFFER_EXT: osg::notify( osg::WARN ) << "GL_RENDERBUFFER"; break;
    default: osg::notify( osg::WARN ) << "Unknown"; break;
    }
    osg::notify( osg::WARN ) << std::endl;

    if( color0Type == GL_TEXTURE )
    {
        GLint textureID;
        glGetFramebufferAttachmentParameterivEXT( GL_DRAW_FRAMEBUFFER_EXT,
            GL_COLOR_ATTACHMENT0_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &textureID );
        osg::notify( osg::WARN ) << "  Texture ID: " << (GLuint) textureID << std::endl;

        glBindTexture( GL_TEXTURE_2D, (GLuint) textureID );

        GLint texWidth, texHeight;
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth );
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight );
        osg::notify( osg::WARN ) << "  width: " << texWidth << " height: " << texHeight << std::endl;
    }

    GLint depthType;
    glGetFramebufferAttachmentParameterivEXT( GL_DRAW_FRAMEBUFFER_EXT,
        GL_DEPTH_ATTACHMENT_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT, &depthType );
    osg::notify( osg::WARN ) << "  Depth type: " << std::hex << depthType << std::dec << " ";
    switch( depthType ) {
    case GL_NONE: osg::notify( osg::WARN ) << "GL_NONE"; break;
    case GL_FRAMEBUFFER_DEFAULT: osg::notify( osg::WARN ) << "GL_FRAMEBUFFER_DEFAULT"; break;
    case GL_TEXTURE: osg::notify( osg::WARN ) << "GL_TEXTURE"; break;
    case GL_RENDERBUFFER_EXT: osg::notify( osg::WARN ) << "GL_RENDERBUFFER"; break;
    default: osg::notify( osg::WARN ) << "Unknown"; break;
    }
    osg::notify( osg::WARN ) << std::endl;

    if( depthType == GL_RENDERBUFFER_EXT )
    {
        GLint rbID;
        glGetFramebufferAttachmentParameterivEXT( GL_DRAW_FRAMEBUFFER_EXT,
            GL_DEPTH_ATTACHMENT_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &rbID );
        osg::notify( osg::WARN ) << "  RB ID: " << (GLuint) rbID << std::endl;

#if( OSGWORKS_OSG_VERSION > 20907 )
        fboExt->glBindRenderbuffer( GL_RENDERBUFFER_EXT, rbID );
#else
        fboExt->glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, rbID );
#endif

        GLint rbWidth, rbHeight;
        glGetRenderbufferParameterivEXT( GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &rbWidth );
        glGetRenderbufferParameterivEXT( GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &rbHeight );
        osg::notify( osg::WARN ) << "  width: " << rbWidth << " height: " << rbHeight << std::endl;
    }
    else if( depthType == GL_TEXTURE )
    {
        GLint textureID;
        glGetFramebufferAttachmentParameterivEXT( GL_DRAW_FRAMEBUFFER_EXT,
            GL_DEPTH_ATTACHMENT_EXT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &textureID );
        osg::notify( osg::WARN ) << "  Depth texture ID: " << (GLuint) textureID << std::endl;

        glBindTexture( GL_TEXTURE_2D, (GLuint) textureID );

        GLint texWidth, texHeight;
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth );
        glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight );
        osg::notify( osg::WARN ) << "  width: " << texWidth << " height: " << texHeight << std::endl;
    }
}



void
debugDumpImage( const std::string& fileName, const char* pixels, const int w, const int h )
{
    std::ptrdiff_t rowBytes( w * 4 );
    // Create an image view of the RGBA 8-bit pixel block.
    boost::gil::rgba8_view_t iv = boost::gil::interleaved_view( w, h,
        ( const boost::gil::rgba8_ptr_t )pixels, rowBytes );
    // Create a view flipped in y, and write as PNG.
    boost::gil::png_write_view< boost::gil::rgba8_view_t >(
        fileName, boost::gil::flipped_up_down_view( iv ) );
}
void
debugDumpDepthImage( const std::string& fileName, const short* pixels, const int w, const int h )
{
    std::ptrdiff_t rowBytes( w * 2 );
    // Create an image view of the 16-bit pixel block.
    boost::gil::gray16_view_t iv = boost::gil::interleaved_view( w, h,
        ( const boost::gil::gray16_ptr_t )pixels, rowBytes );
    // Create a view flipped in y, and write as PNG.
    boost::gil::png_write_view< boost::gil::gray16_view_t >(
        fileName, boost::gil::flipped_up_down_view( iv ) );
}


std::string elementName( const std::string& prefix, int element, const std::string& suffix )
{
    std::ostringstream ostr;
    ostr << prefix << element << suffix;
    return( ostr.str() );
}


// namespace backdropFX
}
