// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/DepthPartitionStage.h>
#include <backdropFX/DepthPartition.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <backdropFX/DepthPeelBin.h>
#include <osg/StateSet>
#include <osg/Uniform>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Version.h>
#include <osg/Timer>

#include <backdropFX/Utils.h>
#include <string>



namespace backdropFX
{


DepthPartitionStage::DepthPartitionStage()
  : osgUtil::RenderStage(),
    _depthPartition( NULL )
{
    internalInit();
}

DepthPartitionStage::DepthPartitionStage( const osgUtil::RenderStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _depthPartition( NULL )
{
    internalInit();
}
DepthPartitionStage::DepthPartitionStage( const DepthPartitionStage& rhs, const osg::CopyOp& copyop )
  : osgUtil::RenderStage( rhs ),
    _depthPartition( rhs._depthPartition )
{
    internalInit();
}

DepthPartitionStage::~DepthPartitionStage()
{
}


void
DepthPartitionStage::internalInit()
{
    setClearMask( 0 );

    // Create StateSet and uniform for sending the projection matrix for
    // each partition.
    _stateSet = new osg::StateSet;
    UTIL_MEMORY_CHECK( _stateSet.get(), "DepthPartitionStage::internalInit StateSet", )

    // The partition matrix is used instead of the projection matrix to
    // transform from eye space into clip coords.
    _partitionMatrix = new osg::Uniform( osg::Uniform::FLOAT_MAT4, "bdfx_partitionMatrix" );
    UTIL_MEMORY_CHECK( _partitionMatrix.get(), "DepthPartitionStage::internalInit _partitionMatrix", )
    _stateSet->addUniform( _partitionMatrix.get() );

    // this vec4 color is set slightly different for each partition pass
    // and added into the resulting color. Odd numbered passes as reddish.
    _partitionDebug = new osg::Uniform( osg::Uniform::FLOAT_VEC4, "bdfx_partitionDebug" );
    UTIL_MEMORY_CHECK( _partitionDebug.get(), "DepthPartitionStage::internalInit _partitionDebug", )
    _stateSet->addUniform( _partitionDebug.get() );
}


inline void computeFrustum(
    // Returned values:
    double& newLeft, double& newRight, double& newBottom, double& newTop,
    // Input values:
    const double newNear,
    const double inLeft, const double inRight, const double inBottom, const double inTop, const double inNear )
{
    const double ratio = newNear / inNear;
    newLeft = inLeft * ratio;
    newRight = inRight * ratio;
    newBottom = inBottom * ratio;
    newTop = inTop * ratio;

    osg::notify( osg::DEBUG_FP ) << "newNear: " << newNear << std::endl;
    osg::notify( osg::DEBUG_FP ) << "  Params: " <<
        newLeft << ", " << newRight << ", " << newBottom << ", " << newTop << std::endl;
}


void
DepthPartitionStage::draw( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    if( _stageDrawnThisFrame )
        return;
    _stageDrawnThisFrame = true;

    osg::notify( osg::DEBUG_FP ) << "backdropFX: DepthPartitionStage::draw" << std::endl;
    UTIL_GL_ERROR_CHECK( "DepthPartitionStage draw start" );


    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    if( fboExt == NULL )
    {
        osg::notify( osg::WARN ) << "backdropFX: SRS: FBOExtensions == NULL." << std::endl;
        return;
    }


    // Bind the FBO.
    osg::FrameBufferObject* fbo( _depthPartition->getFBO() );
    if( fbo != NULL )
    {
        fbo->apply( state );
    }
    else
    {
        osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0 );
    }
    UTIL_GL_ERROR_CHECK( "DepthPartitionStage post FBO" );

    state.applyAttribute( getViewport() );


    //
    // Render the input texture image to clear the depth buffer.

    _depthPartition->performClear( renderInfo );

    // Done rendering input texture image.
    //


    // Get the overall projection parameters.
    osg::Matrixd projD = _camera->getProjectionMatrix();
    double inLeft, inRight, inBottom, inTop, inNear, inFar;
    projD.getFrustum( inLeft, inRight, inBottom, inTop, inNear, inFar );

    // Get the desired number of partitions.
    const double ratio = _depthPartition->getRatio();
    unsigned int numPartitions = _depthPartition->getNumPartitions();
    const bool autoCompute = ( numPartitions == 0 );
    if( autoCompute )
    {
        // App has requested that we compute the best number of partitions.
        double tempFar = inFar;
        do {
            numPartitions++;
            tempFar *= ratio;
        } while( tempFar > inNear );
        osg::notify( osg::DEBUG_FP ) << "Computed partitions: " << numPartitions << std::endl;
    }
    else
        osg::notify( osg::DEBUG_FP ) << "Using partitions: " << numPartitions << std::endl;

    const bool dumpImages = (
        ( _depthPartition->getDebugMode() & backdropFX::BackdropCommon::debugImages ) != 0 );


    //
    // Draw loop

    double tempFar = inFar;
    bool doCopyTexture( false );
    unsigned int idx;
    for( idx=0; idx<numPartitions; idx++ )
    {
        {
            // Create the projection matrix for this partition and set
            // as the bdfx_partitionMatrix uniform.

            double newNear;
            if( numPartitions == 1 )
                // Just one partitions, so do the whole view volume.
                newNear = inNear;
            else
            {
                newNear = tempFar * ratio;
                // if numPartitions is 0 (auto computed), we're done.

                if( ( !autoCompute ) && ( newNear < inNear ) )
                {
                    // If numPartitions was specified (greater than 1) and our
                    // new near plane is closer (less than) the view volume near,
                    // clamp to the view volume near and stop looping.
                    newNear = inNear;
                    idx = numPartitions - 1;
                }
            }
            osg::notify( osg::DEBUG_FP ) << "  backdropFX: DepthPartitionStage pass " << idx << ", far " << tempFar << " near " << newNear << std::endl;

            double newLeft, newRight, newBottom, newTop;
            computeFrustum( newLeft, newRight, newBottom, newTop, newNear,
                inLeft, inRight, inBottom, inTop, inNear );

            osg::Matrixf m = osg::Matrix::frustum( newLeft, newRight, newBottom, newTop, newNear, tempFar );
            _partitionMatrix->set( m );

            tempFar = newNear;
        }

        // TBD should be tied to debug.
        // Debugging aid: Add a pinkish tink to odd partitions.
        // This is done in bdfx-finalize.fs.
        if( idx & 1 )
            _partitionDebug->set( osg::Vec4f( .5f, 0.f, 0.f, 0.f ) );
        else
            _partitionDebug->set( osg::Vec4f( 0.f, 0.f, 0.f, 0.f ) );

        if( dumpImages )
        {
            // If dumping images, pass the current partition number to the DepthPeelBin.
            // It encodes the partition number in the dumped image file name.
            osgUtil::RenderBin::RenderBinList::iterator rb = _bins.find( 0 );
            if( rb != _bins.end() )
            {
                backdropFX::DepthPeelBin* dpb = dynamic_cast< backdropFX::DepthPeelBin* >( rb->second.get() );
                if( dpb != NULL )
                {
                    dpb->setPartitionNumber( idx );
                    osg::notify( osg::INFO ) << "  Setting partNum: " << idx << std::endl;
                }
            }
        }

        // Render child stages.
        drawPreRenderStages( renderInfo, previous );

        // Normally, this is a no-op, as there are no direct children to
        // DepthPartition, only the DepthPeel node (which is rendered in the
        // drawPreRenderStages() call above). However, if DepthPeel is disabled,
        // then the scene is attached directly to DepthPartition as a child
        // Group, and in thin case drawImplementation() renders the scene.
        //osg::Timer timerA;
        RenderBin::drawImplementation( renderInfo, previous );
        //osg::notify( osg::ALWAYS ) << "DepthPart drawImpl: " << timerA.time_s() << std::endl;

        if( _depthPartition->getPrototypeHACK() )
        {
            // Don't need this when it's a parent of DepthPeel
            // but need it for a standalone test app
            drawInner( renderInfo, previous, doCopyTexture );
        }

        UTIL_GL_ERROR_CHECK( "depth partition inner draw loop" );
    }

    // End of draw loop
    //


    // Unbind
    // If backdropFX is configured to render to window (no destination specified by app)
    // then this is the last code to execute as a prerender stage of the top-level
    // Camera's RenderStage. (RenderFX is drawn as post-render, in that case.)
    // After we finish here and return, one of the first things RenderStage will do 
    // is call glDrawBuffer(GL_BACK). We must unbind the FBO here, otherwise the
    // glDrawBuffer call will generate an INVALID_OPERATION error.
    if( fbo != NULL )
    {
        osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0);
    }

    if( state.getCheckForGLErrors() != osg::State::NEVER_CHECK_GL_ERRORS )
    {
        std::string msg( "at DPRS draw end" );
        UTIL_GL_ERROR_CHECK( msg );
        UTIL_GL_FBO_ERROR_CHECK( msg, fboExt );
    }
}

void
DepthPartitionStage::setDepthPartition( DepthPartition* depthPartition )
{
    _depthPartition = depthPartition;
}

osg::StateSet*
DepthPartitionStage::getPerCullStateSet()
{
    return( _stateSet.get() );
}



// namespace backdropFX
}
