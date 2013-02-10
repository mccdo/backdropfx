// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/SkyDome.h>
#include <backdropFX/SunBody.h>
#include <backdropFX/MoonBody.h>
#include <backdropFX/LocationData.h>

#include <osgwTools/Shapes.h>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgUtil/UpdateVisitor>
#include <osgUtil/CullVisitor>
#include <osg/MatrixTransform>
#include <osg/Quat>
#include <osg/Geode>
#include <osg/NodeCallback>
#include <osgText/Text>

#include <osgEphemeris/EphemerisEngine.h>
#include <osgEphemeris/DateTime.h>
#include <osgEphemeris/CelestialBodies.h>

#include <osg/Depth>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/AlphaFunc>
#include <osg/TextureCubeMap>

#include <backdropFX/Utils.h>
#include <osg/Config>
#include <osg/io_utils>
#include <sstream>


namespace backdropFX
{


class SkyDomeUpdateCB : public osg::NodeCallback
{
public:
    SkyDomeUpdateCB( SkyDome* sd )
      : NodeCallback(),
        _sd( sd )
    {}

    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        if( _sd->_dirty != 0 )
        {
            if( _sd->_dirty & SkyDome::RebuildDirty )
                _sd->rebuild();
            if( _sd->_dirty & SkyDome::DebugDirty )
                _sd->updateDebug();
            if( _sd->_dirty & SkyDome::LocationDataDirty )
                _sd->updateLocationData();
            _sd->_dirty = 0;
        }

        traverse( node, nv );
    }

protected:
    SkyDome* _sd;
};


class SkyDomeCullCB : public osg::NodeCallback
{
public:
    SkyDomeCullCB()
    {}

    virtual void operator()( osg::Node* node, osg::NodeVisitor* nv )
    {
        // Must be the cull visitor.
        osgUtil::CullVisitor* cv( static_cast< osgUtil::CullVisitor* >( nv ) );

        // Get the near/far mode so we can restore it later, then
        // disable it. (CullVisitor will cull if this is on and the BB
        // puts it behind the viewer, so we must turn it off before
        // traversing the sky dome elements.)
        osg::CullSettings::ComputeNearFarMode restoreMode( cv->getComputeNearFarMode() );
        cv->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );

        traverse( node, nv );

        cv->setComputeNearFarMode( restoreMode );
    }

protected:
};


class LocationCB : public backdropFX::LocationData::Callback
{
public:
    LocationCB( SkyDome* sd )
      : _sd( sd )
    {}
    ~LocationCB()
    {}

    void operator()( unsigned int changeMask )
    {
        unsigned int quickBitmaskValues = (
            backdropFX::LocationData::LatLongChanged |
            backdropFX::LocationData::EastUpChanged |
            backdropFX::LocationData::DateTimeChanged );

        if( changeMask & quickBitmaskValues  )
            _sd->_dirty |= SkyDome::LocationDataDirty;
        if( changeMask & ~quickBitmaskValues  )
            _sd->_dirty |= SkyDome::RebuildDirty;
    }

protected:
    SkyDome* _sd;
};



// Lifted directly from CullVisitor
class RenderStageCache : public osg::Object
{
public:
    RenderStageCache() {}
    RenderStageCache(const RenderStageCache&, const osg::CopyOp&) {}

    META_Object(myLib,RenderStageCache);

    void setRenderStage( osgUtil::CullVisitor* cv, SkyDomeStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        _renderStageMap[cv] = rs;
    }        

    SkyDomeStage* getRenderStage( osgUtil::CullVisitor* cv )
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        return _renderStageMap[cv].get();
    }

    typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr< SkyDomeStage > > RenderStageMap;

    OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};




// Statics
const unsigned int SkyDome::RebuildDirty( 1 << 0 );
const unsigned int SkyDome::LocationDataDirty( 1 << 1 );
const unsigned int SkyDome::DebugDirty( 1 << 2 );
const unsigned int SkyDome::AllDirty( 0xffffffff );



