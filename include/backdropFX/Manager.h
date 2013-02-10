// i Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_MANAGER_H__
#define __BACKDROPFX_MANAGER_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/DepthPeelBin.h>
#include <backdropFX/RenderingEffects.h>
#include <osg/Referenced>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/FrameBufferObject>
#include <osg/Texture2D>


namespace backdropFX
{


/** \brief Primary application interface for backdropFX rendering and scene management.

Manager is a <a href="http://en.wikipedia.org/wiki/Singleton_pattern"> singleton</a>.

The Manager class manages a collection of custom Group nodes and custom RenderStage
objects. These objects are below your osgViewer::Viewer (or osgUtil::SceneView), but
are above your scene graph root.

To render your scene using backdropFX, attach the scene root node using
setSceneData(), then call rebuild() to create the managed scene graph.

To attach backdropFX to a viewer, use getManagedRoot(), and add
the returned Group node to your Viewer or SceneView object.

Before rendering, you must specify the size of the internal textures that backdropFX 
is using as framebuffers by using the function setTextureWidthHeight(). Usually,
this is the size of your window or screen. For SceneView-based multi-view usage,
this is the size of your largest viewport (ignoring x and y).

The classes that Manager creates and manages are exposed to the application
for direct control. For example, to control the SkyDome directly, call
getSkyDome(), then call directly into the returned SkyDome reference.

*/
class BACKDROPFX_EXPORT Manager : public osg::Referenced
{
public:
    static Manager* instance( const bool erase=false );

    /** Access the managed root. 
	This is the top-most root node of the backdropFX hierarchy. Add the return 
	value to a Camera. For example in the case of SceneView:
    \code
        sceneView->setSceneData( Manager::instance()->getManagedRoot() );
    \endcode */
    osg::Node* getManagedRoot();

    /** Specify the root node of the application scene graph. For example:
    \code
        Manager::instance()->setSceneData( osgDB::readNodeFile( "foo.osg" ) );
    \endcode

    Later, when your application calls the rebuild function, rebuild adds the scene graph
	to one or more Manager-internal group nodes.
    */
    void setSceneData( osg::Node* node );

    /** Specify the destination for the final image. Pass NULL (the
    default) to render to the default FBO (from the window system). */
    void setFBO( osg::FrameBufferObject* dest );
    /** Get the destination setFBO() specifies. */
    osg::FrameBufferObject* getFBO() const;

    /** Build the internal structure based on the passed feature criteria.
    \bold You must call this before attempting to render. There is no default. \endbold
    \param featureFlags If absent, all features are enabled. Passing
    \c skyDome causes the sky dome to appear. Leave out \c skyDome if you don't want
    to see it. Pass \c depthPeel enabled transparency. Leave out \c depthPeel
    if you don't want transparency. */
    void rebuild( unsigned int featureFlags=defaultFeatures );
    static unsigned int defaultFeatures;
    static unsigned int skyDome;
    static unsigned int depthPeel;

    // Lighting and shadow controls
    // TBD

    /** Directly access the SkyDome class. */
    SkyDome& getSkyDome();
    /** Directly access the DepthPartition class. */
    DepthPartition& getDepthPartition();
    /** Directly access the DepthPeel class. */
    osg::Group& getDepthPeel();
    /** Directly access the glow Camera class. */
    osg::Camera& getGlowCamera();
    /** Directly access the RenderingEffects class. */
    RenderingEffects& getRenderingEffects();

    /** Access to rendered output. Color buffer A is the combined output
    of the SkyDome, DepthPartition, and DepthPeel classes. */
    osg::Texture2D* getColorBufferA();
    /** Access to rendered output. Color buffer Glow is the first output
    of the glow Camera, a non-blurred glow map. */
    osg::Texture2D* getColorBufferGlow();
    /** Access to rendered output. Depth buffer is the second output
    of the glow Camera and contains focal-distance processed depth values
    in the range 0.0 (in sharp focus) to 1.0 (fully blurred). */
    osg::Texture2D* getDepthBuffer();

    /** Specify texture dimensions. In typical usage, pass the window width and height.
    For SceneView multi-view rendering, pass the largest viewport width and height, ignoring x and y.
    \bold You must call this before attempting to render. There is no default. \endbold
    */
    void setTextureWidthHeight( unsigned int texW, unsigned int texH );
    void getTextureWidthHeight( unsigned int& texW, unsigned int& texH ) const;

    /** Debug mode control. Pass zero to disable debugging (the default),
    or pass a bitwise OR'd set of debug flags to enable specific debugging features.
    See the complete \ref debugflags "Manager Debug Flags" documentation. */
    void setDebugMode( unsigned int dm );
    unsigned int getDebugMode();

    // Profile mode on/off
    // TBD

protected:
    Manager();
    ~Manager();

    void internalInit();

    void resize();

    osg::ref_ptr< osg::Node > _sceneData;

