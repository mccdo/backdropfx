// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/SunBody.h>
#include <osgwTools/Shapes.h>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/CullFace>
#include <osg/Texture2D>

#include <backdropFX/LocationData.h>

#include <backdropFX/Utils.h>
#include <osg/io_utils>


namespace backdropFX {


SunBody::SunBody( float radius )
  : _dirty( true ),
    _radius( radius ),
    _scale( 1.f ),
    _sub( 1 ),
    _ra( 0.f ),
    _dec( 0.f ),
    _distance( 384403.f )
{
}

SunBody::SunBody( const SunBody& moon, const osg::CopyOp& copyop )
  : _dirty( true ),
    _radius( moon._radius ),
    _scale( moon._scale ),
    _sub( moon._sub ),
    _ra( moon._ra ),
    _dec( moon._dec ),
    _distance( moon._distance )
{
}

SunBody::~SunBody()
{
}


void
SunBody::update()
{
    if( _dirty )
    {
        internalInit();
        _dirty = false;
    }
}

void
SunBody::setScale( float scale )
{
    if( _scale != scale )
    {
        _scale = scale;
        _dirty = true;
    }
}
float
SunBody::getScale() const
{
    return( _scale );
}
void
SunBody::setSubdivisions( unsigned int sub )
{
    if( _sub != sub )
    {
        _sub = sub;
        _dirty = true;
    }
}
unsigned int
SunBody::getSubdivisions() const
{
    return( _sub );
}

void
SunBody::setRADecDistance( float ra, float dec, float distance )
{
    if( (_ra != ra) || (_dec != dec) || (_distance != distance) )
    {
        _ra = ra;
        _dec = dec;
        _distance = distance;
        osg::notify( osg::INFO ) << "backdropFX: Sun " << _ra << "h " << dec << ", " << distance << std::endl;

        double sunRotateZ = (12. - _ra) / -12. * osg::PI;
        double sunRotateX = _dec / 180. * osg::PI;
        osg::Matrix sunMatrix( osg::Matrix::translate( osg::Vec3( 0., _distance, 0. ) ) *
            osg::Matrix::rotate( sunRotateX, osg::Vec3( 1., 0., 0. ) ) *
            osg::Matrix::rotate( sunRotateZ, osg::Vec3( 0., 0., 1. ) ) );

        // Set the LocationData singleton sun matrix.
        backdropFX::LocationData::s_instance()->storeSunMatrix( sunMatrix );

        // Set the sunTransform uniform.
        if( _sunTransform == NULL )
        {
            _sunTransform = new osg::Uniform( "sunTransform",
                osg::Matrix::identity() );
            UTIL_MEMORY_CHECK( _sunTransform.get(), "SunBody", );
        }
        _sunTransform->set( sunMatrix );
    }
}
float
SunBody::getRA() const
{
    return( _ra );
}
float
SunBody::getDec() const
{
    return( _dec );
}
float
SunBody::getDistance() const
{
    return( _distance );
}


void
SunBody::internalInit()
{
    if( getName().empty() )
        setName( "Sun Geometry" );

    // Delete everything.
    setStateSet( NULL );
    getPrimitiveSetList().clear();

    // Create the sphere.
    osgwTools::makeGeodesicSphere( _radius * _scale, _sub, this );
    setTexCoordArray( 0, NULL );

    {
        osg::StateSet* ss( getOrCreateStateSet() );
        UTIL_MEMORY_CHECK( ss, "SunBody", );

        osg::ref_ptr< osg::CullFace > cf = new osg::CullFace;
        UTIL_MEMORY_CHECK( cf.get(), "SunBody", );
        ss->setAttributeAndModes( cf.get(), osg::StateAttribute::ON | osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "shaders/sun.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "SunBody", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "shaders/sun.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "SunBody", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "SunBody", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        // Sun transform uniform
        if( _sunTransform == NULL )
            setRADecDistance( 0., 0., 0. );
        ss->addUniform( _sunTransform.get() );
    }
}


// namespace backdropFX
}
