// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#ifndef __BACKDROPFX_DEPTH_PARTITION_H__
#define __BACKDROPFX_DEPTH_PARTITION_H__ 1

#include <backdropFX/Export.h>
#include <backdropFX/BackdropCommon.h>
#include <osg/Group>
#include <osg/FrameBufferObject>


namespace backdropFX {


/** \class backdropFX::DepthPartition DepthPartition.h backdropFX/DepthPartition.h

\brief Renders frustum in back-to-front ordered partitions.

*/
class BACKDROPFX_EXPORT DepthPartition : public osg::Group, public backdropFX::BackdropCommon
{
public:
    DepthPartition();
    DepthPartition( const DepthPartition& dp, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY );

    META_Node( backdropFX, DepthPartition );

    void traverse( osg::NodeVisitor& nv );

    /** Specifies the desired number of partitions.
       \li 0 = Let DepthPartition autocompute the required number of partitions.
       \li 1 = 1 partition
       \li 2 = 2 partitions

    etc. Default is 0. */
    void setNumPartitions( unsigned int numPartitions );
    unsigned int getNumPartitions() const;

    /** Sets the ratio to compute the near plane from the far plane.
     \c zNear \c = \c zFar \c * \c ratio. */
    void setRatio( double ratio );
    double getRatio() const;


    /** Enables for standalone use. */
    void setPrototypeHACK( bool enable=true ) { _proto = enable; }
    bool getPrototypeHACK() const { return( _proto ); }


    //
    // For internal use
    //

    void setRenderingCache( osg::Object* rc ) { _renderingCache = rc; }
    osg::Object* getRenderingCache() { return _renderingCache.get(); }
    const osg::Object* getRenderingCache() const { return _renderingCache.get(); }

    void resizeGLObjectBuffers( unsigned int maxSize );
    void releaseGLObjects( osg::State* state ) const;

protected:
    ~DepthPartition();
    void internalInit();

    unsigned int _numPartitions;
    double _ratio;

    osg::ref_ptr< osg::Object > _renderingCache;


    bool _proto;
};


// namespace backdropFX
}

// __BACKDROPFX_DEPTH_PARTITION_H__
#endif
