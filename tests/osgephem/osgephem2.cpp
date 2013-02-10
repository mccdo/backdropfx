// Copyright 2010 Skew Matrix Software. All rights reserved.

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgEphemeris/EphemerisModel.h>

#include <osgEphemeris/EphemerisEngine.h>

#include <rndr/MoonBody.h>


int
main( int argc,
      char ** argv )
{
    {
        osgEphemeris::EphemerisData* ed( new( std::string("shmemfile") ) osgEphemeris::EphemerisData );
        osg::ref_ptr< osgEphemeris::EphemerisEngine > ee( new osgEphemeris::EphemerisEngine( ed ) );

        std::cout << std::endl;
        std::cout << "Test 1: 1 Jan 2000, noon, Greenwish obs" << std::endl;
        ee->setDateTime( osgEphemeris::DateTime( 2000, 1, 1, 12, 0 ) );
        ee->setLatitudeLongitude( 51.5, 0. );
        ee->update( false );

        std::cout << std::endl;
        std::cout << "Test 2: 11 Jan 2010 (today), 9:00am, Denver" << std::endl;
        ee->setDateTime( osgEphemeris::DateTime( 2010, 1, 11, 9, 0 ) );
        ee->setLatitudeLongitude( 39.86, -104.68 ); // Denver Intl Airport
        ee->update( false );

        std::cout << std::endl;
        std::cout << "Test 3: 21 June 2009 (summer soltice), noon, Denver" << std::endl;
        ee->setDateTime( osgEphemeris::DateTime( 2009, 6, 21, 12, 0 ) );
        ee->update( false );

        std::cout << std::endl;
        std::cout << "Test 4: 21 June 2009 (summer soltice), noon, Greenwich Obs" << std::endl;
        ee->setLatitudeLongitude( 51.5, 0. );
        ee->update( false );

        std::cout << std::endl;
        std::cout << "Test 5: 21 March 2010 (vernal equinox), noon, Greenwich Obs" << std::endl;
        ee->setDateTime( osgEphemeris::DateTime( 2010, 3, 21, 12, 0 ) );
        ee->update( false );

        return( 0 );
    }

    osg::ref_ptr< osg::Group > root = new osg::Group;
    root->addChild( osgDB::readNodeFile( "lz.osg" ) );

    osg::ref_ptr< osgEphemeris::EphemerisModel > em = new osgEphemeris::EphemerisModel;
    em->setLatitudeLongitude( 40., -113 );
    osgEphemeris::DateTime* dt = new osgEphemeris::DateTime( 2010, 1, 1, 12, 0 );
    em->setDateTime( *dt );
    root->addChild( em.get() );

    osgViewer::Viewer viewer;
    viewer.getCamera()->setClearColor( osg::Vec4( 0., 0., 0., 0. ) );
    viewer.setSceneData( root.get() );
    return( viewer.run() );
}
