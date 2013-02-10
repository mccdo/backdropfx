// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/TextureCubeMap>
#include <osg/FrameBufferObject>

#include <backdropFX/Manager.h>
#include <backdropFX/SkyDome.h>
#include <backdropFX/LocationData.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>

#include <osgwTools/CameraConfigObject.h>
#include <osgDB/FileUtils>


// TBD proto code
osg::TextureCubeMap*
createClouds()
{
    osg::ref_ptr< osg::TextureCubeMap > cloudMap( new osg::TextureCubeMap );
    cloudMap->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
    cloudMap->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );

    std::string fileName( "sky-cmap-skew-matrix-03_cuberight.tif" );
    osg::ref_ptr< osg::Image > image( osgDB::readImageFile( fileName ) );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::POSITIVE_X, image.get() );

    fileName = "sky-cmap-skew-matrix-03_cubeleft.tif";
    image = osgDB::readImageFile( fileName );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::NEGATIVE_X, image.get() );

    fileName = "sky-cmap-skew-matrix-03_cubefront.tif";
    image = osgDB::readImageFile( fileName );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::POSITIVE_Y, image.get() );

    fileName = "sky-cmap-skew-matrix-03_cubeback.tif";
    image = osgDB::readImageFile( fileName );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::NEGATIVE_Y, image.get() );

    fileName = "sky-cmap-skew-matrix-03_cubetop.tif";
    image = osgDB::readImageFile( fileName );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::POSITIVE_Z, image.get() );

    fileName = "sky-cmap-skew-matrix-03_cubebottom.tif";
    image = osgDB::readImageFile( fileName );
    if( image == NULL )
        return( NULL );
    cloudMap->setImage( osg::TextureCubeMap::NEGATIVE_Z, image.get() );

    return( cloudMap.release() );
}


void
backdropFXSetUp( osg::Node* root, unsigned int width, unsigned int height,
                bool useShaderModule )
{
    //
    // Initialize the backdropFX Manager.
    backdropFX::Manager::instance()->setSceneData( root );
    backdropFX::Manager::instance()->rebuild( backdropFX::Manager::skyDome );

    // TBD don't RTT yet. Get this working in a window first.
    // backdropFX::Manager::instance()->setFBO( createDestinationFBO() );
    // TBD When we do RTT, it is app responsibility to splat an fstp to display the output image.
    //   TBD Maybe backdropFX should contain a convenience mechanism for this.

    // With a reasonably large FOV in the window, the sun and moon typically
    // appear too small. Scale them up to make them more visible.
    backdropFX::SkyDome& sd = backdropFX::Manager::instance()->getSkyDome();
    sd.setSunScale( 2. );
    sd.setMoonScale( 2. );
    sd.setDebugMode( backdropFX::BackdropCommon::debugVisual );

    // TBD Proto code
    //sd->useTexture( createClouds() );

    // Depth partitioning
    backdropFX::DepthPartition& dPart = backdropFX::Manager::instance()->getDepthPartition();
    dPart.setNumPartitions( 1 );


    // Must always explicitly set the width and height of the rendered area.
    backdropFX::Manager::instance()->setTextureWidthHeight( width, height );
}

void
viewerSetUp( osgViewer::Viewer& viewer, double aspect, osg::Node* root )
{
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );

    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrix(
        osg::Matrix::perspective( 35., aspect, .01, 100000. ) );

    viewer.getCamera()->setClearMask( 0 );

    viewer.setSceneData( backdropFX::Manager::instance()->getManagedRoot() );

    osgGA::TrackballManipulator* tb = new osgGA::TrackballManipulator();
    viewer.setCameraManipulator( tb );
    tb->setNode( root );
    tb->home( 0 );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );
}

int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;

    osg::ref_ptr< osg::Node > lz( osgDB::readNodeFile( "lzhack.osg" ) );
    root->addChild( lz.get() );

    // Disable GL_LIGHT0, leaving only the Sun enabled. This is kind of a hack,
    // as we currently can't disable this on the root node, have to disable it on
    // a child node.
    lz->getOrCreateStateSet()->setMode( GL_LIGHT0, osg::StateAttribute::OFF );

    bool useShaderModule( true );
    if( useShaderModule )
    {
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( false ); // Use bdfx-main
        smv.setAttachTransform( false ); // Use bdfx-transform
        smv.setSupportSunLighting( true ); // Use shaders that light from the Sun
        root->accept( smv );
    }

    osgViewer::Viewer viewer;
    unsigned int width, height;
    if( true )
    {
        width = 800;
        height = 600;
        viewer.setUpViewInWindow( 30, 30, width, height );
    }
    else
    {
        osgwTools::configureViewer( viewer );
        //viewer.setUpViewAcrossAllScreens();
        viewer.realize();
        if( ( viewer.getCamera() ) && ( viewer.getCamera()->getViewport() ) )
        {
            const osg::Viewport* vp = viewer.getCamera()->getViewport();
            width = (unsigned int) vp->width();
            height = (unsigned int) vp->height();
        }
        else
        {
            osg::notify( osg::ALWAYS ) << "No camera" << std::endl;
            width = 1920;
            height = 1200;
        }
        osg::notify( osg::ALWAYS ) << "  Using width " << width << " and height " << height << std::endl;
    }

    backdropFXSetUp( root.get(), width, height, useShaderModule );
    viewerSetUp( viewer, (double)width/(double)height, root.get() );

    // Set the time and location info.
    int day( 21 ), hour( 5 ), minute( 0 ), sec( 0 );
    osgEphemeris::DateTime dateTime( 2010, 1, day, hour, minute, sec );
    backdropFX::LocationData::s_instance()->setDateTime( dateTime );
    backdropFX::LocationData::s_instance()->setLatitudeLongitude( 39.86, -104.68 );
    backdropFX::LocationData::s_instance()->setEastUp(
        osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 0, 1. ) );

    while( !viewer.done() )
    {
        if( (sec += 15) >= 60 )
        {
            sec -= 60;
            if( ++minute >= 60 )
            {
                minute -= 60;
                if( ++hour >= 24 )
                {
                    hour -= 24;
                    day++;
                    dateTime.setDayOfMonth( day );
                }
                dateTime.setHour( hour );
            }
            dateTime.setMinute( minute );
        }
        dateTime.setSecond( sec );
        backdropFX::LocationData::s_instance()->setDateTime( dateTime );

        viewer.frame();
    }

    // Cleanup and exit.
    backdropFX::Manager::instance( true );
    return( 0 );
}


namespace backdropFX {


/** \page skydometest Test: skydome

The purpose of this test is to verify correct sky dome rendering and animation.

This test enables debug mode in the SkyDome class and sets it to debugVisual. This renders
a celestial sphere grid to display right ascension and declination. The test animates at the
rate of 15 seconds per frame, showing the rotation of the celestial sphere, and the gradual
change in position of the Moon over time.

This test loads the a modified version of the \c lz.osg model file called \c lzhack.osg.
The model is lit from the Sun. (The modified version is necessary because the original
version disabled lighting and did not contain normals.)

\section clp Command Line Parameters
The \c skydome test doesn't support any command line options.

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler

\section ss Screen Shots

\image html fig05-skydome.jpg

Initial view, showing the \c lzhack.osg model with the sky dome rendered behind.
Rendered with depthVisual enabled, the sky dome shows grid lines and test
tags to indicate location on the celestial sphere. "9h 45n", for example,
means "right ascension 9 hours, declination 45 degrees north".

\image html fig06-skydome.jpg

View changed to look at the West, showing sunset.

*/


// backdropFX
}
