// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/Manager.h>
#include <osgwTools/CountsVisitor.h>
#include <osgwTools/RemoveData.h>
#include <osgwTools/CountsVisitor.h>
#include <osgwTools/StateSetUtils.h>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Shader>
#include <osg/Notify>
#include <osgwTools/Version.h>

#include <sstream>
#include <iomanip>
#include <set>
#include <boost/algorithm/string/trim.hpp>


namespace backdropFX
{


bool convertFFPToShaderModules( osg::Node* node, backdropFX::ShaderModuleVisitor* smv )
{
#if( OSGWORKS_VERSION >= 10153 )
    osgwTools::CountsVisitor cv;
    cv.setUserMode( GL_LIGHTING );
    node->accept( cv );
    cv.dump( osg::notify( osg::INFO ) );

    // Logic: If at least 95% of the drawables have lighting off...
    if( ( cv.getTotalDrawables() * .95 ) <= cv.getNumDrawablesUserModeOff() )
    {
        // Assume lighting is off for the entire scene graph.
        // - Remove all GL_LIGHTING modes.
        // - Remove all Material StateAttributes.
        // - Place a single GL_LIGHTING "off" mode at the root node.
        // SMV will insert the appropriate shader module to disable lighting.
        osgwTools::RemoveData rdv;
        rdv.setRemovalFlags( osgwTools::RemoveData::EMPTY_STATESETS );
        rdv.addRemoveAttribute( osg::StateAttribute::MATERIAL );
        rdv.addRemoveMode( GL_LIGHTING );
        node->accept( rdv );
        node->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    }
#endif

    // TBD collect positional state

    bool ownSMV = ( smv == NULL );
    backdropFX::ShaderModuleVisitor* localSMV;
    if( ownSMV )
        localSMV = new backdropFX::ShaderModuleVisitor;
    else
        localSMV = smv;

    node->accept( *localSMV );

    localSMV->mergeDefaults( *node );
    if( ownSMV )
        delete localSMV;

#if 0
    // Conversion from FFP to shaders could produce empty StateSets. Remove them.
    {
        osgwTools::CountsVisitor counter;
        node->accept( counter );
        osg::notify( osg::INFO ) << "Empty StateSets: " <<
            counter._emptyStateSets << std::endl;
        // TBD remove empty state sets.
    }
#endif

#if( OSGWORKS_VERSION >= 10153 )
    cv.reset();
    node->accept( cv );
    cv.dump( osg::notify( osg::INFO ) );
#endif

    return( true );
}

osg::StateSet* accumulateStateSetsAndShaderModules( ShaderModuleCullCallback::ShaderMap& shaders, const osg::NodePath& nodePath )
{
    osg::NodePath::const_iterator it;
    for( it = nodePath.begin(); it != nodePath.end(); it++ )
    {
        osg::Node* node = *it;
        ShaderModuleCullCallback* smccb = dynamic_cast< ShaderModuleCullCallback* >( node->getCullCallback() );
        if( smccb != NULL )
        {
            const ShaderModuleCullCallback::ShaderMap& newShaders = smccb->getShaderMap();
            shaders.insert( newShaders.begin(), newShaders.end() );
        }
    }

    return( osgwTools::accumulateStateSets( nodePath ) );
}

std::string
insertLineNumbers(const std::string& source)
{
    if (source.empty()) return source;

    unsigned int lineNum = 1;       // Line numbers start at 1
    std::ostringstream ostr;

    std::string::size_type previous_pos = 0;
    do
    {
        std::string::size_type pos = source.find_first_of("\n", previous_pos);
        if( pos != std::string::npos )
        {
            ostr << std::setw( 4 ) << std::right << lineNum << ": " << source.substr( previous_pos, pos-previous_pos ) << std::endl;
            previous_pos = pos + ( 1 < source.size() ) ? pos+1 : std::string::npos;
        }
        else
        {
            ostr << std::setw( 4 ) << std::right << lineNum << ": " << source.substr( previous_pos, std::string::npos ) << std::endl;
            previous_pos = std::string::npos;
        }
        ++lineNum;
    } while( previous_pos != std::string::npos );

    return( ostr.str() );
}
std::string
insertLineNumbersAsComments( const std::string& source, const std::string& fileName=std::string("") )
{
    if( source.empty() )
        return source;

    // Insert \n at end of line per OpenGL spec for shader source.
    std::string newline( "\n" ); // LF

    unsigned int lineNum = 1;       // Line numbers start at 1
    std::ostringstream ostr;

    std::string::size_type previous_pos = 0;
    do
    {
        ostr << "/* " << fileName << std::setw( 5 ) << std::right << lineNum << " */ ";

        std::string::size_type pos = source.find_first_of("\n", previous_pos);
        if( pos != std::string::npos )
        {
            ostr << source.substr( previous_pos, pos-previous_pos ) << newline;
            previous_pos = pos + ( 1 < source.size() ) ? pos+1 : std::string::npos;
        }
        else
        {
            ostr << source.substr( previous_pos, std::string::npos ) << newline;
            previous_pos = std::string::npos;
        }
        ++lineNum;
    } while( previous_pos != std::string::npos );

    return( ostr.str() );
}

void
dumpShaderSource( std::ostream& ostr, const std::string& preamble, const std::string& source )
{
    std::string sourceWithLineNumbers = insertLineNumbers( source );

    ostr << std::endl << preamble << " source dump:\n"
        << sourceWithLineNumbers << std::endl;
}

void
shaderPreProcess( osg::Shader* shader )
{
    const bool debugDump( ( backdropFX::Manager::instance()->getDebugMode() &
        backdropFX::BackdropCommon::debugShaders ) != 0 );
	std::string::size_type findStartPos = 0;
	std::set<std::string> includeOneShot;

    osg::notify( osg::INFO ) << "Starting shaderPreProcess for " <<
        shader->getTypename() << " shader:\n" << shader->getFileName() << std::endl;
    if( shader->getShaderSource().empty() )
    {
        osg::notify( osg::WARN ) << "BDFX: shaderPreProcess: empty " <<
            shader->getTypename() << " shader:\n" << shader->getFileName() << std::endl;
        return;
    }
    if( debugDump )
        dumpShaderSource( osg::notify( osg::INFO ), "original", shader->getShaderSource() );

    std::string sourceNum, preNum, postNum;
    if( debugDump )
        sourceNum = insertLineNumbersAsComments( shader->getShaderSource(), shader->getName() );

    bool done( false );
    while( !done )
    {
        const std::string& source( shader->getShaderSource() );
        std::string::size_type bdfxPos( source.find( "BDFX ", findStartPos ) );
        if( bdfxPos == std::string::npos )
        {
            done = true;
            break;
        }

		// attempt to ensure nothing but whitespace preceeds token introducer on this line
		// search backwards for first preceeding EOL
        std::string::size_type crPos_pre( source.rfind( "\r", bdfxPos ) );
		if(crPos_pre == std::string::npos) crPos_pre = 0; // if no preceeding EOL found, set position to beginning of file
        std::string::size_type nlPos_pre( source.rfind( "\n", bdfxPos ) );
		if(nlPos_pre == std::string::npos) nlPos_pre = 0; // if no preceeding EOL found, set position to beginning of file
        std::string::size_type eolPos_pre = osg::maximum< std::string::size_type >( crPos_pre, nlPos_pre );
		// extract region between BDFX token introducer and previous EOL (or start)
		std::string preceedingTokenIntroducer;
		if(bdfxPos > eolPos_pre)
		{
			preceedingTokenIntroducer = source.substr( eolPos_pre, bdfxPos - eolPos_pre); // this may pick up the EOL, but it will be discarded as whitespace below
		} // if
		// trim out any whitespace
		const std::string nonWhitespace = boost::algorithm::trim_left_copy(preceedingTokenIntroducer);
		// determine if anything other than whitespace remains
		if(nonWhitespace.length() != 0)
		{ // skip this BDFX token
			findStartPos = bdfxPos+5; // next find will start past this token introducer
			continue; // skip all the processing below
		} // if




        const std::string preSource( source.substr( 0, bdfxPos ) );

        std::string::size_type spacePos( source.find( " ", bdfxPos+5 ) );
        const std::string token( source.substr( bdfxPos+5, spacePos-(bdfxPos+5) ) );

        osg::notify( osg::DEBUG_FP ) << "  Processing token: \"" << token << "\"" << std::endl;
        if( token == std::string( "INCLUDE" ) )
        {
            spacePos += 1;
            std::string::size_type crPos( source.find( "\r", spacePos ) );
            std::string::size_type nlPos( source.find( "\n", spacePos ) );
            std::string::size_type eolPos = osg::minimum< std::string::size_type >( crPos, nlPos );
            const std::string postSource( source.substr( eolPos+1 ) );

            const std::string fileName( source.substr( spacePos, eolPos-spacePos ) );
            osg::notify( osg::DEBUG_FP ) << "    \"" << fileName << "\"" << std::endl;

            osg::ref_ptr< osg::Shader> newShader = new osg::Shader;
            std::string shaderFile( osgDB::findDataFile( fileName ) );
            if( shaderFile.empty() )
            {
                osg::notify( osg::WARN ) << "    Unable to find included shader file: \"" << fileName << "\"" << std::endl;
                done = true;
                break;
            }

			// is this file marked as already included in the OneShot set?
			if( includeOneShot.find(shaderFile) == includeOneShot.end() )
			{ // not found -- not already included
				newShader->loadShaderSourceFromFile( shaderFile );
                newShader->setName( osgDB::getSimpleFileName( shaderFile ) );
				const std::string newSource( newShader->getShaderSource() );
				shader->setShaderSource( preSource + newSource + postSource );
				includeOneShot.insert(shaderFile); // mark this file as already included in the OneShot set
			} // if
			else
			{ // filename found, already included
				// recombine without the INCLUDE directive since it has already been included previously
				shader->setShaderSource( preSource + postSource );
			} // else

            if( debugDump )
            {
                std::string::size_type bdfxPos( sourceNum.find( "BDFX INCLUDE" ) );
                preNum = sourceNum.substr( 0, bdfxPos );
                std::string::size_type prevLnPos( preNum.find_last_of( "\n" ) );
                preNum = sourceNum.substr( 0, prevLnPos+1 );

                std::string::size_type crPos( sourceNum.find( "\r", bdfxPos+12 ) );
                std::string::size_type nlPos( sourceNum.find( "\n", bdfxPos+12 ) );
                std::string::size_type eolPos = osg::minimum< std::string::size_type >( crPos, nlPos );
                const std::string postNum( sourceNum.substr( eolPos+1 ) );

                const std::string newSource( insertLineNumbersAsComments( newShader->getShaderSource(), newShader->getName() ) );
                sourceNum = std::string( preNum + newSource.substr( 0, newSource.length()-1 ) + postNum );
                //osg::notify( osg::ALWAYS ) << "Pre: " << std::endl << preNum << std::endl;
                //osg::notify( osg::ALWAYS ) << "New: " << std::endl << newSource << std::endl;
                //osg::notify( osg::ALWAYS ) << "Post: " << std::endl << postNum << std::endl;
                //osg::notify( osg::ALWAYS ) << "Final: " << sourceNum << std::endl;
            }

            if( debugDump && ( osg::getNotifyLevel() >= osg::DEBUG_FP ) )
            {
                dumpShaderSource( osg::notify( osg::DEBUG_FP ), "pre", preSource );
                dumpShaderSource( osg::notify( osg::DEBUG_FP ), "post", postSource );
                dumpShaderSource( osg::notify( osg::DEBUG_FP ), "merged", shader->getShaderSource() );
            }
        }
        else
        {
            osg::notify( osg::WARN ) << "  Unknown token: \"" << token << "\"" << std::endl;
            done = true;
        }
    }

    if( debugDump )
    {
        dumpShaderSource( osg::notify( osg::DEBUG_FP ), "processed", shader->getShaderSource() );

        std::string fileName = osgDB::getSimpleFileName( shader->getFileName() );
        if( fileName.empty() )
            osg::notify( osg::WARN ) << "backdropFX: shaderPreProcess: Empty shader file name." << std::endl;
        fileName += std::string( "-dbg.txt" );

        std::ofstream ofstr( fileName.c_str(), std::ios_base::binary | std::ios_base::out );
        if( !ofstr.good() )
            osg::notify( osg::WARN ) << "backdropFX: shaderPreProcess: Bad file" << std::endl;
        ofstr << sourceNum;
    }
}

backdropFX::ShaderModuleCullCallback*
getOrCreateShaderModuleCullCallback( osg::Node& node )
{
    osg::NodeCallback* nodecb = node.getCullCallback();
    backdropFX::ShaderModuleCullCallback* smccb = dynamic_cast<
        backdropFX::ShaderModuleCullCallback* >( nodecb );

    osg::NodeCallback* nestedcb = NULL;
    if( ( nodecb != NULL ) && ( smccb == NULL ) )
        nestedcb = nodecb;

    if( smccb == NULL )
        smccb = new backdropFX::ShaderModuleCullCallback;

    if( nestedcb != NULL )
        smccb->addNestedCallback( nestedcb );
    node.setCullCallback( smccb );

    return( smccb );
}

std::string
getShaderSemantic( const std::string& name )
{
    std::string::size_type pos0 = name.find( "-" ) + 1;
    std::string::size_type pos1 = name.find( "-", pos0 );
    if( pos1 == std::string::npos )
        pos1 = name.find( ".", pos0 );
    return( name.substr( pos0, pos1-pos0 ) );
}


// namespace backdropFX
}
