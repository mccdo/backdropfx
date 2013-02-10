// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>

#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/Material>
#include <osg/Fog>
#include <osg/CullFace>
#include <osgDB/FileNameUtils>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>


/** \cond */
class KbdEventHandler : public osgGA::GUIEventHandler
{
public:
    KbdEventHandler( osg::Group* ffpRoot, osg::Group* shaderRoot )
      : _ffpRoot( ffpRoot ),
        _shaderRoot( shaderRoot ),
        _currentLighting( DEFAULT )
    {
        __LOAD_SHADER(_lightingOff,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-off.vs")
        __LOAD_SHADER(_lightingOn,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-on.vs")
    }
    
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
    {
        bool handled( false );
        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            case 'L':
            case 'l':
            {
                backdropFX::ShaderModuleCullCallback* smccb =
                    backdropFX::getOrCreateShaderModuleCullCallback( *_shaderRoot );
                osg::StateSet* ss = _ffpRoot->getOrCreateStateSet();
                ss->setDataVariance( osg::Object::DYNAMIC );
                switch( _currentLighting )
                {
                case DEFAULT:
                    _currentLighting = FORCE_OFF;
                    smccb->setShader( "lighting", _lightingOff.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceOverride );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE );
                    break;
                case FORCE_OFF:
                    _currentLighting = FORCE_ON;
                    smccb->setShader( "lighting", _lightingOn.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceOverride );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
                    break;
                case FORCE_ON:
                    _currentLighting = DEFAULT;
                    smccb->setShader( "lighting", _lightingOn.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceDefault );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
                    break;
                }

                backdropFX::RebuildShaderModules rsm;
                _shaderRoot->accept( rsm );

                break;
            }
            }
        }
        }
        return( handled );
    }
protected:
    osg::ref_ptr< osg::Group > _ffpRoot, _shaderRoot;
    osg::ref_ptr< osg::Shader > _lightingOff, _lightingOn;

    typedef enum {
        DEFAULT,
        FORCE_OFF,
        FORCE_ON
    } CurrentLighting;
    CurrentLighting _currentLighting;
};
/** \endcond */


