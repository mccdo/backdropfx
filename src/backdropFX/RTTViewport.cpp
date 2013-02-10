// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <backdropFX/RTTViewport.h>


namespace backdropFX
{


RTTViewport::RTTViewport()
  : osg::Viewport()
{
    internalInit();
}

RTTViewport::RTTViewport( osg::Viewport::value_type x, osg::Viewport::value_type y,
                         osg::Viewport::value_type width, osg::Viewport::value_type height )
  : osg::Viewport( x, y, width, height )
{
    internalInit();
}
RTTViewport::RTTViewport( const RTTViewport& vp, const osg::CopyOp& copyop )
  : osg::Viewport( vp, copyop )
{
    internalInit();
}
RTTViewport::RTTViewport( const osg::Viewport& vp )
  : osg::Viewport( vp )
{
    internalInit();
}


void RTTViewport::apply( osg::State& state ) const
{
    glViewport( (GLint)0, (GLint)0,
        static_cast< GLsizei >( _width ), static_cast< GLsizei >( _height ) );
}
void RTTViewport::applyFullViewport( osg::State& ) const
{
    glViewport( static_cast< GLint >( _xFull ), static_cast< GLint >( _yFull ),
        static_cast< GLsizei >( _width ), static_cast< GLsizei >( _height ) );
}


void RTTViewport::internalInit()
{
    _xFull = _x;
    _yFull = _y;
    _x = _y = static_cast< osg::Viewport::value_type >( 0 );
}


// namespace backdropFX
}
