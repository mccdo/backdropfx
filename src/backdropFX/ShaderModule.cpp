// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <osgDB/FileUtils>
#include <osg/State>
#include <osg/Program>
#include <osg/Shader>
#include <osg/NodeVisitor>
#include <OpenThreads/ScopedLock>
#include <osgUtil/CullVisitor>

#include <backdropFX/Utils.h>


namespace backdropFX
{



// Statics
const unsigned int ShaderModuleCullCallback::InheritanceDefault( 0 );
const unsigned int ShaderModuleCullCallback::InheritanceOverride( 1 << 0 );



ShaderModuleCullCallback::ShaderModuleCullCallback()
{
}
ShaderModuleCullCallback::ShaderModuleCullCallback( const ShaderModuleCullCallback& smccb )
  : _shaderMap( smccb._shaderMap ),
    _stateSetMap( smccb._stateSetMap )
{
}
ShaderModuleCullCallback::~ShaderModuleCullCallback()
{
}

void
ShaderModuleCullCallback::operator()( osg::Node* node, osg::NodeVisitor* nv )
{
    osg::notify( osg::DEBUG_FP ) << "ShaderModuleCullCallback" << std::endl;

    osgUtil::CullVisitor* cv( dynamic_cast< osgUtil::CullVisitor* >( nv ) );

    osg::NodePath& np( nv->getNodePath() );
    osg::ref_ptr< osg::StateSet >& ss( _stateSetMap[ np ] );
    if( !( ss.valid() ) )
    {
        osg::notify( osg::NOTICE ) << "backdropFX: NULL StateSet. Should only happen if all ShaderModules are empty." << std::endl;
        osg::notify( osg::NOTICE ) << "  Could also happen if RebuildShaderModules was not run from the root node." << std::endl;
        osg::notify( osg::NOTICE ) << "  Node: " << node->getName() << ", class: " << node->className() << std::endl;
        if( !_shaderMap.empty() )
        {
            osg::notify( osg::NOTICE ) << "  Shaders: ";
            ShaderMap::const_iterator itr;
            for( itr = _shaderMap.begin(); itr != _shaderMap.end(); itr++ )
                osg::notify( osg::NOTICE ) << itr->second->getName() << " ";
            osg::notify( osg::NOTICE ) << std::endl;
        }
        traverse( node, nv );
        return;
    }

    if( false )//Manager::instance()->getDebugMode() & BackdropCommon::debugShaders )
    {
        osg::StateAttribute* sa = ss->getAttribute( osg::StateAttribute::PROGRAM );
        osg::Program* prog = dynamic_cast< osg::Program* >( sa );
        // TBD format the program name so that it's useful, or better yet format it at the time we create the name, using '\n' characters
        osg::notify( osg::ALWAYS ) << "Program name: " << prog->getName() << std::endl;
    }
#if 0
    // Trying to find a good way to dump the full shader source
    // _only_ when a program fails to link.
    osg::StateAttribute* sa = ss->getAttribute( osg::StateAttribute::PROGRAM );
    osg::Program* prog = dynamic_cast< osg::Program* >( sa );
    if( !(prog->getPCP( 0 )->isLinked()) )
    {
        osg::notify( osg::NOTICE ) << std::endl;
        osg::notify( osg::NOTICE ) << "===============" << std::endl;
        osg::notify( osg::NOTICE ) << "Failed to link:" << std::endl;
        unsigned int idx;
        for( idx=0; idx<prog->getNumShaders(); idx++ )
            dumpShaderSource( "error", prog->getShader( idx )->getShaderSource() );
    }
#endif

    cv->pushStateSet( ss.get() );
    traverse( node, nv );
    cv->popStateSet();
}

void
ShaderModuleCullCallback::setShader( const std::string& shaderSemantic, osg::Shader* shader, const unsigned int inheritance )
{
    if( shader->getType() == osg::Shader::UNDEFINED ) 
        return;

    ShaderKey key( shaderSemantic, shader->getType() );
    setShader( key, shader, inheritance );
}

void ShaderModuleCullCallback::setShader( const ShaderKey& shaderKey, osg::Shader* shader, const unsigned int inheritance )
{
    osg::ref_ptr< osg::Shader >& shaderCurrent( _shaderMap[ shaderKey ] );
    if( shaderCurrent != shader )
    {
        shaderCurrent = shader;
        _inheritanceMap[ shaderKey ] = inheritance;
    }
}

osg::Shader*
ShaderModuleCullCallback::getShader( const std::string& shaderSemantic, osg::Shader::Type type )
{
    ShaderKey key( shaderSemantic, type );
    ShaderMap::iterator it = _shaderMap.find( key );
    if( it != _shaderMap.end() )
        return( it->second.get() );
    else
        return( NULL );
}
unsigned int
ShaderModuleCullCallback::getShaderInheritance( const std::string& shaderSemantic, osg::Shader::Type type ) const
{
    ShaderKey key( shaderSemantic, type );
    InheritanceMap::const_iterator result = _inheritanceMap.find( key );
    if( result != _inheritanceMap.end() )
        return( result->second );
    else
        return( 0 );
}

bool
ShaderModuleCullCallback::removeShader( const std::string& shaderSemantic, osg::Shader::Type type )
{
    if( getShader( shaderSemantic, type ) == NULL )
        return( false );

    ShaderKey key( shaderSemantic, type );
    _shaderMap.erase( key );
    _inheritanceMap.erase( key );
    return( true );
}

void
ShaderModuleCullCallback::insertStateSet( osg::NodePath& np, osg::StateSet* ss )
{
    _stateSetMap[ np ] = ss;
}
void
ShaderModuleCullCallback::clearStateSetMap()
{
    _stateSetMap.clear();
}



/** \cond */
class ClearShaderStateSetMaps : public osg::NodeVisitor
{
public:
    ClearShaderStateSetMaps()
      : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
    {}
    ~ClearShaderStateSetMaps()
    {}

