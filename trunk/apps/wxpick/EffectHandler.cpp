// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include "EffectHandler.h"
#include <osgWxTree/Utils.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/DepthPeelUtils.h>
#include <osgDB/FileUtils>

#include <iostream>


EffectHandler::EffectHandler( wxTreeCtrl* tree )
  : _tree( tree )
{
}
EffectHandler::~EffectHandler()
{
}

bool EffectHandler::handle( const osgGA::GUIEventAdapter& ea,
    osgGA::GUIActionAdapter& aa )
{
    bool handled = false;
    switch( ea.getEventType() )
    {
    case osgGA::GUIEventAdapter::KEYUP:
        switch( ea.getKey() )
        {
        case 'T':
        case 't':
        {
            osg::Node* currentNode = osgWxTree::getSelectedNode( _tree );
            if( currentNode != NULL )
            {
                if( osgwTools::isTransparent( currentNode->getStateSet() ) )
                {
                    backdropFX::transparentDisable( currentNode );
                }
                else
                {
                    backdropFX::transparentEnable( currentNode, .25 );
                }
                handled = true;
            }
            break;
        }
        case 'G':
        case 'g':
            osg::Node* currentNode = osgWxTree::getSelectedNode( _tree );
            if( currentNode != NULL )
            {
                enableGlow( currentNode );
                handled = true;
            }
            break;
        }
        break;
    }
    return( handled );
}


void
EffectHandler::enableGlow( osg::Node* node )
{
    osg::Group* grp = parentGroup( node );
    if( grp == NULL ) return;
    osg::StateSet* ss = grp->getOrCreateStateSet();

    backdropFX::ShaderModuleCullCallback* smccb =
        backdropFX::getOrCreateShaderModuleCullCallback( *grp );

    osg::Shader* shader = new osg::Shader( osg::Shader::FRAGMENT );
    std::string fileName = "shaders/gl2/bdfx-finalize.fs";
    shader->loadShaderSourceFromFile( osgDB::findDataFile( fileName ) );
    backdropFX::shaderPreProcess( shader );
    smccb->setShader( backdropFX::getShaderSemantic( fileName ), shader );

    osg::Vec4f glowColor( 0., .4, .6, 1. );
    osg::Uniform* glow = new osg::Uniform( "bdfx_glowColor", glowColor );
    ss->addUniform( glow );
}

osg::Group*
EffectHandler::parentGroup( osg::Node* node )
{
    if( node->asGroup() != NULL )
        return( node->asGroup() );
    else if( node->getNumParents() == 0 )
        return( NULL );
    else
        return( node->getParent( 0 ) );
}
