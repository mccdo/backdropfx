// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/TextureCubeMap>
#include <osg/ClearNode>
#include <osg/BlendFunc>
#include <osgwTools/Shapes.h>


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


int
main( int argc,
      char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;
    //root->addChild( osgDB::readNodeFile( "cow.osg" ) );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( osgwTools::makeGeodesicSphere( 10. ) );
    root->addChild( geode );

    geode->getOrCreateStateSet()->setTextureAttributeAndModes( 0, createClouds() );
    geode->getOrCreateStateSet()->setAttributeAndModes( new osg::BlendFunc );
    geode->getOrCreateStateSet()->setMode( GL_CULL_FACE, osg::StateAttribute::OFF );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    osg::ClearNode* cn = new osg::ClearNode;
    cn->setClearColor( osg::Vec4( .2, .4, 1., 1. ) );
    root->addChild( cn );

    osgViewer::Viewer viewer;
    viewer.setSceneData( root.get() );
    return( viewer.run() );
}

