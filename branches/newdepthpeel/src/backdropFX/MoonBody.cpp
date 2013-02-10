// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/MoonBody.h>
#include <osgwTools/Shapes.h>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/CullFace>
#include <osg/Texture2D>
#include <backdropFX/Utils.h>

#include <osg/io_utils>


namespace backdropFX {


MoonBody::MoonBody( float radius )
  : _dirty( true ),
    _radius( radius ),
    _scale( 1.f ),
    _sub( 1 ),
    _ra( 0.f ),
    _dec( 0.f ),
    _distance( 384403.f )
{
}

MoonBody::MoonBody( const MoonBody& moon, const osg::CopyOp& copyop )
  : _dirty( true ),
    _radius( moon._radius ),
    _scale( moon._scale ),
    _sub( moon._sub ),
    _ra( moon._ra ),
    _dec( moon._dec ),
    _distance( moon._distance )
{
}

MoonBody::~MoonBody()
{
}


void
MoonBody::update()
{
    if( _dirty )
    {
        internalInit();
        _dirty = false;
    }
}

void
MoonBody::setScale( float scale )
{
    if( _scale != scale )
    {
        _scale = scale;
        _dirty = true;
    }
}
float
MoonBody::getScale() const
{
    return( _scale );
}
void
MoonBody::setSubdivisions( unsigned int sub )
{
    if( _sub != sub )
    {
        _sub = sub;
        _dirty = true;
    }
}
unsigned int
MoonBody::getSubdivisions() const
{
    return( _sub );
}

void
MoonBody::setRADecDistance( float ra, float dec, float distance )
{
    if( (_ra != ra) || (_dec != dec) || (_distance != distance) )
    {
        _ra = ra;
        _dec = dec;
        _distance = distance;
        osg::notify( osg::INFO ) << "backdropFX: Moon " << _ra << "h " << dec << ", " << distance << std::endl;

        double moonRotateZ = (12. - _ra) / -12. * osg::PI;
        double moonRotateX = _dec / 180. * osg::PI;
        osg::Matrix m( osg::Matrix::rotate( moonRotateX, osg::Vec3( 1., 0., 0. ) ) *
            osg::Matrix::rotate( moonRotateZ, osg::Vec3( 0., 0., 1. ) ) );

        osg::Matrix3 moonOrient( m(0,0), m(0,1), m(0,2),
            m(1,0), m(1,1), m(1,2),
            m(2,0), m(2,1), m(2,2) );
        if( _moonOrient == NULL )
        {
            _moonOrient = new osg::Uniform( "moonOrientation", moonOrient );
            UTIL_MEMORY_CHECK( _moonOrient.get(), "MoonBody moon orientation", );
        }
        else
            _moonOrient->set( moonOrient );

        osg::Matrix moonMatrix( osg::Matrix::translate( osg::Vec3( 0., _distance, 0. ) ) * m );
        if( _moonTransform == NULL )
        {
            _moonTransform = new osg::Uniform( "moonTransform", moonMatrix );
            UTIL_MEMORY_CHECK( _moonTransform.get(), "MoonBody moon transform", );
        }
        else
            _moonTransform->set( moonMatrix );
    }
}
float
MoonBody::getRA() const
{
    return( _ra );
}
float
MoonBody::getDec() const
{
    return( _dec );
}
float
MoonBody::getDistance() const
{
    return( _distance );
}


void
MoonBody::internalInit()
{
    if( getName().empty() )
        setName( "Moon Geometry" );

    // Delete everything.
    setStateSet( NULL );
    getPrimitiveSetList().clear();

    // Create the sphere.
    osgwTools::makeGeodesicSphere( _radius * _scale, _sub, this );

    // Add cylindrical texture coords
    // Because this is a geodesic sphere instead of a lat/long sphere,
    // this will result in a wrapping seam on the back side of the Moon.
    // Should never be visible to an Earth-bound viewer.
    {
        osg::ref_ptr< osg::Vec3Array > verts( dynamic_cast< osg::Vec3Array* >( getVertexArray() ) );
        unsigned int numVerts( verts->size() );
        osg::ref_ptr< osg::Vec2Array > tc( new osg::Vec2Array );
        tc->resize( numVerts );
        setTexCoordArray( 0, tc.get() );

        const float invRadius( 1.f / (_radius * _scale) );
        unsigned int idx;
        for( idx=0; idx<numVerts; idx++ )
        {
            osg::Vec3 v( (*verts)[ idx ] * invRadius );
            osg::Vec2 v2( v.x(), v.y() );
            v2.normalize();
            osg::Vec2& coord( (*tc)[ idx ] );

            // s comes from the angle between the 2D vertex and the +y vector, sweeping through -x first.
            const osg::Vec2 posY( 0, 1 );
            double angle( acos( v2 * posY ) );
            if( v.x() > 0. )
                coord[ 0 ] = ( osg::PI * 2. ) - angle;
            else
                coord[ 0 ] = angle;
            coord[ 0 ] /= ( osg::PI * 2.);

            // t comes from the angle between the vertex and the -z vector.
            const osg::Vec3 negZ( 0, 0, -1 );
            angle = ( acos( v * negZ ) );
            coord[ 1 ] = angle / osg::PI;
        }
    }

    {
        osg::StateSet* ss( getOrCreateStateSet() );
        UTIL_MEMORY_CHECK( ss, "MoonBody", );

        osg::ref_ptr< osg::CullFace > cf = new osg::CullFace;
        UTIL_MEMORY_CHECK( cf.get(), "MoonBody", );
        ss->setAttributeAndModes( cf.get(), osg::StateAttribute::ON | osg::StateAttribute::PROTECTED );

        osg::ref_ptr< osg::Shader > vertShader( osg::Shader::readShaderFile(
            osg::Shader::VERTEX, osgDB::findDataFile( "shaders/moon.vs" ) ) );
        UTIL_MEMORY_CHECK( vertShader.get(), "MoonBody", );
        osg::ref_ptr< osg::Shader > fragShader( osg::Shader::readShaderFile(
            osg::Shader::FRAGMENT, osgDB::findDataFile( "shaders/moon.fs" ) ) );
        UTIL_MEMORY_CHECK( fragShader.get(), "MoonBody", );

        osg::ref_ptr< osg::Program > program( new osg::Program() );
        UTIL_MEMORY_CHECK( program.get(), "MoonBody", );
        program->addShader( vertShader.get() );
        program->addShader( fragShader.get() );
        ss->setAttribute( program.get(), osg::StateAttribute::ON |
            osg::StateAttribute::PROTECTED );

        // Add texture map
        const std::string fileName( "moon.jpg" );
        osg::ref_ptr< osg::Image > image( osgDB::readImageFile( fileName ) );
        UTIL_MEMORY_CHECK( image.get(), "MoonBody texture Image", );
        if( image == NULL )
        {
            osg::notify( osg::WARN ) << "backdropFX: Moon: Can't open data file " << fileName << std::endl;
            return;
        }

        osg::ref_ptr< osg::Texture2D > tex( new osg::Texture2D() );
        UTIL_MEMORY_CHECK( tex.get(), "MoonBody", );
        tex->setImage( image.get() );
        ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

        // Uniform for texture
        osg::ref_ptr< osg::Uniform > texUniform( new osg::Uniform( "tex", 0 ) );
        UTIL_MEMORY_CHECK( texUniform.get(), "MoonBody", );
        ss->addUniform( texUniform.get() );

        // Moon transform uniforms
        if( _moonTransform == NULL )
            setRADecDistance( 0., 0., 0. );
        ss->addUniform( _moonTransform.get() );
        ss->addUniform( _moonOrient.get() );
    }
}


// namespace backdropFX
}
