// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#include <backdropFX/DepthPeelBin.h>
#include <backdropFX/Manager.h>
#include <backdropFX/BackdropCommon.h>
#include <osgUtil/RenderBin>
#include <osgUtil/StateGraph>
#include <osgDB/FileUtils>
#include <osg/CopyOp>
#include <osg/GLExtensions>
#include <osg/GL2Extensions>
#include <osg/FrameBufferObject>
#include <osg/Notify>
#include <backdropFX/Utils.h>
#include <osgwTools/FBOUtils.h>
#include <osgwTools/Shapes.h>
#include <osgwTools/Version.h>

#include <sstream>
#include <iomanip>


#define TRACEDUMP(__t)  // osg::notify( osg::NOTICE ) << __t << std::endl;


namespace backdropFX {



// Statics
GLuint DepthPeelBin::s_textureUnit( 14 );
    

DepthPeelBin::DepthPeelBin()
  : _minPixels( 25 ),
    _maxPasses( 16 ),
    _partitionNumber( 0 )
{
    TRACEDUMP("DepthPeelBin");

    internalInit();
}
DepthPeelBin::DepthPeelBin( osgUtil::RenderBin::SortMode mode )
  : _minPixels( 25 ),
    _maxPasses( 16 ),
    _partitionNumber( 0 )
{
    TRACEDUMP("DepthPeelBin sortMode");

    internalInit();
}
DepthPeelBin::DepthPeelBin( const DepthPeelBin& rhs, const osg::CopyOp& copyOp )
  : _minPixels( rhs._minPixels ),
    _maxPasses( rhs._maxPasses ),
    _partitionNumber( 0 ),
    _opaqueDepth( rhs._opaqueDepth ),
    _transparentDepth( rhs._transparentDepth ),
    _fstp( rhs._fstp ),
    _fstpProgram( rhs._fstpProgram ),
    _fstpBlendFunc( rhs._fstpBlendFunc ),
    _fstpUniform( rhs._fstpUniform ),
    _texturePercentUniform( rhs._texturePercentUniform )
{
    TRACEDUMP("DepthPeelBin copy");
}
DepthPeelBin::~DepthPeelBin()
{
    TRACEDUMP("~DepthPeelBin");
}

void DepthPeelBin::internalInit()
{
    _opaqueDepth = new osg::Depth( osg::Depth::LESS, 0., 1., true );
    _transparentDepth = new osg::Depth( osg::Depth::GREATER, 0., 1., true );

    _fstp = osgwTools::makePlane(
        osg::Vec3( -1,-1,0 ), osg::Vec3( 2,0,0 ), osg::Vec3( 0,2,0 ) );
    UTIL_MEMORY_CHECK( _fstp.get(), "DepthPeel FSTP",  );
    _fstp->setColorBinding( osg::Geometry::BIND_OFF );
    _fstp->setNormalBinding( osg::Geometry::BIND_OFF );
    _fstp->setTexCoordArray( 0, NULL );
    _fstp->setUseDisplayList( false );
    _fstp->setUseVertexBufferObjects( true );

    osg::Shader* vertShader = new osg::Shader( osg::Shader::VERTEX );
    UTIL_MEMORY_CHECK( vertShader, "DepthPeelBin FSTP vertShader",  );
    vertShader->setName( "DepthPeelDisplay.vs" );
    std::string fullName = osgDB::findDataFile( "shaders/" + vertShader->getName() );
    if( fullName.empty() )
        osg::notify( osg::WARN ) << "BDFX: DepthPeelBin: Can't find file " << vertShader->getName() << std::endl;
    else
        vertShader->loadShaderSourceFromFile( fullName );
    osg::Shader* fragShader = new osg::Shader( osg::Shader::FRAGMENT );
    UTIL_MEMORY_CHECK( fragShader, "DepthPeelBin FSTP fragShader",  );
    fragShader->setName( "DepthPeelDisplay.fs" );
    fullName = osgDB::findDataFile( "shaders/" + fragShader->getName() );
    if( fullName.empty() )
        osg::notify( osg::WARN ) << "BDFX: DepthPeelBin: Can't find file " << fragShader->getName() << std::endl;
    else
        fragShader->loadShaderSourceFromFile( fullName );

    _fstpProgram = new osg::Program();
    UTIL_MEMORY_CHECK( _fstpProgram, "DepthPeelBin FSTP program",  );
    _fstpProgram->setName( "DepthPeelDisplay" );
    _fstpProgram->addShader( vertShader );
    _fstpProgram->addShader( fragShader );

    // Enable source alpha / one minus source alpha blending.
    _fstpBlendFunc = new osg::BlendFunc( osg::BlendFunc::SRC_ALPHA,
        osg::BlendFunc::ONE_MINUS_SRC_ALPHA );
    UTIL_MEMORY_CHECK( _fstpBlendFunc, "DepthPeelBin FSTP BlendFunc",  );

    _fstpUniform = new osg::Uniform( "depthPeelTexture", (int)( s_textureUnit+1 ) );
    UTIL_MEMORY_CHECK( _fstpUniform, "DepthPeelBin GSTP Uniform",  );

    _texturePercentUniform = new osg::Uniform( "depthPeelTexturePercent", osg::Vec2f( 1.f, 1.f ) );
    UTIL_MEMORY_CHECK( _texturePercentUniform, "DepthPeelBin Texture Percent Uniform",  );
}

unsigned int DepthPeelBin::drawInit( osg::State& state, osgUtil::RenderLeaf*& previous )
{
    TRACEDUMP("DepthPeelBin::drawInit");

    unsigned int numToPop = (previous ? osgUtil::StateGraph::numToPop(previous->_parent) : 0);
    if (numToPop>1) --numToPop;
    unsigned int insertStateSetPosition = state.getStateSetStackSize() - numToPop;

    if (_stateset.valid())
    {
        state.insertStateSet(insertStateSetPosition, _stateset.get());
    }

    return( insertStateSetPosition );
}
int DepthPeelBin::drawOpaque( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous, bool& remaining )
{
    TRACEDUMP("DepthPeelBin::drawOpaque");

    int drawCount( 0 );
    osg::State& state = *renderInfo.getState();

    unsigned int rbc( 0 ), rla( 0 ), rlb( 0 );

    // draw first set of draw bins.
    RenderBinList::iterator rbitr;
    for(rbitr = _bins.begin();
        rbitr!=_bins.end() && rbitr->first<0;
        ++rbitr)
    {
        rbitr->second->draw(renderInfo,previous);
        rbc++;
        drawCount++;
    }

    // draw fine grained ordering.
    for(RenderLeafList::iterator rlitr= _renderLeafList.begin();
        rlitr!= _renderLeafList.end();
        ++rlitr)
    {
        osgUtil::RenderLeaf* rl = *rlitr;
        rl->render(renderInfo,previous);
        previous = rl;
        rla++;
        drawCount++;
    }


    // draw coarse grained ordering.
    for(StateGraphList::iterator oitr=_stateGraphList.begin();
        oitr!=_stateGraphList.end();
        ++oitr)
    {

        for(osgUtil::StateGraph::LeafList::iterator dw_itr = (*oitr)->_leaves.begin();
            dw_itr != (*oitr)->_leaves.end();
            ++dw_itr)
        {
            osgUtil::RenderLeaf* rl = dw_itr->get();
            rl->render(renderInfo,previous);
            previous = rl;
            rlb++;
            drawCount++;
        }
    }

    remaining = ( rbitr != _bins.end() );

    osg::notify( osg::DEBUG_FP ) << "rbc " << rbc <<
        "  rla " << rla << "  rlb " << rlb << std::endl;
    return( drawCount );
}

void DepthPeelBin::drawTransparent( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    TRACEDUMP("DepthPeelBin::drawTransparent");

    // Advance RenderBin iterator past the pre bins.
    RenderBinList::iterator rbitr;
    for(rbitr = _bins.begin();
        rbitr!=_bins.end() && rbitr->first<0;
        ++rbitr)
    {}

    unsigned int tc( 0 );

    // draw post bins.
    for(;
        rbitr!=_bins.end();
        ++rbitr)
    {
        rbitr->second->draw(renderInfo,previous);
        tc++;
    }
    osg::notify( osg::DEBUG_FP ) << "tc " << tc << std::endl;
}

void DepthPeelBin::drawFSTP( osg::RenderInfo& renderInfo, osg::State& state, osg::GL2Extensions* ext, PerContextInfo& pci,
                            GLint& fstpLoc, GLint& texturePercentLoc )
{
    TRACEDUMP("DepthPeelBin::drawFSTP");

    // Set up the program and uniforms.
    state.applyAttribute( _fstpProgram.get() );
    if( fstpLoc < 0 )
#if OSG_SUPPORTS_UNIFORM_ID
        fstpLoc = state.getUniformLocation( _fstpUniform->getNameID() );
#else
        fstpLoc = state.getUniformLocation( _fstpUniform->getName() );
#endif
    _fstpUniform->apply( ext, fstpLoc );
    if( texturePercentLoc < 0 )
#if OSG_SUPPORTS_UNIFORM_ID
        texturePercentLoc = state.getUniformLocation( _texturePercentUniform->getNameID() );
#else
        texturePercentLoc = state.getUniformLocation( _texturePercentUniform->getName() );
#endif
    _texturePercentUniform->apply( ext, texturePercentLoc );

    state.setActiveTextureUnit( s_textureUnit+1 );
    glBindTexture( GL_TEXTURE_2D, pci._colorTex );
    state.applyAttribute( _fstpBlendFunc.get() );
    state.applyMode( GL_BLEND, true );
    state.applyMode( GL_DEPTH_TEST, false );

    _fstp->draw( renderInfo );

    state.setActiveTextureUnit( s_textureUnit+1 );
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void DepthPeelBin::drawComplete( osg::State& state, unsigned int insertStateSetPosition )
{
    TRACEDUMP("DepthPeelBin::drawComplete");

    if (_stateset.valid())
    {
        state.removeStateSet(insertStateSetPosition);
        // state.apply();
    }
}



// Static
osg::buffered_object< DepthPeelBin::PerContextInfo >
    DepthPeelBin::s_contextInfo;


void DepthPeelBin::drawImplementation( osg::RenderInfo& renderInfo, osgUtil::RenderLeaf*& previous )
{
    TRACEDUMP("DepthPeelBin::drawImplementation");
    UTIL_GL_ERROR_CHECK("DepthPeelBin::drawImplementation start");

    unsigned int debugMode = Manager::instance()->getDebugMode();

    osg::State& state = *renderInfo.getState();
    const unsigned int contextID = state.getContextID();
    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( contextID, true ) );
    UTIL_GL_FBO_ERROR_CHECK("DepthPeelBin::drawImplementation start",fboExt);
    osg::GL2Extensions* ext = osg::GL2Extensions::Get( contextID, true );
    UTIL_MEMORY_CHECK( ext, "DepthPeelBin: NULL GL2Extensions", );

    // Get the last applied viewport. When we render to our internal textures,
    // we will render to the lower left corner in an area the size of the viewport.
    // We track the width and height and maintain internal textures large enough
    // to render the largest width and height we've seen.
    const osg::Viewport* vp = dynamic_cast< const osg::Viewport* >(
        state.getLastAppliedAttribute( osg::StateAttribute::VIEWPORT ) );
    const GLsizei width( vp->width() ), height( vp->height() );

    char* pixels( NULL );
    const bool dumpImages = ( ( debugMode & backdropFX::BackdropCommon::debugImages ) != 0 );
    if( dumpImages )
    {
        pixels = new char[ width * height * 4 ];
        UTIL_MEMORY_CHECK( pixels, "DepthPeelBin: debug image pixel buffer", )
    }

    // Fix for redmine issue 8, and the recurrance of this issue
    // in the new depth peel work.
    // The most general way to handle the single StateGraph case
    // is to save the last StateGraph and re-apply it just before
    // we return from ::draw().
    osgUtil::StateGraph* savedStateGraph( NULL );
    if( _stateGraphList.size() )
        savedStateGraph = _stateGraphList.back();

    PerContextInfo& pci = s_contextInfo[ contextID ];
    unsigned int insertStateSetPosition;
    {
        // Get the current Draw FBO and restore it when fboSRH goes out of scope.
        // Get this now, first, before we call pci._init. Also, having it hear means
        // we query OpenGL once. If, instead, we instantiated one of these for every
        // layer, we would query OpenGL every layer. Having just one instance, with
        // its constructor invoked once, is a better solution.
        FBOSaveRestoreHelper fboSRH( fboExt, pci );

        insertStateSetPosition = drawInit( state, previous );

        if( pci._init &&
            ( ( pci._width < width ) || ( pci._height < height ) ) )
        {
            // We've already created textures at a given size, but we
            // now have a larger viewport. Delete those textures and
            // force a re-initialization.
            // NOTE We never resize the textures smaller, only larger.
            osg::notify( osg::INFO ) << "BDFX: DepthPeelBin cleanup. ";
            pci.cleanup( state );
        }
        if( !pci._init )
        {
            osg::notify( osg::INFO ) << "BDFX: DepthPeelBin resize to width: " <<
                width << " height: " << height << std::endl;
            pci.init( state, width, height );
        }

        // Compute the percentage of the texture we will render to.
        // This is the viewport width and height divided by the texture
        // width and height. It's used to ensure we display the appropriate
        // portion of the texture during drawFSTP().
        osg::Vec2f texturePercent( vp->width() / (float)( pci._width ),
            vp->height() / (float)( pci._height ) );
        _texturePercentUniform->set( texturePercent );

        // Uniform locations, used during drawFSTP(). We declare them here and pass
        // them by reference. drawFSTP() inits them after binding the fstpProgram.
        // Then we reuse the values until next frame. Avoids looking up the uniform
        // location (via map keyed by string) for every layer.
        GLint fstpLoc( -1 ), texturePercentLoc( -1 );


        // Opaque pass.
        bool transparentRemaining( false ); // After opaque pass, are there transparent bins?
        int drawCount( 0 );
        {
            osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, pci._fbo );

            state.applyAttribute( _opaqueDepth.get() );
            state.applyMode( GL_DEPTH_TEST, true );
            glClearDepth( 1.0 );
            unsigned int idx;
            for( idx=0; idx<2; idx++ )
            {
                osgwTools::glFramebufferTexture2D( fboExt, GL_DRAW_FRAMEBUFFER_EXT,
                    GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, pci._depthTex[ idx ], 0 );
                glClear( GL_DEPTH_BUFFER_BIT );
            }
            osgwTools::glFramebufferTexture2D( fboExt, GL_DRAW_FRAMEBUFFER_EXT,
                GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, pci._depthTex[ 2 ], 0 );

            state.setActiveTextureUnit( s_textureUnit );
            glBindTexture( GL_TEXTURE_2D, pci._depthTex[ 0 ] );
            state.setActiveTextureUnit( s_textureUnit+1 );
            glBindTexture( GL_TEXTURE_2D, pci._depthTex[ 1 ] );

            glClearColor( 0., 0., 0., 0. );
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            drawCount = drawOpaque( renderInfo, previous, transparentRemaining );

            // Blend the opaque pass into the output buffer.
            // We could probably do this a different way, by attaching
            // the output color buffer to our FBO and rendering directly
            // into it. This is TBD as a later enhancement.
            fboSRH.restore();

            drawFSTP( renderInfo, state, ext, pci, fstpLoc, texturePercentLoc );

            if( dumpImages )
            {
                FBOSaveRestoreHelper fboSRHRead( fboExt, pci, GL_READ_FRAMEBUFFER_EXT );
                osgwTools::glBindFramebuffer( fboExt, GL_READ_FRAMEBUFFER_EXT, pci._fbo );

                std::string fileName = createFileName( state );
                glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
                glReadPixels( (GLint)( 0 ), (GLint)( 0 ), width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels );
                backdropFX::debugDumpImage( fileName, pixels, width, height );
                osg::notify( osg::NOTICE ) << " - " << fileName << std::endl;

                fileName = createFileName( state, -1, true );
                glReadPixels( (GLint)( 0 ), (GLint)( 0 ), width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, (GLvoid*)pixels );
                backdropFX::debugDumpDepthImage( fileName, (const short*)pixels, width, height );
                osg::notify( osg::NOTICE ) << " - " << fileName << std::endl;
            }
        }


        // drawOpaque() sets this to true if there are transparent bins to render.
        if( transparentRemaining )
        {
            // Transparent passes
            state.setActiveTextureUnit( s_textureUnit );
            glBindTexture( GL_TEXTURE_2D, pci._depthTex[ 2 ] );

            // If we already drew something in the opaque pass, then GL_LESS has already been
            // set. But if we didn't draw anything in the opaque pass (drawCount==0) then the
            // scene graph will almost certainly set depth function to GL_LESS using lazy state
            // setting. In that case, we must apply state _now_ so that the transparent pass can
            // correctly set the depth function to GL_GREATER.
            if( drawCount == 0 )
                state.apply();

            // Create depth peel layers until we hit _maxPasses, or until
            // occlusion query indicates we didn't render anything.
            unsigned int passCount;
            for( passCount = 0; passCount < _maxPasses; passCount++ )
            {
                // Specify the depth buffer to render to.
                osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, pci._fbo );
                osgwTools::glFramebufferTexture2D( fboExt, GL_DRAW_FRAMEBUFFER_EXT,
                    GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, pci._depthTex[ passCount & 0x1 ], 0 );
                osg::notify( osg::DEBUG_FP ) << "  Attaching depth buffer " << pci._depthTex[ passCount & 0x1 ] << std::endl;

                // Use the other depth buffer as an input texture.
                state.setActiveTextureUnit( s_textureUnit+1 );
                glBindTexture( GL_TEXTURE_2D, pci._depthTex[ (passCount+1) & 0x1 ] );
                osg::notify( osg::DEBUG_FP ) << "  Binding depth map " << pci._depthTex[ (passCount+1) & 0x1 ] << std::endl;

                _transparentDepth->apply( state );
                glEnable( GL_DEPTH_TEST );
                glClearDepth( 0.0 );
                glClearColor( 0., 0., 0., 0. );
                glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

                pci._glBeginQuery( GL_SAMPLES_PASSED_ARB, pci._queryID );
                drawTransparent( renderInfo, previous );
                pci._glEndQuery( GL_SAMPLES_PASSED_ARB );

                if( dumpImages )
                {
                    FBOSaveRestoreHelper fboSRHRead( fboExt, pci, GL_READ_FRAMEBUFFER_EXT );
                    osgwTools::glBindFramebuffer( fboExt, GL_READ_FRAMEBUFFER_EXT, pci._fbo );

                    std::string fileName = createFileName( state, passCount );
                    glReadBuffer( GL_COLOR_ATTACHMENT0_EXT );
                    glReadPixels( (GLint)( 0 ), (GLint)( 0 ), width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels );
                    backdropFX::debugDumpImage( fileName, pixels, width, height );
                    osg::notify( osg::NOTICE ) << " - " << fileName << std::endl;

                    fileName = createFileName( state, passCount, true );
                    glReadPixels( (GLint)( 0 ), (GLint)( 0 ), width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, (GLvoid*)pixels );
                    backdropFX::debugDumpDepthImage( fileName, (const short*)pixels, width, height );
                    osg::notify( osg::NOTICE ) << " - " << fileName << std::endl;
                }

                // Query the number of pixels rendered to see if it's time to stop.
                GLint numPixels( 0 );
                pci._glGetQueryObjectiv( pci._queryID, GL_QUERY_RESULT, &numPixels );
                osg::notify( osg::DEBUG_FP ) << "  BDFX: DP pass " << passCount << ",  numPixels " << numPixels << std::endl;
                if( numPixels < _minPixels )
                {
                    passCount++;
                    break;
                }

                // We rendered something, so now we render the FSTP to combine the layer we just
                // created with the original FBO.
                fboSRH.restore();

                drawFSTP( renderInfo, state, ext, pci, fstpLoc, texturePercentLoc );
            }

            if( debugMode & BackdropCommon::debugConsole )
                osg::notify( osg::DEBUG_FP ) << "BDFX: DepthPeelBin: " << passCount << " pass" <<
                ((passCount==1)?".":"es.") << std::endl;

            // Restore to default.
            glClearDepth( 1.0 );

        } // if transparentRemaining
    }


    // RenderBin::drawImplementation wrap-up: State restore.
    drawComplete( state, insertStateSetPosition );

    if( dumpImages )
        delete[] pixels;

    // Re-apply the last StateGraph used to render the child subgraph.
    // This restores state to the way OSG thinks it should be.
    if( savedStateGraph != NULL )
        state.apply( savedStateGraph->getStateSet() );

    UTIL_GL_ERROR_CHECK("DepthPeelBin::drawImplementation end");
    UTIL_GL_FBO_ERROR_CHECK("DepthPeelBin::drawImplementation end",fboExt);
}

