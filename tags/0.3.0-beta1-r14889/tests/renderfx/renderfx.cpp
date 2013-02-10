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
#include <backdropFX/RenderingEffects.h>

#include <backdropFX/Effect.h>
#include <backdropFX/EffectLibrary.h>
#include <backdropFX/EffectLibraryUtils.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>


static osg::ref_ptr< osg::Texture2D > texture0;
static osg::ref_ptr< osg::FrameBufferObject > fbo0;

osg::Texture2D* initTexture( const std::string& fileName=std::string( "" ) )
{
    osg::ref_ptr< osg::Texture2D > tex( new osg::Texture2D );
    tex->setInternalFormat( GL_RGBA );
    tex->setBorderWidth( 0 );
    tex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    tex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::NEAREST );
    if( fileName.empty() )
    {
        // Hm, who manages the texture sizes? TBD...
        tex->setTextureSize( 800, 600 );
        tex->dirtyTextureObject();
    }
    else
    {
        tex->setImage( osgDB::readImageFile( fileName ) );
    }

    return( tex.release() );
}

osg::FrameBufferObject* initFBO( osg::Texture2D* tex )
{
    osg::ref_ptr< osg::FrameBufferObject > fbo( new osg::FrameBufferObject );
    fbo->setAttachment( osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment( tex ) );

    return( fbo.release() );
}

/** \cond */
class EffectHandler : public osgGA::GUIEventHandler
{
public:
    EffectHandler()
      : _glow( false ),
        _dof( false )
    {}
    ~EffectHandler() {}

    void usage()
    {
        osg::notify( osg::NOTICE ) << "  b\tToggle blur effect." << std::endl;
        osg::notify( osg::NOTICE ) << "  c\tToggle combine effect." << std::endl;
        osg::notify( osg::NOTICE ) << "  g\tToggle glow effect." << std::endl;
    }

    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        backdropFX::Manager* mgr = backdropFX::Manager::instance();
        osg::Camera& glowCam = mgr->getGlowCamera();
        backdropFX::RenderingEffects& rfx = mgr->getRenderingEffects();
        backdropFX::EffectVector& ev = rfx.getEffectVector();

        bool handled = false;

        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            case 'b': // blur
            {
                backdropFX::EffectSeperableBlur* esb = new backdropFX::EffectSeperableBlur();
                esb->setName( "SeperableBlur" );
                if( ev.empty() )
                    esb->addInput( 0, mgr->getColorBufferA() );
                else
                    ev.back()->attachOutputTo( esb, 0 );

                unsigned int texW, texH;
                mgr->getTextureWidthHeight( texW, texH );
                esb->setTextureWidthHeight( texW, texH );

                ev.push_back( esb );

                handled = true;
                break;
            }
            case 'c': // combine
            {
                backdropFX::EffectCombine* ec = new backdropFX::EffectCombine();
                ec->setName( "Combine" );
                if( texture0 == NULL )
                    texture0 = initTexture( "testpatternTwoGreen.png" );
                ec->addInput( 1, texture0.get() );
                if( ev.empty() )
                {
                    ec->addInput( 0, mgr->getColorBufferA() );
                }
                else
                {
                    ev.back()->attachOutputTo( ec, 0 );
                }

                // Probably not necessary for EffectCombine, as it has no internal textures.
                unsigned int texW, texH;
                mgr->getTextureWidthHeight( texW, texH );
                ec->setTextureWidthHeight( texW, texH );

                ev.push_back( ec );

                handled = true;
                break;
            }
            case 'g': // glow
            {
                _glow = !_glow;
                backdropFX::configureGlowEffect( _glow );
                handled = true;
                break;
            }
            case 'd': // DOF
            {
                _dof = !_dof;
                backdropFX::configureDOFEffect( _dof );
                handled = true;
                break;
            }
            case osgGA::GUIEventAdapter::KEY_Delete: // delete, clear all effects
            {
                _glow = false;
                _dof = false;
                ev.clear();
                handled = true;
                break;
            }
            }
        }
        }
        return( handled );
    }

protected:
    bool _glow, _dof;
};
/** \endcond */




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
            _viewer.getCamera()->setViewport( new osg::Viewport( 0, 0, width, height ) );

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



void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::skyDome | backdropFX::Manager::depthPeel
        );

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );

    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, unsigned int width, unsigned int height, osg::Node* node )
{
    double aspect = (double)width/(double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    viewer.addEventHandler( new osgViewer::StatsHandler() );
    viewer.addEventHandler( new osgViewer::ThreadingHandler() );
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );
    EffectHandler* eh = new EffectHandler();
    eh->usage();
    viewer.addEventHandler( eh );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

void addGlowState( osg::Node* node )
{
    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *node );

    osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
    std::string shaderName = "shaders/gl2/bdfx-finalize.fs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( shaderName ), shader );

    osg::StateSet* ss = node->getOrCreateStateSet();
    osg::Vec4f glowColor( .5, .4, 0., 1. ); // yellow-ish
    osg::Uniform* glow = new osg::Uniform( "bdfx_glowColor", glowColor );
    ss->addUniform( glow );
}

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "simple example root" );

    osg::ArgumentParser arguments( &argc, argv );
    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    osg::Group* modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    addGlowState( modelParent );
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
        root->accept( smv );
    }

    unsigned int width( 800 ), height( 600 );

    backdropFXSetUp( root.get(), width, height );


    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, width, height );

    viewerSetUp( viewer, width, height, root.get() );


    viewer.run();


    return( 0 );
}


namespace backdropFX {


/** \page simpleexample Example: simple

The purpose of this example is to show the least amount of code required to
render in backdropFX.

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

\c simple also supports a resize handler. However, you must switch
osgViewer threading models to \c SingleThreaded before attempting to resize
the windows.

*/


// backdropFX
}
