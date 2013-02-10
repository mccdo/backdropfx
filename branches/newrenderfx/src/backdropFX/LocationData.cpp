// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/LocationData.h>

#include <backdropFX/Utils.h>
#include <OpenThreads/ScopedLock>

#include <osg/io_utils>


namespace backdropFX
{


// Statics
LocationData* LocationData::_s_instance( NULL );
OpenThreads::Mutex LocationData::_s_instanceLock;

const unsigned int LocationData::LatLongChanged( 1 << 0 );
const unsigned int LocationData::EastUpChanged( 1 << 1 );
const unsigned int LocationData::DateTimeChanged( 1 << 2 );


backdropFX::LocationData*
LocationData::s_instance()
{
    OpenThreads::ScopedLock< OpenThreads::Mutex > lock( _s_instanceLock );

    if( _s_instance == NULL )
    {
        _s_instance = new LocationData;
        UTIL_MEMORY_CHECK( _s_instance, "LocationData instance", NULL );
    }
    return( _s_instance );
}
LocationData::LocationData()
{
    // Default time is noon on 1/1/2011. Note time is specified in absolute GMT.
    int year( 2011 ), month( 1 ), day( 1 );
    int hour( 6 ), min( 0 ), sec( 0 );
    osgEphemeris::DateTime dateTime( year, month, day, hour, min, sec );
    setDateTime( dateTime );

    // Default lat/long is ISU campus.
    setLatitudeLongitude( 42.04444, -93.65 );
    setEastUp( osg::Vec3( 1., 0., 0. ), osg::Vec3( 0., 0, 1. ) );
}
LocationData::~LocationData()
{
}


void
LocationData::setLatitudeLongitude( double latitude, double longitude )
{
    if( ( _latitude != latitude ) || ( _longitude != longitude ) )
    {
        _latitude = latitude;
        _longitude = longitude;
        osg::notify( osg::INFO ) << "backdropFX: latitude: " << _latitude << ", longitude: " << _longitude << std::endl;
        internalInit( LatLongChanged );
    }
}
void
LocationData::getLatitudeLongitude( double& latitude, double& longitude ) const
{
    latitude = _latitude;
    longitude = _longitude;
}
void
LocationData::setEastUp( const osg::Vec3& east, const osg::Vec3& up )
{
    if( ( _east != east ) || ( _up != up ) )
    {
        _east = east;
        _up = up;
        osg::notify( osg::INFO ) << "backdropFX: east: " << _east << ", up: " << _up << std::endl;

        // Data integrity check.
        if( osg::absolute( east * up ) > 0.02 )
            osg::notify( osg::WARN ) << "backdropFX: east and up vectors are not at a 90 degree angle." << std::endl;

        internalInit( EastUpChanged );
    }
}
void
LocationData::getEastUp( osg::Vec3& east, osg::Vec3& up ) const
{
    east = _east;
    up = _up;
}
void
LocationData::setDateTime( const osgEphemeris::DateTime& dateTime )
{
    // TBD
    //if( _dateTime != dateTime )
    {
        _dateTime = dateTime;
        internalInit( DateTimeChanged );
    }
}
osgEphemeris::DateTime
LocationData::getDateTime() const
{
    return( _dateTime );
}


void
LocationData::storeSunMatrix( osg::Matrix& m )
{
    _sunMatrix = m;
    computeSunPosition();
}
void
LocationData::storeCelestialSphereMatrix( osg::Matrix& m )
{
    _celestialSphereMatrix = m;
    computeSunPosition();
}
void
LocationData::computeSunPosition()
{
    _pos = osg::Vec3( 0., 1., 0. ) * _sunMatrix * _celestialSphereMatrix;
    _pos.normalize();
    osg::notify( osg::DEBUG_INFO ) << "backdropFX: LocationData::SunPosition: " << _pos << std::endl;
    _uSunPosition->set( _pos );
}

osg::Vec3
LocationData::getSunPositionVector() const
{
    return( _pos );
}
osg::Uniform*
LocationData::getSunPositionUniform()
{
    return( _uSunPosition.get() );
}

void
LocationData::internalInit( unsigned int changed )
{
    if(  _uSunPosition == NULL )
    {
        _uSunPosition = new osg::Uniform( "bdfx_sunPosition", _pos );
        UTIL_MEMORY_CHECK( _uSunPosition.get(), "LocationData Sun position uniform", );
    }

    if( changed != 0 )
    {
        CallbackList::iterator it;
        for( it=_cbList.begin(); it!=_cbList.end(); it++ )
            (*(*it))( changed );
    }
}



LocationData::Callback::Callback()
{
}

LocationData::CallbackList&
LocationData::getCallbackList()
{
    return( _cbList );
}



// namespace backdropFX
}
