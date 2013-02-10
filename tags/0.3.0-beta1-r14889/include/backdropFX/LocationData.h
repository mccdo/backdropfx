// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_LOCATION_DATA_H__
#define __BACKDROPFX_LOCATION_DATA_H__ 1

#include <backdropFX/Export.h>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osgEphemeris/DateTime.h>

#include <vector>


namespace backdropFX
{


/** \class backdropFX::LocationData LocationData.h backdropFX/LocationData.h

\li Singleton for tracking information about the current viewer
position and ground orientation. 
\li Several modules within the rendering 
system access it. 
\li Sun position is set internally by SkyDome. */
class BACKDROPFX_EXPORT LocationData
{
public:
    static backdropFX::LocationData* s_instance();

    void setLatitudeLongitude( double latitude, double longitude );
    void getLatitudeLongitude( double& latitude, double& longitude ) const;

    void setEastUp( const osg::Vec3& east, const osg::Vec3& up );
    void getEastUp( osg::Vec3& east, osg::Vec3& up ) const;

    void setDateTime( const osgEphemeris::DateTime& dateTime );
    osgEphemeris::DateTime getDateTime() const;


    static const unsigned int LatLongChanged;
    static const unsigned int EastUpChanged;
    static const unsigned int DateTimeChanged;

    /** \class backdropFX::LocationData::Callback LocationData.h backdropFX/LocationData.h
    \brief Executed in response to LocationData changes.
    */
    class BACKDROPFX_EXPORT Callback : public osg::Referenced
    {
    public:
        Callback();

        virtual void operator()( unsigned int changeMask ) = 0;
    };
    typedef std::vector< osg::ref_ptr< Callback > > CallbackList;

    CallbackList& getCallbackList();


    /** Not for apps to call; used internally for 
    lighting, shadows, lens flare, etc. */
    void storeSunMatrix( osg::Matrix& m );
    void storeCelestialSphereMatrix( osg::Matrix& m );

    osg::Vec3 getSunPositionVector() const;
    osg::Uniform* getSunPositionUniform();

protected:
    LocationData();
    ~LocationData();

    void internalInit( unsigned int changed );
    void computeSunPosition();

    double _latitude, _longitude;
    osg::Vec3 _east, _up;
    osgEphemeris::DateTime _dateTime;

    CallbackList _cbList;

    osg::Matrix _sunMatrix, _celestialSphereMatrix;
    osg::Vec3 _pos;
    osg::ref_ptr< osg::Uniform > _uSunPosition;

    static LocationData* _s_instance;

    static OpenThreads::Mutex _s_instanceLock;
};


// namespace backdropFX
}

// __BACKDROPFX_LOCATION_DATA_H__
#endif