    virtual void apply( osg::Node& node )
    {
        osg::NodeCallback* nc( node.getCullCallback() );
        ShaderModuleCullCallback* smccb( dynamic_cast< ShaderModuleCullCallback* >( nc ) );
        if( smccb != NULL )
            smccb->clearStateSetMap();
        traverse( node );
    }
};
/** \endcond */



RebuildShaderModules::RebuildShaderModules( osg::NodeVisitor::TraversalMode tm )
  : osg::NodeVisitor( tm ),
    _depth( 0 )
{
}
RebuildShaderModules::~RebuildShaderModules()
{
}

void
RebuildShaderModules::reset()
{
    _shaderStateSetMap.clear();

    _depth = 0;
}

void
RebuildShaderModules::apply( osg::Node& node )
{
    if( _depth == 0 )
    {
        ClearShaderStateSetMaps csssm;
        node.accept( csssm );
    }

    osg::NodeCallback* nc( node.getCullCallback() );
    ShaderModuleCullCallback* smccb( dynamic_cast< ShaderModuleCullCallback* >( nc ) );
    if( smccb != NULL )
    {
        rebuildSource( smccb, getNodePath() );
    }

    ++_depth;
    traverse( node );
    --_depth;
}


void
RebuildShaderModules::rebuildSource( ShaderModuleCullCallback* smccb, osg::NodePath& np )
{
    ShaderModuleCullCallback::ShaderMap shaderMap;
    ShaderModuleCullCallback::InheritanceMap inheritanceMap;
    ShaderModuleCullCallback::ShaderMap::const_iterator smitr;

    // Iterate over each Node in the path, and collect the ShaderModules from
    // each into the local shaderMap.
    osg::NodePath::const_iterator nitr;
    for( nitr = np.begin(); nitr != np.end(); nitr++ )
    {
        osg::NodeCallback* ncb( (*nitr)->getCullCallback() );
        ShaderModuleCullCallback* localSmccb( dynamic_cast< ShaderModuleCullCallback* >( ncb ) );
        if( localSmccb != NULL )
        {
            // This Node in the path has an SMCCB attached to it. Now we need to iterate
            // over all shaders attached to the SMCCB and addthem to our local shaderMap.
            for( smitr = localSmccb->_shaderMap.begin(); smitr != localSmccb->_shaderMap.end(); smitr++ )
            {
                const unsigned int incomingInheritance = localSmccb->_inheritanceMap.find( smitr->first )->second;
                ShaderModuleCullCallback::InheritanceMap::iterator currentInheritance = inheritanceMap.find( smitr->first );

                // To add this shader, we need to consider two cases.
                // Case 1: We do not have a shader of this ShaderKey in our shaderMap
                // (this is the first shader with this ShaderKey we've encountered). In this
                // case, just add it, and record its inheritance bits.
                if( currentInheritance == inheritanceMap.end() )
                {
                    shaderMap[ smitr->first ] = smitr->second;
                    inheritanceMap[ smitr->first ] = incomingInheritance;
                }
                else
                // Case 2: We've already recorded a shader for this ShaderKey. In this case,
                // examine the existing inheritance bits, and only add the new shader if
                // the existing shader doesn't have inheritance set to override.
                {
                    if( currentInheritance->second != ShaderModuleCullCallback::InheritanceOverride )
                    {
                        shaderMap[ smitr->first ] = smitr->second;
                        inheritanceMap[ smitr->first ] = incomingInheritance;
                    }
                }
            }
        }
    }

    if( shaderMap.empty() )
        return;

    ShaderList sl;
    for( smitr = shaderMap.begin(); smitr != shaderMap.end(); smitr++ )
        sl.push_back( smitr->second.get() );

    osg::ref_ptr< osg::StateSet >& ss( _shaderStateSetMap[ sl ] );
    if( !( ss.valid() ) )
    {
        osg::Program* prog = new osg::Program;
        UTIL_MEMORY_CHECK( prog, "RebuildShaderModules new Program", );
        for( ShaderList::iterator slitr = sl.begin(); slitr != sl.end(); slitr++ )
        {
            osg::Shader* shader = *slitr;
            prog->addShader( shader );
            prog->setName( prog->getName() + shader->getName() + std::string( " " ) );
        }

        // TBD Double hack! This is to support bump mapping for complex surfaces.
        // a) We need a way to communicate vertex attribute locations to a program.
        // b) The locations below are hardcoded; see SurfaceUtils.cpp.
        prog->addBindAttribLocation( "rm_Tangent", 6 );
	    prog->addBindAttribLocation( "rm_Binormal", 7 );

        ss = new osg::StateSet;
        ss->setAttributeAndModes( prog, osg::StateAttribute::ON );
        osg::notify( osg::INFO ) << prog->getName() << std::endl;
    }

    smccb->insertStateSet( np, ss.get() );
}



RemoveShaderModules::RemoveShaderModules( osg::NodeVisitor::TraversalMode tm )
  : osg::NodeVisitor( tm )
{
}
RemoveShaderModules::~RemoveShaderModules()
{
}

void
RemoveShaderModules::apply( osg::Node& node )
{
    osg::NodeCallback* nc( node.getCullCallback() );
    ShaderModuleCullCallback* smccb( dynamic_cast< ShaderModuleCullCallback* >( nc ) );
    if( smccb != NULL )
    {
        osg::ref_ptr< osg::NodeCallback > nestedCallback = smccb->getNestedCallback();
        if( nestedCallback.valid() )
        {
            smccb->removeNestedCallback( nestedCallback.get() );
            node.setCullCallback( nestedCallback.get() );
        }
        else
            node.setCullCallback( NULL );
    }
    traverse( node );
}


// namespace backdropFX
}
