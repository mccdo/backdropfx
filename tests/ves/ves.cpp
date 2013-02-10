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
#include <backdropFX/DepthPartition.h>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/ShaderModuleUtils.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgwTools/ReadFile.h>


class ParentNodePathVisitor : public osg::NodeVisitor
{
public:
    ParentNodePathVisitor( osg::Node* startNode, osg::Node* stopNode = 0 )
      : NodeVisitor( TRAVERSE_PARENTS ),
        m_stopNode( stopNode )
    {
        startNode->accept( *this );
    }
    virtual void apply( osg::Node& node )
    {
        if( ( node.getNumParents() == 0 ) || 
            ( &node == m_stopNode.get() ) )
        {
            _finalNodePath = getNodePath();
        }
        else
        {
            osg::NodeVisitor::traverse( node );
        }
    }
    osg::NodePath getFinalNodePath()
    {
        return( _finalNodePath );
    }
protected:
    osg::NodePath _finalNodePath;
    osg::ref_ptr< osg::Node > m_stopNode;
};



osg::Matrix
createProjection( double aspect )
{
    return( osg::Matrix::perspective( 35., aspect, .05, 1000. ) );
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


void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    // Set position (and enable) light 0.
    // TBD The distance of the light is too great for effective shadowing
    //   at this time.
    // Note this is needed only because SkyDome is off. When SkyDOme is on,
    // bdfx provides a Sun light.
    osg::ref_ptr< osg::Light > light = new osg::Light();
    light->setLightNum( 0 );
    light->setAmbient( osg::Vec4d( 0.36862, 0.36842, 0.36842, 1.0 ) );
    light->setDiffuse( osg::Vec4d( 0.88627, 0.88500, 0.88500, 1.0 ) );
    light->setSpecular( osg::Vec4d( 0.49019, 0.48872, 0.48872, 1.0 ) );
    //We are in openGL space
    light->setPosition( osg::Vec4d( 0.0, -30.0, 30.0, 0.0 ) );
    backdropFX::Manager::instance()->setLight( light.get() );

    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild( backdropFX::Manager::defaultFeatures & 
                                             //~backdropFX::Manager::shadowMap & 
                                             ~backdropFX::Manager::skyDome & 
                                             ~backdropFX::Manager::depthPeel ); // sky dome / depth peel off

    // Disable depth partitioning.
    backdropFX::Manager::instance()->getDepthPartition().setNumPartitions( 1 );


    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, unsigned int width, unsigned int height, osg::Node* node )
{
    double aspect = (double)width / (double)height;

    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    //viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    //viewer.getCamera()->setProjectionMatrix( createProjection( aspect ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    // No longer needed, apparently. Seems like Viewer used to focus the
    // camera manipulator on the entire SkyDome, putting it too far away from
    // the scene. This code compensated for that.
    //osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator();
    //viewer.setCameraManipulator( tb );
    //tb->setNode( root.get() );
    //tb->home( 0 );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
    viewer.addEventHandler( new ResizeHandler( viewer, width, height ) );

    osgGA::TrackballManipulator* tbm = new osgGA::TrackballManipulator;
    viewer.setCameraManipulator( tbm );
    tbm->setNode( node );
    tbm->home( 0 );
}

