// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_SUN_BODY_H__
#define __BACKDROPFX_SUN_BODY_H__ 1

#include <backdropFX/Export.h>
#include <osg/Geometry>

namespace backdropFX {


/** \brief Visual representation of the Sun.

This is a custom Geometry class. The backdropFX system adds it as a
Drawable beneath the SkyDome custom Group node.

BackdropFX draws the Sun and Moon at the same distance from the viewer,
so we don't use the Sun's actual radius but instead use the Moon radius,
because the Sun and Moon have the same apparent diameter when you view them from
the Earth. However, you can change the Sun radius in the constructor, and
there is also support for a scale factor in setScale.

The Sun renders as a geodesic sphere. setSubdivisions controls the sphere
approximation. 0 subdivisions renders 20 triangles. Each successive increase in
subdivisions increases the approximation by a factor of 4.
*/
class BACKDROPFX_EXPORT SunBody : public osg::Geometry
{
public:
    SunBody( float radius = 1738.f ); // Actual Moon radiue: 1738 km
    SunBody( const SunBody& moon, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new SunBody(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new SunBody(*this,copyop); }        
    virtual bool isSameKindAs(const Object* obj) const { return dynamic_cast<const SunBody*>(obj)!=NULL; }
    virtual const char* libraryName() const { return "backdropFX"; }
    virtual const char* className() const { return "SunBody"; }

    void update();

    /** Scale factor for the Sun sphere. Default is 1.0, but some
    users might prefer larger values depending on the 
    matrix field of view projection.
    */
    void setScale( float scale );
    float getScale() const;
    /** Sun sphere subdivisions. The default is 1 subdivision or 80
    triangles, which is suitable for rendering the Sun with a scale
    factor of 1 and a typical field of view and window size.
    */
    void setSubdivisions( unsigned int sub );
    unsigned int getSubdivisions() const;

    /** This function is called by SkyDome::setSunPosition to update the Sun location
    in response to changes in time of day. Note that the Sun position is
    given in celestial coordinates, so there is no need to change the Sun
    position when there is a change in the viewer lat/long position on the
    Earth.

    \param ra Right acsension (Sun position in celestial coordinates.)
    \param dec Declination (Sun position in celestial coordinates.)
    \param distance In backdropFX, this is the radius of the sky dome.

    Set OSG_NOTIFY_LEVEL=INFO to display the parameters to the console.

    Internally, this function computes a matrix to transform the Sun from
    the origin to the specified location. This is set as a uniform, and 
	the Sun shaders use it to transform the sphere vertices.

    This function also stores the Sun transformation matrix in the
    LocationData singleton. See LocationData::storeSunMatrix. This
    lets the code query the vector to the Sun, which lighting uses. It
    also sets a vec3 sunPosition uniform to color the sky dome
    for day, dusk, night, and dawn.
    */
    void setRADecDistance( float ra, float dec, float distance );
    float getRA() const;
    float getDec() const;
    float getDistance() const;

protected:
    ~SunBody();

    void internalInit();

    bool _dirty;

    float _radius;
    float _scale;
    unsigned int _sub;
    float _ra;
    float _dec;
    float _distance;

    osg::ref_ptr< osg::Uniform > _sunTransform;
};


// namespace backdropFX
}

// __BACKDROPFX_SUN_BODY_H__
#endif