SkyDome::SkyDome()
  : _dirty( AllDirty ),
    _radius( 384403.f ),
    _sunScale( 1.f ),
    _sunSub( 1 ),
    _moonScale( 1.f ),
    _moonSub( 1 ),
    _enable( true )
{
    // Disable culling -- we're always visible.
    setCullingActive( false );

    // Set up our update callback.
    _updateCB = new SkyDomeUpdateCB( this );
    UTIL_MEMORY_CHECK( _updateCB.get(), "SkyDomeUpdateCB", );
    setUpdateCallback( _updateCB.get() );

    backdropFX::LocationData::s_instance()->getCallbackList().push_back(
        new LocationCB( this ) );
}

SkyDome::SkyDome( const SkyDome& skydome, const osg::CopyOp& copyop )
  : osg::Group( skydome, copyop ),
    backdropFX::BackdropCommon( skydome, copyop ),
    _dirty( skydome._dirty ),
    _radius( skydome._radius ),
    _sunScale( skydome._sunScale ),
    _sunSub( skydome._sunSub ),
    _moonScale( skydome._moonScale ),
    _moonSub( skydome._moonSub ),
    _enable( skydome._enable )
{
    backdropFX::LocationData::s_instance()->getCallbackList().push_back(
        new LocationCB( this ) );
}

SkyDome::~SkyDome()
{
}


void
SkyDome::setRadius( float radius )
{
    if( _radius != radius )
    {
        _radius = radius;
        _dirty |= RebuildDirty;
    }
}
float
SkyDome::getRadius() const
{
    return( _radius );
}

void
SkyDome::setSunScale( float sunScale )
{
    if( _sunScale != sunScale )
    {
        _sunScale = sunScale;
        _dirty |= RebuildDirty;
    }
}
float
SkyDome::getSunScale() const
{
    return( _sunScale );
}
void
SkyDome::setSunSubdivisions( unsigned int sunSub )
{
    if( _sunSub != sunSub )
    {
        _sunSub = sunSub;
        _dirty |= RebuildDirty;
    }
}
unsigned int
SkyDome::getSunSubdivisions() const
{
    return( _sunSub );
}

void
SkyDome::setMoonScale( float moonScale )
{
    if( _moonScale != moonScale )
    {
        _moonScale = moonScale;
        _dirty |= RebuildDirty;
    }
}
float
SkyDome::getMoonScale() const
{
    return( _moonScale );
}
void
SkyDome::setMoonSubdivisions( unsigned int moonSub )
{
    if( _moonSub != moonSub )
    {
        _moonSub = moonSub;
        _dirty |= RebuildDirty;
    }
}
unsigned int
SkyDome::getMoonSubdivisions() const
{
    return( _moonSub );
}

void
SkyDome::setDebugMode( unsigned int debugMode )
{
    unsigned int currentMode( getDebugMode() );
    if( currentMode != debugMode )
    {
        BackdropCommon::setDebugMode( debugMode );
        _dirty |= DebugDirty;
    }
}

void
SkyDome::useTexture( osg::TextureCubeMap* texture )
{
    _texture = texture;
    _dirty |= RebuildDirty;
}



