// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_MOON_BODY_H__
#define __BACKDROPFX_MOON_BODY_H__ 1

#include <backdropFX/Export.h>
#include <osg/Geometry>

namespace backdropFX {


class BACKDROPFX_EXPORT MoonBody : public osg::Geometry
{
public:
    MoonBody( float radius = 1738.f ); // Actual Moon radiue: 1738 km
    MoonBody( const MoonBody& moon, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    virtual osg::Object* cloneType() const { return new MoonBody(); }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new MoonBody(*this,copyop); }        
    virtual bool isSameKindAs(const Object* obj) const { return dynamic_cast<const MoonBody*>(obj)!=NULL; }
    virtual const char* libraryName() const { return "backdropFX"; }
    virtual const char* className() const { return "MoonBody"; }

    void update();

    void setScale( float scale );
    float getScale() const;
    void setSubdivisions( unsigned int sub );
    unsigned int getSubdivisions() const;

    void setRADecDistance( float ra, float dec, float distance );
    float getRA() const;
    float getDec() const;
    float getDistance() const;

protected:
    ~MoonBody();

    void internalInit();

    bool _dirty;

    float _radius;
    float _scale;
    unsigned int _sub;
    float _ra;
    float _dec;
    float _distance;

    osg::ref_ptr< osg::Uniform > _moonTransform;
    osg::ref_ptr< osg::Uniform > _moonOrient;
};


// namespace backdropFX
}

// __BACKDROPFX_MOON_BODY_H__
#endif
