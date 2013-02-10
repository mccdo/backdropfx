// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osg/Viewport>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <osg/DisplaySettings>
#include <osg/GraphicsContext>
#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/GLObjectsVisitor>

#include <backdropFX/Manager.h>
#include <backdropFX/DepthPartition.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/RTTViewport.h>
#include <backdropFX/EffectLibraryUtils.h>

#include <osgwTools/Shapes.h>
#include <osgwTools/ReadFile.h>
#include <osgwTools/Version.h>

#include <vector>




osg::ref_ptr< osg::GraphicsContext > gc;
osg::ref_ptr< osgViewer::GraphicsWindow > gw;
osg::ref_ptr< osgUtil::SceneView > sv;

struct ViewInfo
{
    ViewInfo( osg::Vec3& look, osg::Vec3& at, osg::Vec3& axis, osg::Matrix& proj )
      : _look( look ),
        _at( at ),
        _axis( axis ),
        _angle( 0. ),
        _proj( proj )
    {
    }

    osg::Matrix getView()
    {
        osg::Vec3 eye = _look - _at;
        osg::Vec3 newEye = eye * osg::Matrix::rotate( _angle, _axis );
        _angle += 0.018; // for next frame
        osg::Matrix view = osg::Matrix::lookAt( _at + newEye, _at, osg::Vec3( 0., 0., 1. ) );
        return( view );
    }

    osg::Vec3 _look;
    osg::Vec3 _at;
    osg::Vec3 _axis;
    float _angle;

    osg::Matrix _proj;
};
typedef std::vector< ViewInfo > ViewInfoList;
ViewInfoList vil;


void setUpViewInWindow( int x, int y, int width, int height )
{
#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
#else
    osg::DisplaySettings* ds = osg::DisplaySettings::instance();
#endif
    osg::ref_ptr< osg::GraphicsContext::Traits > traits = new osg::GraphicsContext::Traits;

    traits->readDISPLAY();
    if (traits->displayNum<0) traits->displayNum = 0;
    
    traits->screenNum = 0;
    traits->x = x;
    traits->y = y;
    traits->width = width;
    traits->height = height;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();
    if (ds->getStereo())
    {
        switch(ds->getStereoMode())
        {
            case(osg::DisplaySettings::QUAD_BUFFER): traits->quadBufferStereo = true; break;
            case(osg::DisplaySettings::VERTICAL_INTERLACE):
            case(osg::DisplaySettings::CHECKERBOARD):
            case(osg::DisplaySettings::HORIZONTAL_INTERLACE): traits->stencil = 8; break;
            default: break;
        }
    }

    osg::Camera* camera = sv->getCamera();

    gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    camera->setGraphicsContext( gc.get() );

    gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
    if (gw)
    {
        osg::notify(osg::INFO)<<"View::setUpViewOnSingleScreen - GraphicsWindow has been created successfully."<<std::endl;
        gw->getEventQueue()->getCurrentEventState()->setWindowRectangle(x, y, width, height );
    }
    else
    {
        osg::notify(osg::NOTICE)<<"  GraphicsWindow has not been created successfully."<<std::endl;
    }

    double fovy, aspectRatio, zNear, zFar;
    camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

    double newAspectRatio = double(traits->width) / double(traits->height);
    double aspectRatioChange = newAspectRatio / aspectRatio;
    if (aspectRatioChange != 1.0)
    {
        camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0);
    }

    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

    GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;

    camera->setDrawBuffer(buffer);
    camera->setReadBuffer(buffer);
}

void setUpViewAcrossAllScreens( unsigned int& widthOut, unsigned int& heightOut )
{
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) 
    {
        osg::notify(osg::NOTICE)<<"View::setUpViewAcrossAllScreens() : Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return;
    }

#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
#else
    osg::DisplaySettings* ds = osg::DisplaySettings::instance();
#endif
    osg::Camera* camera = sv->getCamera();

    double fovy, aspectRatio, zNear, zFar;        
    camera->getProjectionMatrixAsPerspective(fovy, aspectRatio, zNear, zFar);

    osg::GraphicsContext::ScreenIdentifier si;
    si.readDISPLAY();
    
    // displayNum has not been set so reset it to 0.
    if (si.displayNum<0) si.displayNum = 0;

    unsigned int numScreens = wsi->getNumScreens(si);
    osg::notify( osg::NOTICE ) << "multiview: Number of screens: " << numScreens << std::endl;

    unsigned int width( 0 ), height( 0 );
    for( unsigned int i=0; i<numScreens; i++ )
    {
        unsigned int w, h;
        wsi->getScreenResolution(si, w, h);
        width += w;
        height = osg::maximum< unsigned int >( h, height );
    }
    osg::notify( osg::NOTICE ) << "multiview: Got width: " << width << " height: " << height << std::endl;
    if( ( width > 0 ) && ( height > 0 ) )
    {
        if (si.screenNum<0) si.screenNum = 0;
    
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->hostName = si.hostName;
        traits->displayNum = si.displayNum;
        traits->screenNum = si.screenNum;
        traits->x = 0;
        traits->y = 0;
        traits->width = width;
        traits->height = height;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->windowDecoration = false;
        traits->doubleBuffer = true;
        traits->sharedContext = 0;
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        
        gc = osg::GraphicsContext::createGraphicsContext(traits.get());
        camera->setGraphicsContext(gc.get());
        
        gw = dynamic_cast<osgViewer::GraphicsWindow*>(gc.get());
        if (gw)
        {
            osg::notify(osg::INFO)<<"  GraphicsWindow has been created successfully."<<std::endl;
            gw->getEventQueue()->getCurrentEventState()->setWindowRectangle(0, 0, width, height );
        }
        else
        {
            osg::notify(osg::NOTICE)<<"  GraphicsWindow has not been created successfully."<<std::endl;
        }

        double newAspectRatio = double(traits->width) / double(traits->height);
        double aspectRatioChange = newAspectRatio / aspectRatio;
        if (aspectRatioChange != 1.0)
        {
            camera->getProjectionMatrix() *= osg::Matrix::scale(1.0/aspectRatioChange,1.0,1.0);
        }

        camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

        GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;

        camera->setDrawBuffer(buffer);
        camera->setReadBuffer(buffer);

        widthOut = width;
        heightOut = height;
    }
}

