// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifdef __APPLE__
#  ifndef _DARWIN
#    define _DARWIN 1
#  endif
#endif

#include <osgEphemeris/EphemerisEngine.h>
#include <osg/Math>


int
main( int argc,
      char ** argv )
{
    double lo( 0. );
    osgEphemeris::DateTime dt( 2010, 1, 27, 12, 0, 0 );
    double mjd = dt.getModifiedJulianDate();
    double lst = osgEphemeris::EphemerisEngine::getLocalSiderealTimePrecise( mjd, 0. );
    std::cout << std::endl;
    std::cout << "Test 1: 27 Jan 2010, noon, mjd: " << mjd << ", longitude: " << lo << std::endl;
    std::cout << "LST: " << lst << std::endl;

    lo = -5.;
    lst = osgEphemeris::EphemerisEngine::getLocalSiderealTimePrecise( mjd, -5. );
    std::cout << std::endl;
    std::cout << "Test 2: 27 Jan 2010, noon, mjd: " << mjd << ", longitude: " << lo << std::endl;
    std::cout << "LST: " << lst << std::endl;

    lo = 0.;
    dt = osgEphemeris::DateTime( 2000, 1, 1, 12, 0, 0 );
    mjd = dt.getModifiedJulianDate();
    lst = osgEphemeris::EphemerisEngine::getLocalSiderealTimePrecise( mjd, -5. );
    std::cout << std::endl;
    std::cout << "Test 3: 1 Jan 2000, noon, mjd: " << mjd << ", longitude: " << lo << std::endl;
    std::cout << "LST: " << lst << std::endl;

    return( 0 );
}