    //
    // Stucture:
    //
    // _rootNode
    //     _skyDome
    //     _depthPart
    //         _depthPeel
    //             _sceneData
    //     _glowCamera
    //     _renderFX
    //
    osg::ref_ptr< osg::Group > _rootNode;
    // shadow maps
    osg::ref_ptr< SkyDome > _skyDome;
    osg::ref_ptr< DepthPartition > _depthPart;
    osg::ref_ptr< osg::Group > _depthPeel;
    osg::ref_ptr< RenderingEffects > _renderFX;

    // TBD temp, prototype code
    osg::ref_ptr< osg::Camera > _glowCamera;

    osg::ref_ptr< osg::FrameBufferObject > _colorBufferAFBO;
    osg::ref_ptr< osg::Texture2D > _colorBufferA;
    osg::ref_ptr< osg::Texture2D > _colorBufferGlow;
    osg::ref_ptr< osg::Texture2D > _depthBuffer;

    osg::ref_ptr< osg::FrameBufferObject > _fbo;


    unsigned int _texW, _texH;
    osg::ref_ptr< osg::Uniform > _widthHeight;

    unsigned int _dm;

    osg::ref_ptr< backdropFX::DepthPeelBin > _depthPeelBinProxy;
};


/** \mainpage backdropFX

\section toc Contents
- \ref overview "Overview and Features" <br />
- \ref building "Building and Dependencies" <br />
  - \ref recbuild "Building with Required and Recommended Dependencies" <br />
  .
- \ref ootb "Out of the Box" <br />
- \ref shadermodules "The backdropFX Shader Composition System" <br />
  - \ref sm-example-simple "Example: Toggling Lighting State with Shaders"  <br />
  .
- \ref debugflags "Manager Debug Flags" <br />


\section overview Overview and Features

backdropFX is an OSG-based rendering system with several powerful features:
/li Sky Dome with Sun and Moon positioned according to lat/long coordinates and time of day, 
with scene illuminated by the Sun
/li Depth partitioning to minimize the near plane without sacrificing depth buffer precision
/li Depth peeling for order-independent transparency
/li A rendering effects pipeline for screen space effects, such as glow, depth of field, and tone mapping
/li Fully shader-based rendering with extensive support for converting a scene graph FFP state 
attributes to 100 percent shader-based
/li Support for separately compiled shader modules (also known as shader composition)
/li Debugging and profiling aids for developers

Your application primarily interfaces with the backdropFX::Manager class. Manager creates
a scene graph of custom nodes to implement several rendering algorithms. Your scene graph
is a subgraph below this system of custom nodes.

\image html fig07-backdropfx.jpg

backdropFX::SkyDome, backdropFX::DepthPartition, and
backdropFX::RenderingEffects use custom osgUtil::RenderStage classes to implement their
algorithms and support rendering to texture.
Depth peeling is a standard osg::Group node that uses a custom RenderBin called
DepthPeelBin.
The Glow Camera currently creates a glow map, but in the future it will also provide
support for other rendering effects.

During the draw traversal of each form, the following operations occur:
/li If sky dome rendering is enabled, the SkyDome class renders the sky background, Sun, and Moon.
/li The DepthPartition class divides the view frustum into partitions. If depth peeling is enabled,
the following rendering occurs:
/li The DepthPeel class renders the application scene graph one or more times into depth peel 
  layer framebuffer objects.
/li The DepthPeel class composites the layers together in back to front order.

If depth peeling is disabled:
/li DepthPartition renders the application scene graph directly.
Note the DepthPartition node is always present. When disabled, it computes one partition.
  
/li The DepthPartition stores its color output in a texture map and is made available to downstream
rendering stages with a call to backdropFX::Manager::getColorBufferA().
/li If glow is enabled, the Glow Camera creates a glow map. The output is available to
downstream rendering stages with a call to backdropFX::Manager::getColorBufferGlow().
/li The RenderingEffects node processes its list of effects. The RenderingEffects node
is always present. When there are no application-specified effects, a no-op effect
displays color buffer A as a texture-mapped triangle pair.


\section building Building and Dependencies

backdropFX has the following dependencies. The version number indicates the tested version of 
the dependency.

<table>
<tr>
  <td>\b Dependency</td>  <td>\b Version</td>  <td>\b Required</td>
</tr>
<tr>
  <td>OSG</td>  <td>2.8.3</td>  <td>Required</td>
</tr>
<tr>
  <td>osgWorks</td>  <td>1.1.50</td>  <td>Required</td>
</tr>
<tr>
  <td>osgEphemeris</td>  <td>svn trunk</td>  <td>Required</td>
</tr>
<tr>
  <td>Boost</td>  <td>1.44</td>  <td>Required</td>
</tr>
<tr>
  <td>zlib / libPNG</td>  <td></td>  <td>Required</td>
</tr>

<tr>
  <td>Doxygen</td>  <td>1.7.2</td>  <td>Opt, but recommended</td>
</tr>
<tr>
  <td>GraphViz</td>  <td>2.26.3</td>  <td>Opt, but recommended</td>
</tr>

<tr>
  <td>osgWxTree</td>  <td></td>  <td>Opt</td>
</tr>
<tr>
  <td>wxWidgets</td>  <td>2.8.10</td>  <td>Opt</td>
</tr>
<tr>
  <td>Bullet</td>  <td></td>  <td>Opt</td>
</tr>
<tr>
  <td>osgBullet</td>  <td></td>  <td>Opt</td>
</tr>
<tr>
  <td>osgBulletPlus</td>  <td></td>  <td>Opt</td>
</tr>
</table>

\subsection recbuild Building with Required and Recommended Dependencies

  \li \b OSG When installed into a default location, backdropFX CMake finds it automatically.

  \li \b osgWorks When installed into a default location, backdropFX CMake finds it automatically.

  \li \b Boost When installed into a default location, backdropFX CMake finds it automatically.

  \li \bold zlib and libPNG \endbold If CMake fails to find zlib, show advanced options and fill in the
  ZLIB_INCLUDE_DIR and ZLIB_LIBRARY variables. Perform the same step for libPNG if necessary.
  On Windows, this is a two-step process: Configure, set zlib variables, configure again, and
  set libPNG variables. (In Windows, I have successfully used the zlib and libPNG from
  Mike Wieblen's OSG 3rdparty repo.)

  \li \b I tested osgEphemeris backdropFX with an osgEphemeris svn trunk source and build
  tree. Set OSGEPHEMERIS_SOURCE_DIR to the root of the source tree, and OSGEPHEMERIS_BUILD_DIR
  to the root of the CMake output directory.

  \li \b Documentation Select the BACKDROPFX_DOCUMENTATION checkbox to build Doxygen documentation.
  backdropFX CMake finds Doxygen in the default installation. Show advanced variables to
  verify that the GraphViz \b dot executable was also found.

After generating the project files, build the project and the documentation.

backdropFX has an install target, but we did not thoroughly test it. Testing was
primarily running executables from the Windows Visual Studio SDK (Windows)
and directly from the bld/bin dir (OS X). To install and run backdropFX, you need
to set PATH and possibly LD_LIBRARY_PATH to point to the install location.

\b Note: osgEphemeris uses the same name for both Release and Debug Windows DLLs.
This means whichever directory is first in your PATH, Release or Debug, dictates
which DLLs load and cause crashes when binaries mix. To avoid this, I
put the Release directory in my PATH, and set backdropFX project properties in Visual
Studio to put the Debug directory at the start of the PATH for debug builds.

\section ootb Out of the Box

Run these four executables first. Always verify that these executables render correctly
before committing any code changes.

\li \ref simpleexample "The \c simple Example" - This is the smallest and simplest example
that uses the full backdropFX functionality.

\li \ref shaderffptest "The \c shaderffp Test" - This displays a side-by-side comparison
of GL2 FFP lighting and backdropFX shader module equivalents. The images should look the
same, but currently the right image is slightly darker due to an incorrect default value
for ambient scene lighting.

\li \ref skydometest "The \c skydome Test" - This renders the sky dome in motion. Currently,
there are lighting problems with the loaded default terrain model.

\li \ref verticalslicetest "The \c verticalslice Test" - This is the backdropFX Swiss Army Knife,
allowing interactive control over many backdropFX features.

*/



/** \page debugflags Manager Debug Flags

backdropFX supports several debugging features that you can enable to assist
with isolating rendering and other issues. Applications enable debug mode by
passing a set of debug flags to the Manager::setDebugMode() function.

Declared in BackdropCommon, the debug flags are:

\li \b debugConsole Currently non-functional, use this flag to enable
backdropFX console messages.

\li \b debugImages Use this flag to capture screen shots at various
stages of the backdropFX rendering pipeline. You can test this with
the \ref verticalslicetest "verticalslice test" by pressing the 'D' key.
verticalslice sets the \b debugImages debug flag for one frame, and
backdropFX creates several PNG image files in the current working directory.

\li \b debugProfile Currently non-functional, use this flag to enable
backdropFX performance profiling.

\li \b debugShaders Use this flag to dump preprocessed shader source.
backdropFX supports an INCLUDE preprocessor directive to include the 
shader source from other files. OpenGL sees only the preprocessed source.
If the shader generates an error, error message line numbers are given
for the preprocessed source only. This makes locating the actual line
number in the unprocessed source very difficult. With the \b debugShaders
debug flag set, backdropFX writes a preprocessed copy of each shader module
to the current working directory, but inserts the original line numbers
at the start of each line. Go to the line number in the error message,
then read the actual line number from this file to locate the error in
the unprocessed shader source.

\li \b debugVisual Use this flag to add visual elements to the backdropFX
scene to aid in debugging. Currently, this flag enables the celestial sphere
grid in the SkyDome class.

*/


// namespace backdropFX
}

// __BACKDROPFX_MANAGER_H__
#endif