void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height )
{
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild(
        backdropFX::Manager::skyDome | backdropFX::Manager::depthPeel
        );

    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );

    backdropFX::RenderingEffects& rfx = backdropFX::Manager::instance()->getRenderingEffects();
    rfx.setEffectSet( backdropFX::RenderingEffects::effectGlow );
}

osg::Node* glowing( const std::string& fileName )
{
    osg::Node* node = osgDB::readNodeFile( fileName );

    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *node );

    osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
    std::string shaderName = "shaders/gl2/bdfx-finalize.fs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( shaderName ), shader );

    osg::StateSet* ss = node->getOrCreateStateSet();
    osg::Vec4f glowColor( 0., .4, .6, 1. );
    osg::Uniform* glow = new osg::Uniform( "bdfx_glowColor", glowColor );
    ss->addUniform( glow );

    return( node );
}

osg::Node* texturedQuad( osg::Texture2D* tex )
{
    osg::ref_ptr< osg::Geode > geode = new osg::Geode;
    geode->setName( "rttGeode" );
    geode->setNodeMask( 0x1 << 1 ); // Only displayed in 2nd pass. Kind of a hack.
    geode->setCullingActive( false );

    osg::Drawable* draw = osgwTools::makePlane( osg::Vec3( -1., -1., 0. ),
        osg::Vec3( 2., 0., 0. ), osg::Vec3( 0., 2., 0. ) );
    draw->setName( "rttTriPair" );
    draw->setUseDisplayList( false );
    draw->setUseVertexBufferObjects( true );

    geode->addDrawable( draw );

    osg::StateSet* stateSet = draw->getOrCreateStateSet();

    // Always draw. Don't even bother with the depth test.
    stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    stateSet->setTextureAttributeAndModes( 0, tex );

    // Tri pair shaders and uniform
    {
        osg::ref_ptr< osg::Program > program = new osg::Program();
        osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
        std::string shaderName( "multiviewrtt.vs" );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
        if( shader->getShaderSource().empty() )
            osg::notify( osg::WARN ) << "Can't load " << shaderName << std::endl;
        shader->setName( shaderName );
        program->addShader( shader );

        shader = new osg::Shader( osg::Shader::FRAGMENT );
        shaderName = "multiviewrtt.fs";
        shader->loadShaderSourceFromFile( osgDB::findDataFile( shaderName ) );
        if( shader->getShaderSource().empty() )
            osg::notify( osg::WARN ) << "Can't load " << shaderName << std::endl;
        shader->setName( shaderName );
        program->addShader( shader );
        stateSet->setAttribute( program.get() );

        osg::ref_ptr< osg::Uniform > uTex = new osg::Uniform( "tex", 0 );
        stateSet->addUniform( uTex.get() );
    }

    return( geode.release() );
}



