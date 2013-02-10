// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>

#include <osg/MatrixTransform>
#include <osg/Texture2D>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>


void
defaultScene( osg::Group* root )
{
    osg::Node* model( osgDB::readNodeFile( "teapot.osg" ) );

    // Current limitation: Shader module source must be attached to a
    // Group or Group-derived node. In the default scene, we use
    // MatrixTransforms anyhow, so this is not a problem, but is a
    // serious issue for real apps.

    osg::MatrixTransform* mt( new osg::MatrixTransform( osg::Matrix::translate( -2., 0., 0. ) ) );
    mt->addChild( model );
    root->addChild( mt );

    mt = new osg::MatrixTransform;
    mt->addChild( model );
    root->addChild( mt );

    mt = new osg::MatrixTransform( osg::Matrix::translate( 2., 0., 0. ) );
    mt->addChild( model );
    root->addChild( mt );
}


void
assignRootShader( osg::Node* node )
{
    osg::Shader* vertMain = new osg::Shader( osg::Shader::VERTEX );
    vertMain->loadShaderSourceFromFile( osgDB::findDataFile( "test-main.vs" ) );
    osg::Shader* vertLight = new osg::Shader( osg::Shader::VERTEX );
    vertLight->loadShaderSourceFromFile( osgDB::findDataFile( "testoff-lighting.vs" ) );
    osg::Shader* frag = new osg::Shader( osg::Shader::FRAGMENT );
    frag->loadShaderSourceFromFile( osgDB::findDataFile( "test-main.fs" ) );

    backdropFX::ShaderModuleCullCallback* smccb( new backdropFX::ShaderModuleCullCallback() );
    smccb->setShader( "main", vertMain );
    smccb->setShader( "lighting", vertLight );
    smccb->setShader( "main", frag );

    node->setCullCallback( smccb );
}

typedef std::vector< std::string > StringList;
StringList childVertShaders[3];
StringList childFragShaders[3];

void
initShaderLists()
{
    StringList sl;

    sl.push_back( "teston-lighting.vs" );
    childVertShaders[ 1 ] = sl;

    sl.clear();
    sl.push_back( "shaders/gl2/ffp-main.vs" );
    sl.push_back( "shaders/gl2/ffp-init.vs" );
    sl.push_back( "shaders/gl2/ffp-eyecoords-on.vs" );
    sl.push_back( "shaders/gl2/ffp-lighting-on.vs" );
    sl.push_back( "shaders/gl2/ffp-transform.vs" );
    sl.push_back( "shaders/gl2/ffp-finalize.vs" );
    childVertShaders[ 2 ] = sl;
    sl.clear();
    sl.push_back( "shaders/gl2/ffp-main.fs" );
    sl.push_back( "shaders/gl2/ffp-init.fs" );
    sl.push_back( "shaders/gl2/ffp-finalize.fs" );
    childFragShaders[ 2 ] = sl;
}

void
assignChildShader( unsigned int idx, osg::Node* node )
{
    const StringList vsl = childVertShaders[ idx ];
    const StringList fsl = childFragShaders[ idx ];

    backdropFX::ShaderModuleCullCallback* smccb = NULL;
    StringList::const_iterator slitr;
    for( slitr = vsl.begin(); slitr != vsl.end(); slitr++ )
    {
        osg::Shader* shader = new osg::Shader( osg::Shader::VERTEX );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( *slitr ) );
        backdropFX::shaderPreProcess( shader );
        if( smccb == NULL )
            smccb = backdropFX::getOrCreateShaderModuleCullCallback( *node );
        smccb->setShader( backdropFX::getShaderSemantic( *slitr ), shader );
    }
    for( slitr = fsl.begin(); slitr != fsl.end(); slitr++ )
    {
        osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
        shader->loadShaderSourceFromFile( osgDB::findDataFile( *slitr ) );
        backdropFX::shaderPreProcess( shader );
        if( smccb == NULL )
            smccb = backdropFX::getOrCreateShaderModuleCullCallback( *node );
        smccb->setShader( backdropFX::getShaderSemantic( *slitr ), shader );
    }
}

int
main( int argc, char ** argv )
{
    osg::ArgumentParser arguments( &argc, argv );

    /*
    bool renderToWindow( false );
    if( arguments.read( "-w" ) )
    {
        //renderToWindow = true;
        osg::notify( osg::NOTICE ) << "-w (render to window) not yet implemented." << std::endl;
    }
    */

    osg::ref_ptr< osg::Group > root = new osg::Group;

    {
        int idx;
        for( idx=1; idx<arguments.argc(); idx++ )
        {
            osg::ref_ptr< osg::Node > model( osgDB::readNodeFile( arguments[ idx ] ) );
            if( model.valid() )
            {
                // Current limitation: Shader module source must be attached to a
                // Group or Group-derived node.
                osg::ref_ptr< osg::Group > child = new osg::Group;
                root->addChild( child.get() );
                child->addChild( model.get() );
            }
        }
    }
    if( root->getNumChildren() == 0 )
        defaultScene( root.get() );


    initShaderLists();
    unsigned int idx;
    for( idx=0; idx<root->getNumChildren(); idx++ )
        assignChildShader( idx, root->getChild( idx ) );

    {
        // testing.
        backdropFX::ShaderModuleVisitor smv;
        root->accept( smv );

        // Dont's use the shaders assigned by ShaderModuleVisitor, use our own.
        root->setCullCallback( NULL );
        assignRootShader( root.get() );

        // Must run RebuildShaderSource any time the shader source changes:
        // Not just changing the source, but also setting the source for the
        // first time, inserting new nodes into the scene graph that have source,
        // deleteing nodes that have source, etc.
        backdropFX::RebuildShaderModules rsm;
        root->accept( rsm );
    }



    osgViewer::Viewer viewer;
    //viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );
    viewer.setSceneData( root.get() );

    viewer.addEventHandler( new osgViewer::StatsHandler );
    viewer.addEventHandler( new osgViewer::ThreadingHandler );

    return( viewer.run() );
}
