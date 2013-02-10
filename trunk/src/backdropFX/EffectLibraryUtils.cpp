// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/RenderingEffects.h>
#include <backdropFX/Manager.h>
#include <backdropFX/Effect.h>
#include <backdropFX/EffectLibrary.h>
#include <backdropFX/EffectLibraryUtils.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <osg/Program>
#include <osgDB/ReadFile>
#include <backdropFX/Utils.h>

#include <string>
#include <sstream>




namespace backdropFX
{


osg::Program*
createEffectProgram( const std::string& baseName )
{
    std::string fileName;
    const std::string namePrefix( "shaders/effects/" );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".vs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > vertShader;
    __LOAD_SHADER( vertShader, osg::Shader::VERTEX, fileName );

    {
        std::ostringstream ostr;
        ostr << namePrefix << baseName << std::string( ".fs" );
        fileName = std::string( ostr.str() );
    }

    osg::ref_ptr< osg::Shader > fragShader;
    __LOAD_SHADER( fragShader, osg::Shader::FRAGMENT, fileName );

    osg::ref_ptr< osg::Program > program = new osg::Program();
    program->setName( "Effect " + baseName );
    program->addShader( vertShader.get() );
    program->addShader( fragShader.get() );
    return( program.release() );
}


// namespace backdropFX
}
