// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osg/MatrixTransform>

#include <osgwTools/Shapes.h>
#include <osgwTools/Version.h>

#include "DepthPartition.h"

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>


const float winW( 1024.f ), winH( 768.f );

backdropFX::DepthPartition* depthPartition( NULL );
osg::MatrixTransform* sepTransform( NULL );
float sep( 1.f );


class MyKeyHandler : public osgGA::GUIEventHandler
{
public:
    MyKeyHandler() {}
    
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
    {
        bool newMatrix( false );
        bool handled( false );
        int numPartitions = -1;

        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            // 'D' -- increase sep between test quads
            // 'd' -- decrease sep
            case 'D':
                sep += 5.;
                newMatrix = true;
                break;
            case 'd':
                sep -= 5.;
                newMatrix = true;
                break;

            default:
                // 1,2,3 -- force n depth partition passes
                // '0' -- autodetermine depth partition passes
                if( ( ea.getKey() >= '0' ) && ( ea.getKey() < '9' ) )
                    numPartitions = ea.getKey() - '0';
                break;
            }
            break;
        }
        default:
            break;
        }

        if( newMatrix )
        {
            osg::Matrix m = osg::Matrix::translate( 0., sep, 0. );
            sepTransform->setMatrix( m );
            handled = true;

            osg::notify( osg::NOTICE ) << "Plane separation: " << sep << std::endl;
        }
        if( numPartitions >= 0 )
        {
            depthPartition->setNumPartitions( numPartitions );
            handled = true;

            osg::notify( osg::NOTICE ) << "Number of partitions: " << numPartitions << std::endl;
        }

        return( handled );
    }
};



osg::Node*
makeDefaultScene( const float span, const float w, const float h )
{
    osg::Group* root = new osg::Group;
    const float halfW = w * .5;
    const float halfH = h * .5;
    const float halfSpan = span * .5;

    osg::Geometry* geom = osgwTools::makePlane( osg::Vec3( -halfW*1.1, halfSpan, -halfH*1.1 ),
        osg::Vec3( w, 0., 0. ), osg::Vec3( 0., 0., h ) );
    osg::Vec4Array* c = new osg::Vec4Array;
    c->push_back( osg::Vec4( 1., 0., 0., 1. ) );
    geom->setColorArray( c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( geom );
    root->addChild( geode );

    geom = osgwTools::makePlane( osg::Vec3( -halfW, halfSpan, -halfH ),
        osg::Vec3( w, 0., 0. ), osg::Vec3( 0., 0., h ) );
    c = new osg::Vec4Array;
    c->push_back( osg::Vec4( 0., 1., 0., 1. ) );
    geom->setColorArray( c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );

    sepTransform = new osg::MatrixTransform( osg::Matrix::translate( 0., sep, 0. ) );
    geode = new osg::Geode;
    geode->addDrawable( geom );
    sepTransform->addChild( geode );
    root->addChild( sepTransform );

    osg::MatrixTransform* cowTransform = new osg::MatrixTransform( osg::Matrix::translate( 0., -sep, 0. ) );
    cowTransform->addChild( osgDB::readNodeFile( "cow.osg" ) );
    root->addChild( cowTransform );

    root->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    return( root );
}

void
assignRootShader( osg::Node* node )
{
    backdropFX::ShaderModuleCullCallback* smccb =
        dynamic_cast< backdropFX::ShaderModuleCullCallback* >
        ( node->getCullCallback() );

    osg::Shader* vertTransform = new osg::Shader( osg::Shader::VERTEX );
    vertTransform->loadShaderSourceFromFile( osgDB::findDataFile( "shaders/gl2/bdfx-transform.vs" ) );
    backdropFX::shaderPreProcess( vertTransform );

    smccb->setShader( "transform", vertTransform );
}


int
main( int argc, char ** argv )
{
    osg::ref_ptr< osg::Group > root = new osg::Group;

    depthPartition = new backdropFX::DepthPartition;
    root->addChild( depthPartition );
    depthPartition->addChild( makeDefaultScene( 1000., 50., 50. ) );

    {
        //osg::setNotifyLevel( osg::DEBUG_INFO );
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( true );
        smv.setAttachTransform( true );
        backdropFX::convertFFPToShaderModules( root.get(), &smv );

        assignRootShader( root.get() );

        backdropFX::RebuildShaderModules rsm;
        root->accept( rsm );
        //osg::setNotifyLevel( osg::NOTICE );
    }

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 10, 30, winW, winH );
    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new MyKeyHandler );
    viewer.setSceneData( root.get() );

    //viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.getCamera()->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setProjectionMatrixAsPerspective( 15., winW/winH, 0.0000001, 10000. );
    viewer.getCamera()->setClearMask( 0 );

    return( viewer.run() );
}