void
SkyDome::rebuild()
{
    osg::notify( osg::DEBUG_INFO ) << "backdropFX: SkyDome::rebuild: Enter" << std::endl;

    // Delete everything.
    removeChildren( 0, getNumChildren() );
    _sunBody = NULL;
    _moonBody = NULL;


    osg::ref_ptr< osg::Geode > geode( new osg::Geode );
    UTIL_MEMORY_CHECK( geode, "SkyDome Geode", );
    geode->setName( "SkyDome Geode" );
    geode->setCullingActive( false );
    addChild( geode.get() );

    // Set the overall Group StateSet
    {
        osg::StateSet* ss = getOrCreateStateSet();
        UTIL_MEMORY_CHECK( ss, "SkyDome StateSet", );

        // Disable depth test. SkyDome uses painter's algorithm.
        osg::ref_ptr< osg::Depth > depth( new osg::Depth( osg::Depth::ALWAYS,0.0, 1.0, false) );
        UTIL_MEMORY_CHECK( depth, "SkyDome Depth", );
        ss->setAttributeAndModes( depth.get(), osg::StateAttribute::OFF |
            osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::CullFace > cf( new osg::CullFace() );
        UTIL_MEMORY_CHECK( cf.get(), "SkyDome CullFace", );
        ss->setAttributeAndModes( cf.get(), osg::StateAttribute::OFF |
            osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::AlphaFunc > af( new osg::AlphaFunc( osg::AlphaFunc::GREATER, 0.f ) );
        UTIL_MEMORY_CHECK( af.get(), "SkyDome AlphaFunc", );
        ss->setAttributeAndModes( af.get() );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "shaders/skydome.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "SkyDome vertex shader", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "shaders/skydome.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "SkyDome fragment shader", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "SkyDome Program", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        _orientationUniform = new osg::Uniform( "celestialOrientation",
            osg::Matrix::identity() );
        UTIL_MEMORY_CHECK( _orientationUniform.get(), "SkyDome orient uniform", );
        _orientationUniform->setDataVariance( osg::Object::DYNAMIC );
        ss->addUniform( _orientationUniform.get() );

        osg::Vec3 east, up;
        backdropFX::LocationData::s_instance()->getEastUp( east, up );
        up.normalize();
        osg::notify( osg::DEBUG_INFO ) << "backdropFX: SkyDome::rebuild: up: " << up << std::endl;
        osg::ref_ptr< osg::Uniform > uUp( new osg::Uniform( "up", up ) );
        UTIL_MEMORY_CHECK( uUp.get(), "SkyDome up uniform", );
        ss->addUniform( uUp.get() );

        // Sun position vector
        ss->addUniform( backdropFX::LocationData::s_instance()->getSunPositionUniform() );
    }


    // Create the sky dome sphere.
    osg::notify( osg::DEBUG_INFO ) << "backdropFX: Making SkyDome sphere with radius: " << _radius << std::endl;
    osg::ref_ptr< osg::Geometry > dome( osgwTools::makeGeodesicSphere( _radius, 2 ) );
    UTIL_MEMORY_CHECK( dome.get(), "SkyDome sphere", );
    dome->setTexCoordArray( 0, NULL );
    dome->setColorArray( NULL );
    dome->setColorBinding( osg::Geometry::BIND_OFF );
    geode->addDrawable( dome.get() );


    // Add the Sun
    _sunBody = new backdropFX::SunBody;
    UTIL_MEMORY_CHECK( _sunBody.get(), "SkyDome Sun", );
    if( _sunScale != 1.0 )
        _sunBody->setScale( _sunScale );
    _sunBody->setSubdivisions( _sunSub );
    geode->addDrawable( _sunBody.get() );
    _sunBody->update();


    // Add the Moon
    _moonBody = new backdropFX::MoonBody;
    UTIL_MEMORY_CHECK( _moonBody.get(), "SkyDome Moon", );
    if( _moonScale != 1.0 )
        _moonBody->setScale( _moonScale );
    _moonBody->setSubdivisions( _moonSub );
    geode->addDrawable( _moonBody.get() );
    _moonBody->update();

    {
        osg::StateSet* ss = _moonBody->getOrCreateStateSet();
        UTIL_MEMORY_CHECK( ss, "SkyDome Moon StateSet", );

        osg::ref_ptr< osg::BlendFunc > bf( new osg::BlendFunc( osg::BlendFunc::ONE, osg::BlendFunc::ONE ) );
        UTIL_MEMORY_CHECK( bf.get(), "SkyDome Moon BlendFunc", );
        ss->setAttributeAndModes( bf.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );
    }


    // TBD proto code for cloud cube map texture
    if( _texture != NULL )
    {
        osg::ref_ptr< osg::Geometry > clouds( osgwTools::makeGeodesicSphere( _radius, 2 ) );
        UTIL_MEMORY_CHECK( clouds.get(), "SkyDome cloud sphere", );
        clouds->setTexCoordArray( 0, NULL );
        clouds->setColorArray( NULL );
        clouds->setColorBinding( osg::Geometry::BIND_OFF );
        geode->addDrawable( clouds.get() );


        osg::StateSet* ss = clouds->getOrCreateStateSet();
        UTIL_MEMORY_CHECK( ss, "SkyDome cloud texture dome StateSet", );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "skydomeTex.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "SkyDome texture vertex shader", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "skydomeTex.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "SkyDome texture fragment shader", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "SkyDome texture Program", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        // Add texture map
        ss->setTextureAttributeAndModes( 0, _texture.get(), osg::StateAttribute::ON );

        // Uniform for texture
        osg::ref_ptr< osg::Uniform > texUniform( new osg::Uniform( "tex", 0 ) );
        UTIL_MEMORY_CHECK( texUniform.get(), "SkyDome cloud texture Uniform", );
        ss->addUniform( texUniform.get() );

        // Blending
        osg::ref_ptr< osg::BlendFunc > bf( new osg::BlendFunc );
        UTIL_MEMORY_CHECK( bf.get(), "SkyDome cloud texture BlendFunc ", );
        ss->setAttributeAndModes( bf.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );
    }


    // Cull callback to disable computation of near/far planes.
    if( _cullCB == NULL )
    {
        _cullCB = new SkyDomeCullCB();
        UTIL_MEMORY_CHECK( _cullCB.get(), "SkyDomeCullCB", );
        setCullCallback( _cullCB.get() );
    }


    // We removed everything, so if debug is enabled,
    // recreate the debug drawables.
    updateDebug();

    _dirty &= ~RebuildDirty;

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: SkyDome::rebuild: numDrawables: " << geode->getNumDrawables() << std::endl;
}

void
SkyDome::updateLocationData()
{
    osgEphemeris::DateTime localDateTime(
        backdropFX::LocationData::s_instance()->getDateTime() );
    // TBD account for time zone.
    //localDateTime.setHour( dateTime.getHour() - tz );
    osg::notify( osg::INFO ) << "backdropFX: date/time: " <<
        localDateTime.getYear() << "/" <<
        localDateTime.getMonth() << "/" <<
        localDateTime.getDayOfMonth() << " " <<
        localDateTime.getHour() << ":" <<
        localDateTime.getMinute() << ":" <<
        localDateTime.getSecond() << std::endl;

    // Position the Sun and Moon
    setSunPosition( _sunBody.get(), localDateTime );
    setMoonPosition( _moonBody.get(), localDateTime );


    // The default orientation of the celestial sphere is:
    //   0  0  1 :   90  declination
    //   0  0 -1 :  -90  declination
    //   0 -1  0 :    0h right ascension
    //   1  0  0 :    6h right ascension
    //   0  1  0 :   12h right ascension
    //  -1  0  0 :   18h right ascension

    // Get ready to rotate the celestial sphere.
    // Get local orientation information.
    osg::Vec3 east, up;
    backdropFX::LocationData::s_instance()->getEastUp( east, up );
    double latitude, longitude;
    backdropFX::LocationData::s_instance()->getLatitudeLongitude( latitude, longitude );

    // Rotate the celestial sphere.
    //   Part I: Rotate by latitude angle.
    osg::Matrix r( osg::Matrix::rotate( 
        osg::DegreesToRadians( 90.-latitude ), osg::Vec3( -1., 0., 0. ) ) );
    osg::Vec3 y( osg::Vec3( 0., 1., 0. ) * r );
    osg::Vec3 z( osg::Vec3( 0., 0., 1. ) * r );

    // Rotate the celestial sphere.
    //   Part II: Rotate by local sidereal time.
    const double mjd( localDateTime.getModifiedJulianDate( false ) );
    const double lst( osgEphemeris::EphemerisEngine::getLocalSiderealTimePrecise( mjd, -longitude ) );
    r = osg::Matrix::rotate( -lst / 12. * osg::PI, z );
    osg::Vec3 x( osg::Vec3( 1., 0., 0. ) * r );
    y = y * r;

    osg::Matrix orient( x[ 0 ], x[ 1 ], x[ 2 ], 0.,
        y[ 0 ], y[ 1 ], y[ 2 ], 0.,
        z[ 0 ], z[ 1 ], z[ 2 ], 0.,
        0., 0., 0., 1. );

    // Rotate the celestial sphere.
    //   Part III: Account for user-specified east/up
    east.normalize();
    up.normalize();
    osg::Vec3 north( up ^ east );
    osg::Matrix eastUp( east[ 0 ], east[ 1 ], east[ 2 ], 0.,
        north[ 0 ], north[ 1 ], north[ 2 ], 0.,
        up[ 0 ], up[ 1 ], up[ 2 ], 0.,
        0., 0., 0., 1. );

    // Store as Uniform for shader transformation.
    osg::Matrix m( orient * eastUp );
    _orientationUniform->set( m );

    // Store the celestial sphere matrix in the singleton LocationData.
    // Used to compute Sun position (required for lighting, lens flare, etc).
    backdropFX::LocationData::s_instance()->storeCelestialSphereMatrix( m );

    _dirty &= ~LocationDataDirty;
}

void
SkyDome::updateDebug()
{
    osg::notify( osg::DEBUG_INFO ) << "backdropFX: Debug: " << getDebugMode() << std::endl;

    if( ( getDebugMode() & backdropFX::BackdropCommon::debugVisual ) == 0 )
    {
        // Debug is disabled.
        if( getNumChildren() > 1 )
        {
            // Just use our first child and delete any other children.
            removeChildren( 1, getNumChildren()-1 );
        }
        _dirty &= ~DebugDirty;
        return;
    }

    // Debug is enabled.

    // Create a geode to own all the debug drawables.
    osg::ref_ptr< osg::Geode > geode( new osg::Geode );
    UTIL_MEMORY_CHECK( geode.get(), "SkyDome debug Geode", );
    geode->setName( "SkyDome debug Geode" );
    geode->setCullingActive( false );
    addChild( geode.get() );

    // Create the debug sky dome sphere.
    osg::ref_ptr< osg::Geometry > debugDome( osgwTools::makeWireAltAzSphere( _radius, 8, 16 ) );
    UTIL_MEMORY_CHECK( debugDome.get(), "SkyDome debug sphere", );
    debugDome->setTexCoordArray( 0, NULL );
    geode->addDrawable( debugDome.get() );

    {
        osg::StateSet* ss = debugDome->getOrCreateStateSet();
        UTIL_MEMORY_CHECK( ss, "SkyDome debug sphere StateSet", );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "shaders/skydomeDebug.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "SkyDome debug vertex shader", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "shaders/skydomeDebug.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "SkyDome debug fragment shader", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "SkyDome debug Program", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );
    }

    // Debug text labels for celestial sphere.
    osg::ref_ptr< osgText::Text > cloneMe( new osgText::Text );
    UTIL_MEMORY_CHECK( cloneMe.get(), "SkyDome text label clone", );
    cloneMe->setCharacterSize( _radius * 0.04 );
    cloneMe->setAxisAlignment( osgText::TextBase::USER_DEFINED_ROTATION );
    cloneMe->setAlignment( osgText::TextBase::CENTER_CENTER );

    {
        osg::StateSet* ss = cloneMe->getOrCreateStateSet();
        UTIL_MEMORY_CHECK( ss, "SkyDome text label StateSet", );

        // Tell the CullVisitor not to switch RenderBins, just inherit the current RenderBin.
        ss->setRenderBinDetails( 0, "", osg::StateSet::INHERIT_RENDERBIN_DETAILS );

        // Special shaders for rendering text.
        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "shaders/skydomeDebugText.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "SkyDome text label vertex shader", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "shaders/skydomeDebugText.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "SkyDome text label fragment shader", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "SkyDome text label Program", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        // Uniform for font texture
        osg::ref_ptr< osg::Uniform > texUniform( new osg::Uniform( "fontTex", 0 ) );
        UTIL_MEMORY_CHECK( texUniform.get(), "SkyDome text label tex Uniform", );
        ss->addUniform( texUniform.get() );
    }

    float decIdx;
    for( decIdx=45.f; decIdx>-46.f; decIdx-=45.f )
    {
        const double angleDec( decIdx / 180.f * osg::PI );

        int hourIdx;
        for( hourIdx=0; hourIdx<24; hourIdx+=3 )
        {
            const double angleHour( hourIdx / 12. * osg::PI );

            // Label: RA and dec as "<ra>h <dec>{n|s}"
            std::ostringstream ostr;
            ostr << hourIdx << "h " << osg::absolute<float>(decIdx) <<
                ((decIdx < 0.f) ? "s" : "n");

            // Position
            osg::Matrix m( osg::Matrix::rotate( angleDec, osg::Vec3( -1., 0., 0. ) ) *
                osg::Matrix::rotate( angleHour, osg::Vec3( 0., 0., 1. ) ) );
            const osg::Vec3 direction( osg::Vec3( 0., 1., 0. ) * m );
            const osg::Vec3 pos( direction * -_radius );

            // Rotation
            m = osg::Matrix::rotate( angleDec+osg::PI_2, osg::Vec3( 1., 0., 0. ) );
            osg::Vec3 y( osg::Vec3( 0., 1., 0. ) * m );
            osg::Vec3 z( osg::Vec3( 0., 0., 1. ) * m );
            m = osg::Matrix::rotate( angleHour-osg::PI, osg::Vec3( 0., 0., 1. ) );
            osg::Vec3 x( osg::Vec3( 1., 0., 0. ) * m );
            y = y * m;
            z = z * m;

            osg::Quat q;
            q.set( osg::Matrix( x[ 0 ], x[ 1 ], x[ 2 ], 0.,
                y[ 0 ], y[ 1 ], y[ 2 ], 0.,
                z[ 0 ], z[ 1 ], z[ 2 ], 0.,
                0., 0., 0., 1. ) );

            osg::ref_ptr< osgText::Text > label( new osgText::Text( *cloneMe ) );
            UTIL_MEMORY_CHECK( label.get(), "SkyDome text label", );
            label->setText( ostr.str() );
            label->setPosition( pos );
            label->setRotation( q );
            geode->addDrawable( label.get() );
        }
    }

    _dirty &= ~DebugDirty;
}



void
SkyDome::setSunPosition( backdropFX::SunBody* sunBody, const osgEphemeris::DateTime& dateTime )
{
    if( _cSun == NULL )
    {
        _cSun = new osgEphemeris::Sun;
        UTIL_MEMORY_CHECK( _cSun.get(), "SkyDome", );
    }

    _cSun->updatePosition( dateTime.getModifiedJulianDate( false ) );
    double ra, dec;
    _cSun->getPos( &ra, &dec );
    sunBody->setRADecDistance( ra / osg::PI * 12.0,
        osg::RadiansToDegrees( dec ), _radius );
}
void
SkyDome::setMoonPosition( backdropFX::MoonBody* moonBody, const osgEphemeris::DateTime& dateTime )
{
    if( _cMoon == NULL )
    {
        _cMoon = new osgEphemeris::Moon;
        UTIL_MEMORY_CHECK( _cMoon.get(), "SkyDome", );
    }

    double latitude, longitude;
    backdropFX::LocationData::s_instance()->getLatitudeLongitude( latitude, longitude );

    const double mjd( dateTime.getModifiedJulianDate( false ) );
    const double lst( osgEphemeris::EphemerisEngine::getLocalSiderealTimePrecise( mjd, -longitude ) );
    _cMoon->updatePosition( mjd, lst, latitude, _cSun.get() );
    double ra, dec;
    _cMoon->getPos( &ra, &dec );
    moonBody->setRADecDistance( ra / osg::PI * 12.0,
        osg::RadiansToDegrees( dec ), _radius );
}


void
SkyDome::traverse( osg::NodeVisitor& nv )
{
    if( ( nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR ) )
    {
        // Not the cull visitor. Traverse as a Group and return.
        osg::Group::traverse( nv );
        return;
    }

    // Dealing with cull traversal from this point onwards.
    osgUtil::CullVisitor* cv = static_cast< osgUtil::CullVisitor* >( &nv );

    // BackdropCommon cull processing.
    processCull( cv );

    // Get the updates view/proj UniformBundle.
    // Push the StateSet (for this CullVisitor). It contains the
    // updates viewProj matrix uniform.
    UniformBundle* ub( updateViewProjUniforms( cv ) );
    cv->pushStateSet( ub->first.get() );


    //
    // Basic idea of what follows was derived from CullVisitor::apply( Camera& ).
    //

    osgUtil::RenderStage* previousStage = cv->getCurrentRenderBin()->getStage();
    osg::Camera* camera = previousStage->getCamera();

    osg::ref_ptr< RenderStageCache > rsCache = dynamic_cast< RenderStageCache* >( getRenderingCache() );
    if( !rsCache )
    {
        rsCache = new RenderStageCache;
        UTIL_MEMORY_CHECK( rsCache, "SkyDome SkyDomeStage Cache", );
        setRenderingCache( rsCache.get() );
    }

    osg::ref_ptr< SkyDomeStage > sds = rsCache->getRenderStage( cv );
    if( !sds )
    {
        sds = new SkyDomeStage( *previousStage );
        UTIL_MEMORY_CHECK( rsCache, "SkyDome SkyDomeStage", );
        sds->setBackdropCommon( this );
        rsCache->setRenderStage( cv, sds.get() );
    }
    else
    {
        // Reusing custom RenderStage. Reset it to clear previous cull's contents.
        sds->reset();
    }
    sds->setEnable( getEnable() );

    sds->setViewport( cv->getCurrentCamera()->getViewport() );

    // PSC TBD. I don't think we need this.
    //sds->setInheritedPositionalStateContainer( previousStage->getPositionalStateContainer() );


    {
        // Save RenderBin
        osgUtil::RenderBin* previousRenderBin = cv->getCurrentRenderBin();
        cv->setCurrentRenderBin( sds.get() );

        // Traverse
        osg::Group::traverse( nv );

        // Restore RenderBin
        cv->setCurrentRenderBin( previousRenderBin );
    }


    // Hook our RenderStage into the render graph.
    cv->getCurrentRenderBin()->getStage()->addPreRenderStage( sds.get(), camera->getRenderOrderNum() );

    // Pop the StateSet (viewProj uniform)
    cv->popStateSet();
}


void
SkyDome::resizeGLObjectBuffers( unsigned int maxSize )
{
    if( _renderingCache.valid() )
        const_cast< SkyDome* >( this )->_renderingCache->resizeGLObjectBuffers( maxSize );

    osg::Group::resizeGLObjectBuffers(maxSize);
}

void
SkyDome::releaseGLObjects( osg::State* state ) const
{
    if( _renderingCache.valid() )
        const_cast< SkyDome* >( this )->_renderingCache->releaseGLObjects( state );

    osg::Group::releaseGLObjects(state);
}


SkyDome::UniformBundle*
SkyDome::updateViewProjUniforms( osgUtil::CullVisitor* cv )
{
    UniformBundle* ub;
    {
        OpenThreads::ScopedLock< OpenThreads::Mutex > lock( _stateSetMapLock );
        StateSetPerCull::iterator it( _stateSetPerCull.find( cv ) );
        if( it == _stateSetPerCull.end() )
        {
            ub = &( _stateSetPerCull[ cv ] );
            ub->first = new osg::StateSet;
            UTIL_MEMORY_CHECK( ub->first.get(), "SkyDome UniformBundle StateSet", ub );
            ub->second = new osg::Uniform( osg::Uniform::FLOAT_MAT4, "viewProj" );
            UTIL_MEMORY_CHECK( ub->second, "SkyDome UniformBundle Uniform", ub );
            ub->second->setDataVariance( osg::Object::DYNAMIC );
            ub->first->addUniform( ub->second );
        }
        else
            ub = &( it->second );
    }

    // Set the matrices
    {
        // Create view matrix, discarding eye position.
        osg::Matrix view( *( cv->getModelViewMatrix() ) );
        view(3,0) = view(3,1) = view(3,2) = 0.0;

        // Create projection with constant near/far
        osg::Matrix proj( *( cv->getProjectionMatrix() ) );
        double left, right, top, bottom, znear, zfar;
        proj.getFrustum( left, right, bottom, top, znear, zfar );
        zfar = getRadius() * 1.2;
        const double newNear = zfar / 2000.;
        const double nearScale( newNear / znear );
        left *= nearScale;
        right *= nearScale;
        bottom *= nearScale;
        top *= nearScale;
        proj = osg::Matrix::frustum( left, right, bottom, top, newNear, zfar );

        ub->second->set( view * proj );
    }

    return( ub );
}


// namespace backdropFX
}
