// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_RTT_VIEWPORT_H__
#define __BACKDROPFX_RTT_VIEWPORT_H__ 1

#include <backdropFX/Export.h>
#include <osg/Viewport>


namespace backdropFX {
/** \class backdropFX::RTTViewport RTTViewport.h backdropFX/RTTViewport.h

    \brief A viewport that always uses xy=(0,0).

RTTViewport facilitates render-to-texture (RTT) without requiring an osg::Camera
node. Code that internally renders to texture requires xy=(0,0) for efficient use
of GPU memory by minimizing texture width and height. If the application uses an
RTTViewport class when specifying the top-level (SceneView or osgViewer) Camera
viewport, then internal render-to-texture will always use xy=(0,0) for the viewport.
Internal code can access the actual xy values with the xFull() and yFull() methods,
and apply the viewport with the actual xy values by calling applyFullViewport().

This is not a core OSG class, so any core OSG code always gets xy=(0,0). This
includes the RenderStage scissor rectangle application, RenderStage FBO render buffer
creation, and other OSG code that applies the viewport via apply() or accesses the
viewport xy values directly.

Internally, RTTViewport uses these member variables:
 <table border="0">
 <tr>
 <td><b>_x </b></td>
 <td>Always zero</td>
 </tr>
 <tr>
 <td><b>_y </b></td>
 <td>Always zero</td>
 </tr>
 <tr>
 <td><b> _xFull </b></td>   
 <td>Viewport x offset specified in the constructor</td>
 </tr>
 <tr>
 <td><b> _yFull </b></td>
 <td>Viewport y offset specified in the constructor</td>
 </tr>
 <tr>
 <td><b> _width </b></td>
 <td>Viewport width specified in the constructor</td>
 </tr>
 <tr>
 <td><b> _height </b></td>
<td> Viewport height specified in the constructor</td>
</tr>
</table>

Note: RTTViewport doesn't support the osg::Viewport::setViewport() interface,
nor does it support external code that writes directly to the non-const
references that the x() and y() methods return. The setViewport() method is
declared inlined, and not virtual, and derived classes can't override it.
Use the setRTTViewport() method instead of setViewport().
If you call setViewport() on an RTTViewport instance or store values in the
x() or y() references, incorrect rendering results.
*/
class BACKDROPFX_EXPORT RTTViewport : public osg::Viewport
{
public :
    RTTViewport();
    RTTViewport( osg::Viewport::value_type x, osg::Viewport::value_type y,
                 osg::Viewport::value_type width, osg::Viewport::value_type height );
    RTTViewport( const RTTViewport& vp, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );
    RTTViewport( const osg::Viewport& vp );

    META_StateAttribute( backdropFX, RTTViewport, osg::StateAttribute::VIEWPORT );

    /** Apply the (0,0,_width,_height) viewport. */
    virtual void apply( osg::State& state ) const;

    /** Return the x offset, which is specified in the constructor.*/
    inline value_type& xFull() { return _xFull; }

    /** Return the x offset, which is specified in the constructor.*/
    inline value_type xFull() const { return _xFull; }

    /** Return the y offset, which is specified in the constructor.*/
    inline value_type& yFull() { return _yFull; }

    /** Return the y offset, which is specified in the constructor.*/
    inline value_type yFull() const { return _yFull; }

    /** Apply the (_xFull,_yFull,_width,_height) viewport. */
    virtual void applyFullViewport( osg::State& ) const;

    /** Set the viewport.
    Note that the xy offset sets the _xFull and _yFull values without altering the _x and _y values. */
    inline void setRTTViewport( value_type x, value_type y, value_type width, value_type height )
    {
        _xFull = x;
        _yFull = y;
        _width = width;
        _height = height;
    }

protected:
    /** The RTTViewport constructors call this function to save the given xy values as
    _xFull and _yFull, then zero the _x and _y variables.
    */
    void internalInit();

    value_type _xFull;
    value_type _yFull;
};


// namespace backdropFX
}

// __BACKDROPFX_RTT_VIEWPORT_H__
#endif