void 
addTransparentQuad( osg::Group* rootGroup )
{
    osg::ref_ptr< osg::Vec3Array > vertices = new osg::Vec3Array();
    /*vertices->push_back( osg::Vec3( 0.0f, 0.0f, 0.0 ) );
     vertices->push_back( osg::Vec3( 1.0f, 0.0f, 0.0 ) );
     vertices->push_back( osg::Vec3( 1.0f, 1.0f, 0.0 ) );
     vertices->push_back( osg::Vec3( 0.0f, 1.0f, 0.0 ) );*/
    //if( ves::xplorer::scenegraph::SceneManager::instance()->IsDesktopMode() )
    {
        vertices->push_back( osg::Vec3( -1.0f, -1.0f, 1.0 ) );
        vertices->push_back( osg::Vec3(  1.0f, -1.0f, 1.0 ) );
        vertices->push_back( osg::Vec3(  1.0f,  1.0f, 1.0 ) );
        vertices->push_back( osg::Vec3( -1.0f,  1.0f, 1.0 ) );
    }
        
    //
    osg::Vec4f coordinates = osg::Vec4f( 0.f, 1.f, 0.f, 1.f );
    float m_left = coordinates.w();
    float m_right = coordinates.x();
    float m_bottom = coordinates.y();
    float m_top = coordinates.z();
    
    osg::ref_ptr< osg::Vec2Array > texture_coordinates = new osg::Vec2Array();
    texture_coordinates->push_back( osg::Vec2( m_left, m_bottom ) );
    texture_coordinates->push_back( osg::Vec2( m_right, m_bottom ) );
    texture_coordinates->push_back( osg::Vec2( m_right, m_top ) );
    texture_coordinates->push_back( osg::Vec2( m_left, m_top ) );
    
    //
    osg::ref_ptr< osg::Geometry > geometry = new osg::Geometry();
    geometry->setVertexArray( vertices.get() );
    geometry->addPrimitiveSet(
                              new osg::DrawArrays( osg::PrimitiveSet::QUADS, 0, 4 ) );
    geometry->setTexCoordArray( 0, texture_coordinates.get() );
    
    //
    osg::ref_ptr< osg::Geode > geode = new osg::Geode();
    geode->setCullingActive( false );
    geode->setDataVariance( osg::Object::DYNAMIC );
    geode->addDrawable( geometry.get() );

    
    //Attach the image in a Texture2D object
    osg::ref_ptr< osg::Texture2D > texture = new osg::Texture2D();
    texture->setResizeNonPowerOfTwoHint( false );
    texture->setImage( osgDB::readImageFile( "vesParallaxMapping.png" ) );
    texture->setDataVariance( osg::Object::DYNAMIC );
    
    //Create stateset for adding texture
    osg::ref_ptr< osg::StateSet > gstateset = geode->getOrCreateStateSet();
    gstateset->setTextureAttributeAndModes(
                                          0, texture.get(), osg::StateAttribute::ON );

    osg::ref_ptr< osg::Group > uiGroup = new osg::Group();
    uiGroup->addChild( geode.get() );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->setName( "VS UI Quad Program" );
    
    //if( ves::xplorer::scenegraph::SceneManager::instance()->IsDesktopMode() )
    {
        osg::ref_ptr< osg::Shader > vertexShader = new osg::Shader();
        std::string vertexSource =
        "void main() \n"
        "{ \n"
        //Ignore MVP transformation as vertices are already in Normalized Device Coord.
        "gl_Position = gl_Vertex; \n"
        "gl_TexCoord[ 0 ].st = gl_MultiTexCoord0.st; \n"
        "} \n";
        
        vertexShader->setType( osg::Shader::VERTEX );
        vertexShader->setShaderSource( vertexSource );
        vertexShader->setName( "VS UI Quad Vertex Shader" );
        program->addShader( vertexShader.get() );
    }
    
    osg::ref_ptr< osg::Shader > fragmentShader = new osg::Shader();
    std::string fragmentSource =
    "uniform sampler2D baseMap; \n"
    "uniform float opacityVal;\n"
    "uniform vec3 glowColor; \n"
    "uniform vec2 mousePoint; \n"
    
    "void main() \n"
    "{ \n"
    "vec4 baseColor = texture2D( baseMap, gl_TexCoord[ 0 ].st ); \n"
    // Calculate distance to circle center
    "float d = distance(gl_TexCoord[0].st, mousePoint);\n"
    "vec4 tempColor = baseColor;\n"
    
    "if( d < 0.05 )\n"
    "{\n"
    //width of "pixel region" in texture coords
    //"   vec2 texCoordsStep = 1.0/(vec2(float(600),float(967))/float(20)); \n"
    //x and y coordinates within "pixel region"
    //"   vec2 pixelRegionCoords = fract(gl_TexCoord[0].st/texCoordsStep);\n"
    ///Radius squared
    "   float radiusSqrd = pow(0.01,2.0);\n"
    ///tolerance
    "   float tolerance = 0.0001;\n"
    "   vec2 powers = pow(abs(gl_TexCoord[0].st - mousePoint),vec2(2.0));\n"
    //Equation of a circle: (x - h)^2 + (y - k)^2 = r^2
    "   float gradient = smoothstep(radiusSqrd-tolerance, radiusSqrd+tolerance, pow(d,2.0) );\n"
    //blend between fragments in the circle and out of the circle defining our "pixel region"
    "   tempColor = mix( vec4(1.0,0.0,0.0,1.0), baseColor, gradient);\n"
    "}\n"
    //"vec2 powers = pow(abs(gl_TexCoord[ 0 ].st - mousePoint),vec2(2.0));\n"
    ///Radius squared
    //"float radiusSqrd = pow(0.003,2.0);\n"
    ///tolerance
    //"float tolerance = 0.0001;\n"
    //Equation of a circle: (x - h)^2 + (y - k)^2 = r^2
    //"float gradient = smoothstep(radiusSqrd-tolerance, radiusSqrd+tolerance, powers.x+powers.y);\n"
    //blend between fragments in the circle and out of the circle defining our "pixel region"
    //"vec4 tempColor = mix( vec4(1.0,0.0,0.0,1.0), baseColor, gradient);\n"
    "tempColor.a = opacityVal;\n"
    "gl_FragData[ 0 ] = tempColor; \n"
    "gl_FragData[ 1 ] = vec4( glowColor, gl_FragData[ 0 ].a ); \n"
    "} \n";
    
    fragmentShader->setType( osg::Shader::FRAGMENT );
    fragmentShader->setShaderSource( fragmentSource );
    fragmentShader->setName( "VS UI Quad Fragment Shader" );
    program->addShader( fragmentShader.get() );
    
    //Set depth test to always pass and don't write to the depth buffer
    osg::ref_ptr< osg::Depth > depth = new osg::Depth();
    depth->setFunction( osg::Depth::ALWAYS );
    depth->setWriteMask( false );
    
    //Create stateset for adding texture
	osg::StateAttribute::GLModeValue glModeValue =
    osg::StateAttribute::ON |
    osg::StateAttribute::PROTECTED |
    osg::StateAttribute::OVERRIDE;
    osg::ref_ptr< osg::StateSet > stateset = uiGroup->getOrCreateStateSet();
    stateset->setRenderBinDetails( 99, "RenderBin" );
    stateset->setAttributeAndModes( depth.get(), glModeValue );
    stateset->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );
    stateset->setMode( GL_LIGHTING, glModeValue);
    stateset->setAttributeAndModes( program.get(), glModeValue );
    stateset->addUniform( new osg::Uniform( "baseMap", 0 ) );
    osg::ref_ptr< osg::Uniform > opacityUniform = new osg::Uniform( "opacityVal", 0.2f );
    stateset->addUniform( opacityUniform.get() );
    
    osg::ref_ptr< osg::Uniform > mousePointUniform = new osg::Uniform( "mousePoint", osg::Vec2d( 0.2, 0.2 ) );
    stateset->addUniform( mousePointUniform.get() );
    
    osg::ref_ptr< osg::BlendFunc > bf = new osg::BlendFunc();
    bf->setFunction( osg::BlendFunc::SRC_ALPHA, 
                    osg::BlendFunc::ONE_MINUS_SRC_ALPHA );
    stateset->setMode( GL_BLEND, glModeValue );
    stateset->setAttributeAndModes( bf.get(), glModeValue );
    
    
    rootGroup->addChild( uiGroup.get() );
}

