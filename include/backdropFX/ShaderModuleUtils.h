// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SHADER_MODULE_UTILS_H__
#define __BACKDROPFX_SHADER_MODULE_UTILS_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/ShaderModule.h>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osg/Shader>
#include <string>


namespace backdropFX {


/** \defgroup ShaderUtils Shader Module Utilities */
/*@{*/

/** Dump a shader source with line numbers. OSG 2.8.3 did not support
this feature. The code is from the OSG 2.9.x series.
\param ostr The function writes shader source to this output stream.
\param preamble Written to the output stream before the shader source.
\param source Shader source code string.
*/
BACKDROPFX_EXPORT void dumpShaderSource( std::ostream& ostr, const std::string& preamble, const std::string& source );

/** \brief Run a preprocessor on shader source.

Currently, the preprocessor supports including files. The syntax for including
a file in your shader is:
\code
BDFX INCLUDE <filename>
\endcode
This preprocessor directive must start at column 0 with no text following the file name.

The preprocessor removes the directive and file name, loads the file (which it finds using  
the OSG_FILE_PATH), and inserts the file contents instead of the directive and file name.
*/
BACKDROPFX_EXPORT void shaderPreProcess( osg::Shader* shader );

/** Returns the Node ShaderModuleCullCallback. If the Node doesn't have a
ShaderModuleCullCallback, this function creates one, attaches it as a cull callback,
then returns it.

This routine handles cull callback nesting properly. It always adds the ShaderModuleCullCallback
as the cull callback, and adds any existing cull callback as a nested callback.
*/
BACKDROPFX_EXPORT ShaderModuleCullCallback* getOrCreateShaderModuleCullCallback( osg::Node& node );

/** \brief Extracts a shader semantic from a formatted string.

The string must be in one of these formats:
\code
   [text]-<semantic>-[text].[text]
   [text]-<semantic>-[text]
   [text]-<semantic>.[text]
\endcode

\param name The formatted string, and is usually a file name. For examples of shader
name strings that are formatted properly for use with this routine, see the
files in data/shaders/gl2.

As an example, consider the file name "ffp-lighting-on.vs". The semantic
is "lighting". As another example, consider "bdfx-main.fs". The semantic
is "main".
*/
BACKDROPFX_EXPORT std::string getShaderSemantic( const std::string& name );

/** Allocate, load, and preprocess a shader from a source file.
\param __s Pointer to an unallocated osg::Shader.
\param __t Shader type (VERTEX, GEOMETRY, FRAGMENT, etc.).
\param __n Shader source file name. File must be located in OSG file path.
You must #include <osg/FileNameUtils> to use this macro.
*/
#define __LOAD_SHADER(__s,__t,__n) \
    __s = new osg::Shader( __t ); \
    if( __s.valid() ) \
    { \
        __s->setName( osgDB::getSimpleFileName( __n ) ); \
        __s->loadShaderSourceFromFile( osgDB::findDataFile( __n ) ); \
        backdropFX::shaderPreProcess( __s.get() ); \
    }


/*@}*/


// namespace backdropFX
}


// __BACKDROPFX_SHADER_MODULE_UTILS_H__
#endif