std::string DepthPeelBin::createFileName( osg::State& state, int pass, bool depth )
{
    unsigned int contextID = state.getContextID();
    int frameNumber = state.getFrameStamp()->getFrameNumber();
    std::ostringstream ostr;
    ostr << std::setfill( '0' );
    ostr << "f" << std::setw( 6 ) << frameNumber <<
        "_c" << std::setw( 2 ) << contextID << "_";

    ostr << "peel_part" << _partitionNumber;
    if( pass == -1 )
        ostr << "_a";
    else
        ostr << "_b" << std::setw( 2 ) << pass;
    if( depth )
        ostr << "_z";
    ostr << ".png";
    return( ostr.str() );
}



DepthPeelBin::PerContextInfo::PerContextInfo()
  : _init( false ),
    _fbo( 0 ),
    _width( 0 ),
    _height( 0 ),
    _colorTex( 0 ),
    _queryID( 0 )
{
    _depthTex[ 0 ] = _depthTex[ 1 ] = _depthTex[ 2 ] = 0;
}

void
DepthPeelBin::PerContextInfo::init( const osg::State& state, const GLsizei width, const GLsizei height )
{
    TRACEDUMP("PerContextInfo::init");

    _width = width;
    _height = height;


    // Create two depth buffers; First two are ping-pong buffers for each pass.
    // The third is the persistent depth buffer from the opaque pass.
    glGenTextures( 3, _depthTex );
    UTIL_GL_ERROR_CHECK( "DepthPeelBin PerContext Depth Tex" );
    int idx;
    for( idx=0; idx<3; idx++ )
    {
        glBindTexture( GL_TEXTURE_2D, _depthTex[ idx ] );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL ); // Alpha == 1.0 if R [func] texel.

        glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height,
            0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL );
    }


    glGenTextures( 1, &_colorTex );
    UTIL_GL_ERROR_CHECK( "DepthPeelBin PerContextInfo Color Tex" );
    glBindTexture( GL_TEXTURE_2D, _colorTex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, _width, _height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_2D, 0 );


    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( state.getContextID(), true ) );

    osgwTools::glGenFramebuffers( fboExt, 1, &_fbo );
    osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, _fbo );
    osgwTools::glFramebufferTexture2D( fboExt, GL_DRAW_FRAMEBUFFER_EXT,
        GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _colorTex, 0 );

    UTIL_GL_ERROR_CHECK( "DepthPeenBin PerContextInfo" );


    osg::setGLExtensionFuncPtr( _glGenQueries, "glGenQueries", "glGenQueriesARB");
    osg::setGLExtensionFuncPtr( _glDeleteQueries, "glDeleteQueries", "glDeleteQueriesARB");
    osg::setGLExtensionFuncPtr( _glBeginQuery, "glBeginQuery", "glBeginQueryARB");
    osg::setGLExtensionFuncPtr( _glEndQuery, "glEndQuery", "glEndQueryARB");
    osg::setGLExtensionFuncPtr( _glGetQueryObjectiv, "glGetQueryObjectiv","glGetQueryObjectivARB");
    osg::setGLExtensionFuncPtr( _glGetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv","glGetFramebufferAttachmentParameterivEXT");

    _glGenQueries( 1, &_queryID );


    UTIL_GL_ERROR_CHECK( "DepthPeelBin PerContextInfo end" );
    UTIL_GL_FBO_ERROR_CHECK( "DepthPeelBin PerContextInfo end", fboExt );
    _init = true;
}
void
DepthPeelBin::PerContextInfo::cleanup( const osg::State& state )
{
    TRACEDUMP("  PerContextInfo::cleanup");

    glDeleteTextures( 3, _depthTex );
    _depthTex[ 0 ] = _depthTex[ 1 ] = _depthTex[ 2 ] = 0;

    glDeleteTextures( 1, &_colorTex );
    _colorTex = 0;

    osg::FBOExtensions* fboExt( osg::FBOExtensions::instance( state.getContextID(), true ) );
    osgwTools::glBindFramebuffer( fboExt, GL_FRAMEBUFFER_EXT, 0 );
    osgwTools::glDeleteFramebuffers( fboExt, 1, &_fbo );
    _fbo = 0;

    _glDeleteQueries( 1, &_queryID );
    _queryID = 0;

    _init = false;
}