int main( int argc, char ** argv )
{
    osg::ArgumentParser arguments( &argc, argv );

    unsigned int width( 0 ), height( 0 );
    bool renderToWindow( false );
    osg::notify( osg::NOTICE ) << "  -w <w> <h>\tOpen in a window. Default renders across all available displays." << std::endl;
    if( arguments.read( "-w", width, height ) )
    {
        renderToWindow = true;
        osg::notify( osg::INFO ) << "Found: -w (window mode) width: " << width << ", height: " << height << std::endl;
    }


    // Set up viewer / SceneView
    sv = new osgUtil::SceneView;
    osg::ref_ptr< osg::FrameStamp > fs = new osg::FrameStamp();
    fs->setFrameNumber( 0 );
    sv->setFrameStamp( fs.get() );

    if( renderToWindow )
        setUpViewInWindow( 20, 30, width, height );
    else
        setUpViewAcrossAllScreens( width, height );

    // 2 views. Index 0 is RTT, index 1 renders to window.
    for( unsigned int idx=0; idx<2; idx++ )
    {
        osg::Vec3 look( 0., idx==0?-16.:-24., idx==0?7.0:-14.0 );
        osg::Vec3 at( 0., 0., 0. );
        osg::Vec3 axis( 1., 0., idx==0?0.5:-0.5 );

        osg::Matrix proj( osg::Matrix::perspective( 50., (float)width/(float)height, 1., 100. ) );
        vil.push_back( ViewInfo( look, at, axis, proj ) );
        osg::notify( osg::NOTICE ) << "multiview: Added view." << std::endl;
    }
    osg::notify( osg::NOTICE ) << "multiview: width: " << width << " height: " << height << std::endl;


    // Create texture for first view to render to, and FBO to hold it.
    osg::ref_ptr< osg::Texture2D > rttTex = new osg::Texture2D;
    rttTex->setName( "rttTex" );
    rttTex->setInternalFormat( GL_RGBA );
    rttTex->setBorderWidth( 0 );
    rttTex->setTextureSize( width, height );
    rttTex->setFilter( osg::Texture::MIN_FILTER, osg::Texture::NEAREST );
    rttTex->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    osg::ref_ptr< osg::FrameBufferObject > ourFBO = new osg::FrameBufferObject;
    ourFBO->setAttachment( osg::Camera::COLOR_BUFFER0,
        osg::FrameBufferAttachment( rttTex.get() ) );


    // Set up scene graph
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setName( "Multiviewrtt test case 0" );

    osg::Node* loadedModels = osgDB::readNodeFiles( arguments );
    if( loadedModels == NULL )
        loadedModels = osgwTools::readNodeFiles( "teapot.osg.(6,0,0).trans cow.osg.(-4,0,0).trans" );
    root->addChild( loadedModels );
    root->addChild( glowing( "teapot.osg.-2,0,6.trans" ) );

    // Convert FFP to shaders
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        backdropFX::convertFFPToShaderModules( root.get(), &smv );
    }


    backdropFXSetUp( root.get(), width, height );

    // Hierarchy *above* bdfx managed root contains tri pair to display quad.
    osg::ref_ptr< osg::Group > ultRoot = new osg::Group;
    ultRoot->addChild( texturedQuad( rttTex.get() ) );
    ultRoot->addChild( backdropFX::Manager::instance()->getManagedRoot() );
    {
        backdropFX::RebuildShaderModules rsm;
        ultRoot->accept( rsm );
    }
    sv->setSceneData( ultRoot.get() );


    gc->realize();
    gc->makeCurrent();
    osg::Camera* cam = sv->getCamera();
    osg::StateSet* stateSet = cam->getOrCreateStateSet();
    stateSet->setGlobalDefaults();
    sv->setGlobalStateSet( stateSet );
    sv->setDefaults( osgUtil::SceneView::HEADLIGHT );
    gw->grabFocusIfPointerInWindow();
    sv->setState( gc->getState() );
#if( OSGWORKS_OSG_VERSION >= 20908 )
    // April 30 2010, instance() now returns a ref_ptr.
    // June 18 2010, 2.9.8 released.
    sv->setDisplaySettings( osg::DisplaySettings::instance().get() );
#else
    sv->setDisplaySettings( osg::DisplaySettings::instance() );
#endif
    sv->inheritCullSettings( *( sv->getCamera() ) );
    {
        osgUtil::GLObjectsVisitor glov;
        glov.setState( sv->getState() );
        sv->getSceneData()->accept(glov);
    }
    {
        // The SceneView camera should not be allowed to do a clear.
        cam->setClearMask( 0 );
    }

    sv->getUpdateVisitor()->setFrameStamp( fs.get() );
    for( int fc=0; fc<400; fc++ )
    {
        fs->setFrameNumber( fs->getFrameNumber() + 1 );

        glViewport( 0, 0, width, height );
        // No need to clear. Handled by bdfx SkyDome.

        // Update
        sv->getSceneData()->accept( *( sv->getUpdateVisitor() ) );

        for( unsigned int idx = 0; idx < 2; idx++ )
        {
            backdropFX::Manager::instance()->setFBO( idx==0 ? ourFBO.get() : 0 );

            // Set viewport, view matrix, and projection matrix
            cam->setViewMatrix( vil[ idx ].getView() );
            cam->setProjectionMatrix( vil[ idx ]._proj );

            // Picks up the textured tri pair only on the second pass.
            // A nicer way to do this would be to use two SceneViews.
            sv->getCullVisitor()->setTraversalMask( 0x1 << idx );

            // Get bounds, cull, and draw
            root->getBound();
            sv->cull();
            sv->draw();
        }

        // swap
        gc->swapBuffers();
    }

    return( 0 );
}



namespace backdropFX {


/** \page multiviewrtttest Test: multiviewrtt -- Support for RTT using multiple views and SceneView.

This is a modified version of the multiview test that demonstrates how the same
technique can be used for render to texture (RTT) operations.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>-w <w> <h></b></td>
    <td>Open in a window. Default renders across all available displays.</td>
  </tr>
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If you don't specify a model, this test displays the cow and teapot.</td>
  </tr>
</table>

*/


// backdropFX
}