void
runSMV( osg::Group* group, osg::Node* modelNode )
{
    //if( !scenegraph::SceneManager::instance()->IsRTTOn() )
    {
        backdropFX::ShaderModuleCullCallback::ShaderMap tempMap;
        ParentNodePathVisitor parentVisitor( group, backdropFX::Manager::instance()->getManagedRoot() );
        osg::NodePath nodePath = parentVisitor.getFinalNodePath();
        osg::StateSet* tempState = backdropFX::accumulateStateSetsAndShaderModules( tempMap, nodePath );
        
        backdropFX::ShaderModuleVisitor smv;
        smv.setInitialStateSet( tempState, tempMap );
        
        backdropFX::convertFFPToShaderModules( modelNode, &smv );
    }    
    group->addChild( modelNode );
    //if( !scenegraph::SceneManager::instance()->IsRTTOn() )
    {
        backdropFX::RebuildShaderModules rsm;
        backdropFX::Manager::instance()->getManagedRoot()->accept( rsm );
    }
}

#if 0
// This is no longer the preferred way to set up lights in bdfx.
// See the call to setLight in the backdropFXSetUp function.
void
setupLight( osg::Group* group )
{
    osg::Light* light0 = new osg::Light();
    light0->setLightNum( 0 );
    light0->setAmbient( osg::Vec4d( 0.36862, 0.36842, 0.36842, 1.0 ) );
    light0->setDiffuse( osg::Vec4d( 0.88627, 0.88500, 0.88500, 1.0 ) );
    light0->setSpecular( osg::Vec4d( 0.49019, 0.48872, 0.48872, 1.0 ) );
    //We are in openGL space
    light0->setPosition( osg::Vec4d( 0.0, -10000.0, 10000.0, 0.0 ) );
    //m_light0->setDirection( osg::Vec3d( 0.0, 1.0, -1.0 ) );
    
    osg::LightSource* lightSource0 = new osg::LightSource();
    lightSource0->setLight( light0 );
    lightSource0->setLocalStateSetModes( osg::StateAttribute::ON );
    //See the opengl docs on the difference between ABSOLUTE and RELATIVE
    lightSource0->setReferenceFrame( osg::LightSource::RELATIVE_RF );
    
    /*m_lightModel0->setAmbientIntensity( osg::Vec4( 0.1, 0.1, 0.1, 1.0 ) );
    //Get correct specular lighting across pipes
    //See http://www.ds.arch.tue.nl/General/Staff/Joran/osg/osg_specular_problem.htm
    m_lightModel0->setLocalViewer( true );*/
    
    osg::ref_ptr< osg::StateSet > lightStateSet =
        group->getOrCreateStateSet();
    lightStateSet->setAssociatedModes( light0, osg::StateAttribute::ON );
    lightStateSet->setMode( GL_LIGHTING, osg::StateAttribute::ON );
    //lightStateSet->setAttributeAndModes(
    //                                    m_lightModel0.get(), osg::StateAttribute::ON );
    group->addChild( lightSource0 );
}
#endif

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "simple example root" );

    osg::ArgumentParser arguments( &argc, argv );
    bool useSMV = ( arguments.find( "-nosmv" ) < 1 );

    unsigned int width( 800 ), height( 600 );
    
    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 20, 30, width, height );
    
    // This is no longer the preferred way to set up lights in bdfx.
    // See the call to setLight in the backdropFXSetUp function.
    //setupLight( root.get() );
    
    backdropFXSetUp( root.get(), width, height );
    
    osg::Node* loadedModels = 0;
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(10,0,0).trans cow.osg" );
    osg::ref_ptr< osg::Group > modelParent = new osg::Group;
    modelParent->addChild( loadedModels );
    root->addChild( modelParent.get() );

    // DO NOT disable GL_LIGHT0. We have the sky dome turned off, which means there
    // is no Sun light source, so GL_LIGHT0 is the only light source by default.
    //modelParent->getOrCreateStateSet()->setMode( GL_LIGHT0, osg::StateAttribute::OFF );


    // Convert loaded data to use shader composition.
    if( useSMV )
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        //smv.setSupportSunLighting( false ); // Use shaders that support Sun lighting.

        osg::NotifySeverity ns = osg::getNotifyLevel();
        osg::setNotifyLevel( osg::INFO );

        backdropFX::convertFFPToShaderModules( root.get(), &smv );

        osg::setNotifyLevel( ns );

        backdropFX::RebuildShaderModules rsm;
        backdropFX::Manager::instance()->getManagedRoot()->accept( rsm );
    }

    viewer.realize();

    viewerSetUp( viewer, width, height, root.get() );

    //viewer.run();
    int frameNum = 0;
    while( !viewer.done() )
    {
        frameNum += 1;

        if( frameNum == 1 )
        {
            addTransparentQuad( modelParent.get() );
        }
        

        if( frameNum == 100 )
        {
            osg::ref_ptr< osg::Node > customModels = osgDB::readNodeFiles( arguments );
            if( customModels.valid() )
            {
            osg::ref_ptr< osg::Group > temp1 = new osg::Group();
            modelParent->addChild( temp1.get() );
            runSMV( temp1.get(), customModels.get() );
            }
        }
        viewer.frame();
    }
    return( 0 );
}


namespace backdropFX {


/** \page simpleexample Example: simple

The purpose of this example is to show the least amount of code required to
render in backdropFX.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>-nosmv</b></td>
    <td>Force app to not use ShaderModuleVisitor. By default, the app uses the ShaderModuleVisitor to replace FFP with Uniforms and Programs.</td>
  </tr>
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
