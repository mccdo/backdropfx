// Copyright (c) 2011 Skew Matrix Software. All rights reserved.

#include <backdropFX/LightInfo.h>
#include <osg/Texture2D>
#include <osg/FrameBufferObject>

#include <backdropFX/Utils.h>


namespace backdropFX
{

    
LightInfo::LightInfo()
  : _enable( false )
{
}
LightInfo::LightInfo( const LightInfo& rhs, const osg::CopyOp& copyop )
  : _light( rhs._light ),
    _enable( rhs._enable )
{
}



// namespace backdropFX
}
