// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>

#include <backdropFX/MoonBody.h>


int
main( int argc,
      char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->setCullingActive( false );

    osg::ref_ptr< osg::Geode > geode = new osg::Geode;
    geode->setCullingActive( false );
    root->addChild( geode.get() );

    osg::ref_ptr< backdropFX::MoonBody > moon( new backdropFX::MoonBody( 1 ) );
    moon->setSubdivisions( 2 );
    moon->setRADecDistance( 12., 0., 0. );
    geode->addDrawable( moon.get() );

    osg::Uniform* viewProjUniform;
    {
        osg::StateSet* ss( root->getOrCreateStateSet() );

        osg::Matrix m = osg::Matrix::identity();
        viewProjUniform = new osg::Uniform( "viewProj", m );
        viewProjUniform->setDataVariance( osg::Object::DYNAMIC );
        ss->addUniform( viewProjUniform );

        m = osg::Matrix::identity();
        osg::Uniform* orientUniform = new osg::Uniform( "celestialOrientation", m );
        ss->addUniform( orientUniform );

        osg::Uniform* moonxformUniform = new osg::Uniform( "moonTransform", m );
        ss->addUniform( moonxformUniform );

        osg::Uniform* sunPosUniform = new osg::Uniform( "sunPosition",
            osg::Vec3( -1., 0., 0. ) );
        ss->addUniform( sunPosUniform );

        osg::Matrix3 mo;
        osg::Uniform* moonOrientation = new osg::Uniform( "moonOrientation", mo );
        ss->addUniform( moonOrientation );
    }
    moon->update(); // must call before rendering.

    osgViewer::Viewer viewer;
    viewer.setCameraManipulator( new osgGA::TrackballManipulator() );
    viewer.getCamera()->setClearColor( osg::Vec4( 0., 0., 0., 0. ) );
    viewer.setSceneData( root.get() );

    while( !viewer.done() )
    {
        viewProjUniform->set( viewer.getCamera()->getViewMatrix() * 
            viewer.getCamera()->getProjectionMatrix() );
        viewer.frame();
    }
    return( 0 );
}