void
defaultScene( osg::Group* root )
{
    osg::Node* model( osgDB::readNodeFile( "teapot.osg" ) );

    // Current limitation: Shader module source must be attached to a
    // Group or Group-derived node. In the default scene, we use
    // MatrixTransforms anyhow, so this is not a problem, but is a
    // serious issue for real apps.

    osg::MatrixTransform* mt;

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( 0., 0., -4.5 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
		// Add some fog
		osg::Fog* fog = new osg::Fog();
		fog->setMode(osg::Fog::LINEAR);
		fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
		fog->setStart(0.0);
		fog->setEnd(100.0);
		ss->setMode(GL_FOG, osg::StateAttribute::ON);
		ss->setAttribute(fog,osg::StateAttribute::ON);
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( -1., 0., -3.0 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
		// Add some fog
		osg::Fog* fog = new osg::Fog();
		fog->setMode(osg::Fog::EXP2);
		fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
		fog->setDensity(0.03f);
		ss->setMode(GL_FOG, osg::StateAttribute::ON);
		ss->setAttribute(fog,osg::StateAttribute::ON);
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( 1., 0., -3.0 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
		// Add some fog
		osg::Fog* fog = new osg::Fog();
		fog->setMode(osg::Fog::EXP);
		fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
		fog->setDensity(0.03f);
		ss->setMode(GL_FOG, osg::StateAttribute::ON);
		ss->setAttribute(fog,osg::StateAttribute::ON);
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( -1., 0., -1.5 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( 1., 0., -1.5 ) );
    {
        osg::Geode* geode = dynamic_cast< osg::Geode* >( model );
        osg::Geode* blueModel = new osg::Geode( *geode, osg::CopyOp::DEEP_COPY_ALL );
        mt->addChild( blueModel );
        root->addChild( mt );

        // Test color material by changing the color to blue.
        osg::Geometry* geom = dynamic_cast< osg::Geometry* >( blueModel->getDrawable( 0 ) );
        osg::Vec4Array* c = new osg::Vec4Array;
        c->push_back( osg::Vec4( 0., 0., 1., 1. ) );
        geom->setColorArray( c );
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( -1., 0., 0. ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        osg::Material* mat = new osg::Material;
        mat->setDiffuse( osg::Material::FRONT_AND_BACK, osg::Vec4( 1., 0., 0., 1. ) );
        mat->setSpecular( osg::Material::FRONT_AND_BACK, osg::Vec4( .9, .9, 0., 1. ) );
        mat->setShininess( osg::Material::FRONT_AND_BACK, 32.f );
        ss->setAttribute( mat );
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( 1., 0., 0. ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
        ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
        ss->setMode( GL_LIGHT1, osg::StateAttribute::ON );

        osg::LightSource* ls = new osg::LightSource;
        mt->addChild( ls );
        osg::Light* lt = new osg::Light;
        lt->setLightNum( 1 );
        lt->setAmbient( osg::Vec4( 0., 0., 0., 0. ) );
        lt->setDiffuse( osg::Vec4( 1., 1., 1., 1. ) );
        lt->setSpecular( osg::Vec4( 0., 0., 0., 0. ) );
        lt->setPosition( osg::Vec4( 1., 0., 0., 0. ) );
        ls->setLight( lt );
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( -1., 0., 1.5 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
        ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
        ss->setMode( GL_LIGHT2, osg::StateAttribute::ON );
        ss->setMode( GL_LIGHT3, osg::StateAttribute::ON );

        osg::LightSource* ls = new osg::LightSource;
        ls->setReferenceFrame( osg::LightSource::RELATIVE_RF );
        mt->addChild( ls );
        osg::Light* lt = new osg::Light;
        lt->setLightNum( 2 );
        lt->setDiffuse( osg::Vec4( 1., .15, .15, 1. ) );
        lt->setPosition( osg::Vec4( 1., 0., 1., 0. ) );
        ls->setLight( lt );

        ls = new osg::LightSource;
        ls->setReferenceFrame( osg::LightSource::ABSOLUTE_RF );
        mt->addChild( ls );
        lt = new osg::Light;
        lt->setLightNum( 3 );
        lt->setDiffuse( osg::Vec4( .15, .75, 1., 1. ) );
        lt->setPosition( osg::Vec4( -1., -1., 0., 0. ) );
        ls->setLight( lt );
    }

    mt = new osg::MatrixTransform(
        osg::Matrix::translate( 1., 0., 1.5 ) );
    mt->addChild( model );
    root->addChild( mt );
    {
        osg::StateSet* ss = mt->getOrCreateStateSet();
        ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
        ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
        ss->setMode( GL_LIGHT4, osg::StateAttribute::ON );

        osg::LightSource* ls = new osg::LightSource;
        mt->addChild( ls );
        osg::Light* lt = new osg::Light;
        lt->setLightNum( 4 );
        lt->setDiffuse( osg::Vec4( 1., 1., 1., 1. ) );
        lt->setPosition( osg::Vec4( -3., 0., -4., 1. ) );
        ls->setLight( lt );
    }
}


int
main( int argc, char ** argv )
{
    osg::ArgumentParser arguments( &argc, argv );

    osg::notify( osg::NOTICE ) << "  -cf\tCull front faces. Use this option to reproduce a bug with lighting back faces." << std::endl;
    bool cullFace( false );
    if( arguments.read( "-cf" ) )
        cullFace = true;

    osg::ref_ptr< osg::Group > rootFFP, rootShader;
    rootFFP = new osg::Group;
    rootShader = new osg::Group;
    if( arguments.argc() > 1 )
    {
        // Yes, we really want two copies of the loaded scene graph.
        rootFFP->addChild( osgDB::readNodeFiles( arguments ) );
        rootShader->addChild( osgDB::readNodeFiles( arguments ) );
    }
    else
    {
        defaultScene( rootFFP.get() );
        defaultScene( rootShader.get() );
    }

    {
        // Convert FFP to shaders
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( true );
        smv.setAttachTransform( true );
        rootShader->accept( smv );

        // Must run RebuildShaderSource any time the shader source changes:
        // Not just changing the source, but also setting the source for the
        // first time, inserting new nodes into the scene graph that have source,
        // deleteing nodes that have source, etc.
        backdropFX::RebuildShaderModules rsm;
        rootShader->accept( rsm );
    }

    if( cullFace )
    {
        osg::notify( osg::NOTICE ) << "-cf full front faces." << std::endl;

        osg::CullFace* cf = new osg::CullFace( osg::CullFace::FRONT );
        rootFFP->getOrCreateStateSet()->setAttributeAndModes( cf );
        rootShader->getOrCreateStateSet()->setAttributeAndModes( cf );
    }


    osgViewer::CompositeViewer viewer;
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );

    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    unsigned int width, height;
    wsi->getScreenResolution( osg::GraphicsContext::ScreenIdentifier( 0 ), width, height );
    const float aspect( (float)width / (float)( height * 2.f ) );
    const osg::Matrix proj( osg::Matrix::perspective( 50., aspect, 1., 10. ) );

    // Shared event handlers.
    osg::ref_ptr< KbdEventHandler > keh = new KbdEventHandler( rootFFP.get(), rootShader.get() );
    osg::ref_ptr< osgViewer::StatsHandler > sh = new osgViewer::StatsHandler;
    osg::ref_ptr< osgViewer::ThreadingHandler > th = new osgViewer::ThreadingHandler;
    osg::ref_ptr< osgViewer::WindowSizeHandler > wsh = new osgViewer::WindowSizeHandler;
    osg::ref_ptr< osgGA::TrackballManipulator > tb = new osgGA::TrackballManipulator;
    osg::ref_ptr< osgGA::StateSetManipulator > stateSetManipulator = new osgGA::StateSetManipulator;

    osg::ref_ptr< osg::StateSet > camSS;
    osg::ref_ptr<osg::GraphicsContext> gc;

    // view one
    {
        osgViewer::View* view = new osgViewer::View;
        view->setUpViewOnSingleScreen( 0 );
        viewer.addView( view );
        view->setSceneData( rootFFP.get() );

        view->addEventHandler( keh.get() );
        view->addEventHandler( sh.get() );
        view->addEventHandler( th.get() );
        view->addEventHandler( wsh.get() );
        view->setCameraManipulator( tb.get() );

        camSS = view->getCamera()->getOrCreateStateSet();
        stateSetManipulator->setStateSet( camSS.get() );
        view->addEventHandler( stateSetManipulator.get() );

        viewer.realize();
        gc = view->getCamera()->getGraphicsContext();
        view->getCamera()->setViewport( new osg::Viewport( 0, 0, width/2, height ) );
        view->getCamera()->setProjectionMatrix( proj );
    }

    // view two
    {
        osgViewer::View* view = new osgViewer::View;
        viewer.addView(view);
        view->setSceneData( rootShader.get() );

        view->addEventHandler( keh.get() );
        view->addEventHandler( sh.get() );
        view->addEventHandler( th.get() );
        view->addEventHandler( wsh.get() );
        view->setCameraManipulator( tb.get() );

        view->getCamera()->setStateSet( camSS.get() );
        view->addEventHandler( stateSetManipulator.get() );

        view->getCamera()->setViewport( new osg::Viewport( width/2, 0, width/2, height ) );
        view->getCamera()->setGraphicsContext( gc.get() );
        view->getCamera()->setProjectionMatrix( proj );
    }


    return( viewer.run() );
}

// Command line params:
//   The drawer:
//     usmc23_4019.asm.ive
//   Scaled cow:
//     cow.osg.(.25).scale


namespace backdropFX {


/** \page shaderffptest Test: shaderffp

The purpose of this test is to verify correct rendering when replacing FFP
with shader module equivalents. It also demonstrates how to toggle lighting
state via keyboard interaction.

Models to display can be specified on the command line. Otherwise, the test
loads the default scene, which verifies FFP lighting.
In the future, the test should be modified (or other tests added) to verify
other FFP features such as tex gen, texture mapping, fog, etc.

The test makes a deep copy of the scene, rendering the original on the left.
The test processes the scene graph copy using the ShaderModuleVisitor, which
replaces FFP state with shader modules and uniforms, and displays it on the
right. The two images should appear identical, but there are some minor
anomalies due to differences in default values, such as ambient scene light color.

Note that \c shaderffp doesn't use the Manager class, which is not typical usage.
Instead, it uses the shader modules feature standalone. This demonstrates that
the shader module feature could be broken out into a separate project if desired.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>-cf</b></td>
    <td>Cull front faces. Use this option to reproduce a bug with lighting back faces.</td>
  </tr>
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If no models are specified, this test displays several lit teapots.</td>
  </tr>
</table>

\section kbd Keyboard Commands

<table border="0">
  <tr>
    <td><b>L/l</b></td>
    <td>Cycle lighting through three states: Force off, force on, and default (on, but not forced).</td>
  </tr>
</table>

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler
    \li osgViewer::WindowSizeHandler
    \li osgGA::StateSetManipulator

\section ss Screen Shots

\image html fig04-shaderffp.jpg

This shows the default scene. Each teapot uses a different type of lighting,
exercising sbsolute/relative reference frame positioning, point, directional,
color material on and off, specular highlights, and other FFP lighting effects.
The six teapots on the left are rendered using standard OpenGL 2.x FFP lighting.
The six on the right are 100% shader-based, using backdropFX shader modules to
emulate FFP rendering.

*/


// backdropFX
}
