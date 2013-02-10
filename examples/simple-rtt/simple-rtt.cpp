// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>

#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/LocationData.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>
#include <osgwTools/Shapes.h>


// Start RTT infrastructure and support

// TBD primarily for debugging. Remove after we get RTT working.
class MyDCB : public osg::Drawable::DrawCallback
{
    virtual void drawImplementation( osg::RenderInfo& renderInfo, const osg::Drawable* drawable ) const
    {
        drawable->drawImplementation( renderInfo );
    }
};

osg::Node* rttSetUp( unsigned int width, unsigned int height )
{
    osg::ref_ptr< osg::Group > rttRoot = new osg::Group;
    rttRoot->setName( "rttRoot" );
    rttRoot->addChild( backdropFX::Manager::instance()->getManagedRoot() );

    // Configure the bdfx Manager to render to an FBO and texture.
    osg::ref_ptr< osg::Texture2D > rttTex = new osg::Texture2D;
    rttTex->setName( "rttTex" );
    rttTex->setInternalFormat( GL_RGBA );
    rttTex->setBorderWidth( 0 );
    rttTex->setTextureSize( width, height );
    rttTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    rttTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    osg::ref_ptr< osg::FrameBufferObject > fbo = new osg::FrameBufferObject;
    fbo->setAttachment( osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( rttTex.get() ) );
    backdropFX::Manager::instance()->setFBO( fbo.get() );

    // Set up triangle pair to display texture
    {
        osg::Geode* geode = new osg::Geode;
        geode->setName( "rttGeode" );
        geode->setCullingActive( false );

        osg::Drawable* draw = osgwTools::makePlane( osg::Vec3( -1., -1., 0. ),
            osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 2., 0. ) );
        draw->setName( "rttTriPair" );
        draw->setUseDisplayList( false );
        draw->setUseVertexBufferObjects( true );

        geode->addDrawable( draw );
        rttRoot->addChild( geode );

        // What order do I draw in relative to other stuff? Add callback so I can set a breakpoint there.
        draw->setDrawCallback( new MyDCB );

        osg::StateSet* stateSet = draw->getOrCreateStateSet();

        // Always draw. Don't even bother with the depth test.
        stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

        stateSet->setTextureAttributeAndModes( 0, rttTex.get() );

        // Tri pair shaders and uniform
        {
            osg::ref_ptr< osg::Program > program = new osg::Program();
            osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
            std::string shaderName( "simple-rtt.vs" );
            shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
            if( shader->getShaderSource().empty() )
                osg::notify( osg::WARN ) << "Can't load " << shaderName << std::endl;
            shader->setName( shaderName );
            program->addShader( shader );

            shader = new osg::Shader( osg::Shader::FRAGMENT );
            shaderName = "simple-rtt.fs";
            shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
            if( shader->getShaderSource().empty() )
                osg::notify( osg::WARN ) << "Can't load " << shaderName << std::endl;
            shader->setName( shaderName );
            program->addShader( shader );
            stateSet->setAttribute( program.get() );

            osg::ref_ptr< osg::Uniform > uTex = new osg::Uniform( "tex", 0 );
            stateSet->addUniform( uTex.get() );
        }
    }

    return( rttRoot.release() );
}

// End RTT infrastructure and support




osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .01, 100000. ) );
}

/** \cond */
class ResizeHandler : public osgGA::GUIEventHandler
{
public:
    ResizeHandler( osgViewer::Viewer& viewer, unsigned int width, unsigned int height )
      : _viewer( viewer ),
        _maxWidth( width ),
        _maxHeight( height )
    {}
    ~ResizeHandler() {}

    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        bool handled = false;

        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::RESIZE:
        {
            unsigned int width = ( unsigned int )ea.getWindowWidth();
            unsigned int height = ( unsigned int )ea.getWindowHeight();

            const double aspect = (double) width / (double) height;
            _viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );
            _viewer.getCamera()->getViewport()->setViewport( 0, 0, width, height );

            if( ( width > _maxWidth ) || ( height > _maxHeight ) )
            {
                _maxWidth = osg::maximum< unsigned int >( width, _maxWidth );
                _maxHeight = osg::maximum< unsigned int >( height, _maxHeight );
                backdropFX::Manager::instance()->setTextureWidthHeight( _maxWidth, _maxHeight );
            }
        }
        }
        return( handled );
    }

protected:
    osgViewer::Viewer& _viewer;

    unsigned int _maxWidth, _maxHeight;
};
/** \endcond */


osg::FrameBufferObject*
createDestinationFBO()
{
    osg::ref_ptr< osg::Texture2D > tex( new osg::Texture2D );
    tex->setTextureSize( 1920, 1200 );
    tex->dirtyTextureObject();
    tex->setInternalFormat( GL_RGBA );
    tex->setBorderWidth( 0 );
    tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );

    osg::ref_ptr< osg::FrameBufferObject > fbo( new osg::FrameBufferObject );
    fbo->setAttachment( osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( tex.get() ) );
    // backdropFX doesn't output depth buffer, so no need to specify it.

    return( fbo.release() );
}

void backdropFXSetUp( osg::Node* root, osg::Node* rttRoot, unsigned int width, unsigned int height )
{
    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::skyDome | backdropFX::Manager::depthPeel
        );

    // Note that Manager::rebuild() runs RSM implicitly,
    // but it has no idea we have a RTT hierachy above its managed root,
    // so we have to run it again. RSM must always be ran from
    // the ultimate root just below the top level SceneView Camera.
    // Otherwise, the NodePath, used as a key in ShaderModuleCullCallback,
    // fails to match. This would cause the error message:
    // "backdropFX: NULL StateSet. Should only happen if all ShaderModules are empty."
    {
        backdropFX::RebuildShaderModules rsm;
        rttRoot->accept( rsm );
    }

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );

    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void viewerSetUp( osgViewer::Viewer& viewer, osg::Node* node, osg::Node* rttRoot, unsigned int width, unsigned int height )
{
    double aspect = (double)width / (double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( rttRoot );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

int main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "simple example root" );

    osg::ArgumentParser arguments( &argc, argv );
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    osg::Group* modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    root->addChild( modelParent );

    // Disable GL_LIGHT0, leaving only the Sun enabled. This is kind of a hack,
    // as we currently can't disable this on the root node, have to disable it on
    // a child node.
    modelParent->getOrCreateStateSet()->setMode( GL_LIGHT0, osg::StateAttribute::OFF );

    // Convert loaded data to use shader composition.
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        smv.setSupportSunLighting( true ); // Use shaders that support Sun lighting.
        backdropFX::convertFFPToShaderModules( root.get(), &smv );
    }

    unsigned int width( 800 ), height( 600 );

    osg::ref_ptr< osg::Node > rttRoot = rttSetUp( width, height );
    backdropFXSetUp( root.get(), rttRoot.get(), width, height );

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, width, height );
    viewer.realize();

    viewerSetUp( viewer, root.get(), rttRoot.get(), width, height );


    viewer.run();


    return( 0 );
}


namespace backdropFX {


/** \page simplerttexample Example: simple-rtt

The purpose of this example is to show the least amount of code required to
render backdropFX into a texture and display it using a fullscreen triangle pair.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If no models are specified, this exmaple displays the cow and teapot.</td>
  </tr>
</table>

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler

\c simple-rtt also supports a resize handler. However, you must switch
osgViewer threading models to \c SingleThreaded before attempting to resize
the windows.

*/


// backdropFX
}