DepthPeelBin::FBOSaveRestoreHelper::FBOSaveRestoreHelper( osg::FBOExtensions* fboExt, DepthPeelBin::PerContextInfo& pci, GLenum fboTarget )
  : _fboExt( fboExt ),
    _pci( pci ),
    _fboTarget( fboTarget )
{
    GLenum queryEnum;
    switch( _fboTarget )
    {
    default:
        osg::notify( osg::WARN ) << "BDFX: FBOSaveRestoreHelper has unknown fbo: " << _fboTarget << std::endl;
        // Intentional fallthrough.
    case GL_DRAW_FRAMEBUFFER_EXT:
        queryEnum = GL_DRAW_FRAMEBUFFER_BINDING_EXT;
        break;
    case GL_READ_FRAMEBUFFER_EXT:
        queryEnum = GL_READ_FRAMEBUFFER_BINDING_EXT;
        break;
    }
    GLint fboID;
    glGetIntegerv( queryEnum, &fboID );
    _fboID = (unsigned int) fboID;
    UTIL_GL_ERROR_CHECK("FBOSaveRestoreHelper constructor");
    osg::notify( osg::INFO ) << "BDFX: FBOSaveRestoreHelper saving FBO ID: " << _fboID << std::endl;
}
DepthPeelBin::FBOSaveRestoreHelper::~FBOSaveRestoreHelper()
{
    osg::notify( osg::INFO ) << "BDFX: FBOSaveRestoreHelper destructor restoring FBO ID: " << _fboID << std::endl;
    osgwTools::glBindFramebuffer( _fboExt, _fboTarget, _fboID );
    UTIL_GL_ERROR_CHECK("FBOSaveRestoreHelper destructor");
}

GLuint DepthPeelBin::FBOSaveRestoreHelper::getTextureID( GLenum attachment )
{
    osgwTools::glBindFramebuffer( _fboExt, _fboTarget, _fboID );
    GLint textureID;
    _pci._glGetFramebufferAttachmentParameteriv( _fboTarget,
        attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT, &textureID );
    osg::notify( osg::DEBUG_FP ) << "  Texture ID: " << (GLuint) textureID << std::endl;
    return( ( GLuint )textureID );
}

void DepthPeelBin::FBOSaveRestoreHelper::restore()
{
    osg::notify( osg::INFO ) << "BDFX: FBOSaveRestoreHelper restoring FBO ID: " << _fboID << std::endl;
    UTIL_GL_ERROR_CHECK("FBOSaveRestoreHelper pre-restore");
    osgwTools::glBindFramebuffer( _fboExt, _fboTarget, _fboID );
    UTIL_GL_ERROR_CHECK("FBOSaveRestoreHelper restore");
}



// backdropFX
}
