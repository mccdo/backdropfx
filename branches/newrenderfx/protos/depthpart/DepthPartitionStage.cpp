// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include "DepthPartitionStage.h" //<backdropFX/DepthPartitionStage.h>
#include "DepthPartition.h" //<backdropFX/DepthPartition.h>
#include <osgUtil/RenderStage>
#include <osg/GLExtensions>
#include <osg/FrameBufferObject>
#include <osg/StateSet>
#include <osg/Uniform>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Version.h>

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

    osg::Matrixf m;
    _partitionMatrix = new osg::Uniform( "bdfx_partitionMatrix", m );
    _stateSet->addUniform( _partitionMatrix.get() );
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

    osg::notify( osg::DEBUG_INFO ) << "backdropFX: DepthPartitionStage::draw" << std::endl;


    osg::State& state( *renderInfo.getState() );
    const unsigned int contextID( state.getContextID() );
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );

    const osg::Viewport* vp = dynamic_cast< const osg::Viewport* >(
        state.getLastAppliedAttribute( osg::StateAttribute::VIEWPORT ) );


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
    UTIL_GL_ERROR_CHECK( "DepthPartitionStage" );

    vp->apply( state );


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
    unsigned int numPartitions = this->_depthPartition->getNumPartitions();
    if( numPartitions == 0 )
    {
        // App has requested that we compute the best number of partitions.
        double tempFar = inFar;
        do {
            numPartitions++;
            tempFar /= 2000.;
        } while( tempFar > inNear );
        osg::notify( osg::DEBUG_FP ) << "Computed partitions: " << numPartitions << std::endl;
    }
    else
        osg::notify( osg::DEBUG_FP ) << "Using partitions: " << numPartitions << std::endl;


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

            // TBD allow a customizable ratio, instead of hard coded 2000.
            double newNear = osg::maximum< double >( tempFar / 2000., inNear );

            double newLeft, newRight, newBottom, newTop;
            computeFrustum( newLeft, newRight, newBottom, newTop, newNear,
                inLeft, inRight, inBottom, inTop, inNear );

            osg::Matrixf m = osg::Matrix::frustum( newLeft, newRight, newBottom, newTop, newNear, tempFar );
            _partitionMatrix->set( m );

            tempFar = newNear;
        }

        // Render child stages.
        drawPreRenderStages( renderInfo, previous );

        // Don't need this when it's a parent of DepthPeel
        // but need it for a standalone test app
        drawInner( renderInfo, previous, doCopyTexture );

        glClear( GL_DEPTH_BUFFER_BIT );
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
        std::string msg( "at SRS draw end" );
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
