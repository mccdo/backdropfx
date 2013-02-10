// Copyright (c) 2010 Skew Matrix Software. All rights reserved.

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/GUIEventHandler>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>

#include <osg/MatrixTransform>
#include <osg/TexGenNode>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/Material>
#include <osg/Fog>
#include <osg/TexGen>
#include <osg/CullFace>
#include <osgDB/FileNameUtils>

#include <backdropFX/ShaderModule.h>
#include <backdropFX/ShaderModuleUtils.h>
#include <backdropFX/ShaderModuleVisitor.h>
#include <backdropFX/Manager.h>

#include <osgwTools/Shapes.h>


/** \cond */
class KbdEventHandler : public osgGA::GUIEventHandler
{
public:
    KbdEventHandler( osg::Group* ffpRoot, osg::Group* shaderRoot )
      : _ffpRoot( ffpRoot ),
        _shaderRoot( shaderRoot ),
        _currentLighting( DEFAULT )
    {
        __LOAD_SHADER(_lightingOff,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-off.vs")
        __LOAD_SHADER(_lightingOn,osg::Shader::VERTEX,"shaders/gl2/ffp-lighting-on.vs")
    }
    
    virtual bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& )
    {
        bool handled( false );
        switch( ea.getEventType() )
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
        {
            switch( ea.getKey() )
            {
            case 'L':
            case 'l':
            {
                backdropFX::ShaderModuleCullCallback* smccb =
                    backdropFX::getOrCreateShaderModuleCullCallback( *_shaderRoot );
                osg::StateSet* ss = _ffpRoot->getOrCreateStateSet();
                ss->setDataVariance( osg::Object::DYNAMIC );
                switch( _currentLighting )
                {
                case DEFAULT:
                    _currentLighting = FORCE_OFF;
                    smccb->setShader( "lighting", _lightingOff.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceOverride );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE );
                    break;
                case FORCE_OFF:
                    _currentLighting = FORCE_ON;
                    smccb->setShader( "lighting", _lightingOn.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceOverride );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
                    break;
                case FORCE_ON:
                    _currentLighting = DEFAULT;
                    smccb->setShader( "lighting", _lightingOn.get(),
                        backdropFX::ShaderModuleCullCallback::InheritanceDefault );
                    ss->removeMode( GL_LIGHTING );
                    ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
                    break;
                }

                backdropFX::RebuildShaderModules rsm;
                _shaderRoot->accept( rsm );

                break;
            }
            }
        }
        }
        return( handled );
    }
protected:
    osg::ref_ptr< osg::Group > _ffpRoot, _shaderRoot;
    osg::ref_ptr< osg::Shader > _lightingOff, _lightingOn;

    typedef enum {
        DEFAULT,
        FORCE_OFF,
        FORCE_ON
    } CurrentLighting;
    CurrentLighting _currentLighting;
};
/** \endcond */


void
defaultScene( osg::Group* root, int page )
{
    osg::Node* teapot( osgDB::readNodeFile( "teapot.osg" ) );

    osg::Geode* testShape = NULL;
	osg::Geode* testBox = new osg::Geode();
	osg::Geode* testSphere = new osg::Geode();
	osg::Geometry *sphereGeometry = NULL;

	sphereGeometry = osgwTools::makeGeodesicSphere( 3.0, 5 );
	sphereGeometry->setTexCoordArray(1, sphereGeometry->getTexCoordArray(0)); // duplicate texcoords from unit 0 to unit 1
	testSphere->addDrawable( sphereGeometry );

	testBox->addDrawable( osgwTools::makeBox( osg::Vec3( 1.5, 1.5, 1.5 ) ) );

	if(page == 3) // texgen spheres
		testShape = testSphere;
	else
		testShape = testBox;

    // Current limitation: Shader module source must be attached to a
    // Group or Group-derived node. In the default scene, we use
    // MatrixTransforms anyhow, so this is not a problem, but is a
    // serious issue for real apps.

    osg::MatrixTransform* mt;


	switch(page)
	{
	case 0: // basic tests
		{
			// ambient only
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 0., 0., -3.0 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
				ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT1, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT2, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT3, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT4, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT5, osg::StateAttribute::ON );

				osg::LightSource* ls = new osg::LightSource;
				mt->addChild( ls );
				osg::Light* lt = new osg::Light;
				lt->setLightNum( 5 );
				lt->setAmbient( osg::Vec4( 1.0, 1.0, 1.0, 1.0 ) );
				lt->setDiffuse( osg::Vec4( 0., 0., 0., 1. ) );
				lt->setPosition( osg::Vec4( -3., 0., -4., 1. ) );
				ls->setLight( lt );
			}


			// Lighting off
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1., 0., -1.5 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
			}

			// Material Color Blue
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 1., 0., -1.5 ) );
			{
				osg::Geode* geode = dynamic_cast< osg::Geode* >( teapot );
				osg::Geode* blueTeapot = new osg::Geode( *geode, osg::CopyOp::DEEP_COPY_ALL );
				mt->addChild( blueTeapot );
				root->addChild( mt );

				// Test color material by changing the color to blue.
				osg::Geometry* geom = dynamic_cast< osg::Geometry* >( blueTeapot->getDrawable( 0 ) );
				osg::Vec4Array* c = new osg::Vec4Array;
				c->push_back( osg::Vec4( 0., 0., 1., 1. ) );
				geom->setColorArray( c );
			}

			// Diffuse+specular changes
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1., 0., 0. ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				osg::Material* mat = new osg::Material;
				mat->setDiffuse( osg::Material::FRONT_AND_BACK, osg::Vec4( 1., 0., 0., 1. ) );
				mat->setSpecular( osg::Material::FRONT_AND_BACK, osg::Vec4( .9, .9, 0., 1. ) );
				mat->setShininess( osg::Material::FRONT_AND_BACK, 32.f );
				ss->setAttribute( mat );
			}

			// Lighting experimentation
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 1., 0., 0. ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
				ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT1, osg::StateAttribute::ON );

				osg::LightSource* ls = new osg::LightSource;
				mt->addChild( ls );
				osg::Light* lt = new osg::Light;
				lt->setLightNum( 1 );
				lt->setAmbient( osg::Vec4( 0., 0., 0., 0. ) );
				lt->setDiffuse( osg::Vec4( 1., 1., 1., 1. ) );
				lt->setSpecular( osg::Vec4( 0., 0., 0., 0. ) );
				lt->setPosition( osg::Vec4( 1., 0., 0., 0. ) );
				ls->setLight( lt );
			}

			// more lighting
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1., 0., 1.5 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
				ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT2, osg::StateAttribute::ON );
				ss->setMode( GL_LIGHT3, osg::StateAttribute::ON );

				osg::LightSource* ls = new osg::LightSource;
				ls->setReferenceFrame( osg::LightSource::RELATIVE_RF );
				mt->addChild( ls );
				osg::Light* lt = new osg::Light;
				lt->setLightNum( 2 );
				lt->setDiffuse( osg::Vec4( 1., .15, .15, 1. ) );
				lt->setPosition( osg::Vec4( 1., 0., 1., 0. ) );
				ls->setLight( lt );

				ls = new osg::LightSource;
				ls->setReferenceFrame( osg::LightSource::ABSOLUTE_RF );
				mt->addChild( ls );
				lt = new osg::Light;
				lt->setLightNum( 3 );
				lt->setDiffuse( osg::Vec4( .15, .75, 1., 1. ) );
				lt->setPosition( osg::Vec4( -1., -1., 0., 0. ) );
				ls->setLight( lt );
			}

		    
			// more lighting
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 1., 0., 1.5 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				ss->setMode( GL_LIGHTING, osg::StateAttribute::ON );
				ss->setMode( GL_LIGHT0, osg::StateAttribute::OFF );
				ss->setMode( GL_LIGHT4, osg::StateAttribute::ON );

				osg::LightSource* ls = new osg::LightSource;
				mt->addChild( ls );
				osg::Light* lt = new osg::Light;
				lt->setLightNum( 4 );
				lt->setDiffuse( osg::Vec4( 1., 1., 1., 1. ) );
				lt->setPosition( osg::Vec4( -3., 0., -4., 1. ) );
				ls->setLight( lt );
			}
			break;
		} // basic tests
	case 1: // fog
		{
			// Linear Fog
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1., 0., -4.5 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				// Add some fog
				osg::Fog* fog = new osg::Fog();
				fog->setMode(osg::Fog::LINEAR);
				fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
				fog->setStart(0.0);
				fog->setEnd(100.0);
				ss->setMode(GL_FOG, osg::StateAttribute::ON);
				ss->setAttribute(fog,osg::StateAttribute::ON);
			}

			// EXP2 Fog
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1., 0., -3.0 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				// Add some fog
				osg::Fog* fog = new osg::Fog();
				fog->setMode(osg::Fog::EXP2);
				fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
				fog->setDensity(0.03f);
				ss->setMode(GL_FOG, osg::StateAttribute::ON);
				ss->setAttribute(fog,osg::StateAttribute::ON);
			}

			// EXP Fog
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 1., 0., -3.0 ) );
			mt->addChild( teapot );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
				// Add some fog
				osg::Fog* fog = new osg::Fog();
				fog->setMode(osg::Fog::EXP);
				fog->setColor(osg::Vec4( 1.0, 0.0, 0.0, 1.0));
				fog->setDensity(0.03f);
				ss->setMode(GL_FOG, osg::StateAttribute::ON);
				ss->setAttribute(fog,osg::StateAttribute::ON);
			}
			break;
		} // 1=fog
	case 2: // texgen box
	case 3: // texgen sphere
		{
            const std::string fileName( "testpattern.png" );
            osg::ref_ptr< osg::Image > image( osgDB::readImageFile( fileName ) );
            if( image == NULL )
            {
                osg::notify( osg::WARN ) << "shaderffp: Can't open data file " << fileName << std::endl;
                return;
            }
            osg::ref_ptr< osg::Texture2D > tex( new osg::Texture2D() );
            tex->setImage( image.get() );

            // Disable lighting for all texture testing.
            root->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );


            // Set up a transform for the eye linear tex gen planes
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -1.0, -0.5, 0. ) );
            mt->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
            root->addChild( mt );

            // Set up for EYE_LINEAR
            osg::ref_ptr< osg::TexGenNode > eltgn = new osg::TexGenNode;
            eltgn->getTexGen()->setMode( osg::TexGen::EYE_LINEAR );
            eltgn->setTextureUnit( 0 );
            mt->addChild( eltgn.get() );


            // Lower left cube: No TexGen.
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -2.5, 0., -2.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );
			}

            // Lower right cube: TexGen on, EYE_LINEAR
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 2.5, 0., -2.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

                ss->setTextureAttribute( 0, eltgn->getTexGen() );
                ss->setTextureMode( 0, GL_TEXTURE_GEN_S, true );
                ss->setTextureMode( 0, GL_TEXTURE_GEN_T, true );
                ss->setTextureMode( 0, GL_TEXTURE_GEN_R, true );
                ss->setTextureMode( 0, GL_TEXTURE_GEN_Q, true );
			}

            // Middle left cube: TexGen on, SPHERE_MAP
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -2.5, 0., 2.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

                osg::TexGen* oltg = new osg::TexGen();
                oltg->setMode( osg::TexGen::SPHERE_MAP );
                ss->setTextureAttributeAndModes( 0, oltg );
			}

            // Middle right cube: TexGen on, OBJECT_LINEAR
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 2.5, 0., 2.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

                osg::TexGen* oltg = new osg::TexGen();
                oltg->setMode( osg::TexGen::OBJECT_LINEAR );
                ss->setTextureAttributeAndModes( 0, oltg );
			}

            // Upper left cube: TexGen on, NORMAL_MAP
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( -2.5, 0., 7.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

                osg::TexGen* nmtg = new osg::TexGen();
                nmtg->setMode( osg::TexGen::NORMAL_MAP );
                ss->setTextureAttributeAndModes( 0, nmtg );
			}

            // Upper right cube: TexGen on, REFLECTION_MAP
			mt = new osg::MatrixTransform(
				osg::Matrix::translate( 2.5, 0., 7.5 ) );
			mt->addChild( testShape );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, tex.get(), osg::StateAttribute::ON );

                osg::TexGen* rmtg = new osg::TexGen();
                rmtg->setMode( osg::TexGen::REFLECTION_MAP );
                ss->setTextureAttributeAndModes( 0, rmtg );
			}
            break;
		} // 2/3-texgen
	case 4: // multitexture/texenv spheres
		{
            const std::string fileNameCheck( "testpatternCheck.png" );
			const std::string fileNameDots( "testpatternDots.png" );
			const std::string fileNameOne( "testpatternOneRed.png" );
			const std::string fileNameTwo( "testpatternTwoGreen.png" );
			const std::string fileNameThree( "testpatternThreeBlue.png" );
			const std::string fileNameFour( "testpatternFourWhite.png" );

			osg::ref_ptr< osg::Image > imageCheck( osgDB::readImageFile( fileNameCheck ) );
            osg::ref_ptr< osg::Image > imageDots( osgDB::readImageFile( fileNameDots ) );

            osg::ref_ptr< osg::Image > imageOne( osgDB::readImageFile( fileNameOne ) );
            osg::ref_ptr< osg::Image > imageTwo( osgDB::readImageFile( fileNameTwo ) );
            osg::ref_ptr< osg::Image > imageThree( osgDB::readImageFile( fileNameThree ) );
            osg::ref_ptr< osg::Image > imageFour( osgDB::readImageFile( fileNameFour ) );

			if( imageCheck == NULL)
            {
                osg::notify( osg::WARN ) << "shaderffp: Can't open data file " << fileNameCheck << std::endl;
                return;
            }
            if( imageDots == NULL )
            {
                osg::notify( osg::WARN ) << "shaderffp: Can't open data file " << fileNameDots << std::endl;
                return;
            }
            osg::ref_ptr< osg::Texture2D > texCheck( new osg::Texture2D() );
            osg::ref_ptr< osg::Texture2D > texDots( new osg::Texture2D() );

			osg::ref_ptr< osg::Texture2D > texOne( new osg::Texture2D() );
            osg::ref_ptr< osg::Texture2D > texTwo( new osg::Texture2D() );
            osg::ref_ptr< osg::Texture2D > texThree( new osg::Texture2D() );
            osg::ref_ptr< osg::Texture2D > texFour( new osg::Texture2D() );

			texCheck->setImage( imageCheck.get() );
            texDots->setImage( imageDots.get() );

			texOne->setImage( imageOne.get() );
            texTwo->setImage( imageTwo.get() );
            texThree->setImage( imageThree.get() );
            texFour->setImage( imageFour.get() );

            // Disable lighting for all texture testing.
            root->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );


            // Lower left: checkerboard
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -3.5, 0., -3.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texCheck.get(), osg::StateAttribute::ON );
			}

            // Lower right: dots
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( 3.5, 0., -3.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texDots.get(), osg::StateAttribute::ON );
			}

            // Middle left: combined (MODULATE) with duplicated texcoord
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -3.5, 0., 3.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texCheck.get(), osg::StateAttribute::ON );
                ss->setTextureAttributeAndModes( 1, texDots.get(), osg::StateAttribute::ON );
			}

            // Middle right: combined (MODULATE) with texgen REFLECTION texcoord
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( 3.5, 0., 3.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texDots.get(), osg::StateAttribute::ON );
                ss->setTextureAttributeAndModes( 1, texCheck.get(), osg::StateAttribute::ON );

				osg::TexGen* oltg = new osg::TexGen();
                oltg->setMode( osg::TexGen::REFLECTION_MAP );
                ss->setTextureAttributeAndModes( 1, oltg );
			}

            // Upper left: combined (REPLACE) with duplicated texcoord
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -3.5, 0., 10.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texCheck.get(), osg::StateAttribute::ON );
                ss->setTextureAttributeAndModes( 1, texDots.get(), osg::StateAttribute::ON );

				osg::ref_ptr<osg::TexEnv> te = new osg::TexEnv;
                te->setMode(osg::TexEnv::REPLACE);
                ss->setTextureAttributeAndModes(1, te, osg::StateAttribute::ON);
			}

            // Upper right: combined (ADD) with duplicated texcoord
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( 3.5, 0., 10.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texCheck.get(), osg::StateAttribute::ON );
                ss->setTextureAttributeAndModes( 1, texDots.get(), osg::StateAttribute::ON );

				osg::ref_ptr<osg::TexEnv> te = new osg::TexEnv;
                te->setMode(osg::TexEnv::ADD);
                ss->setTextureAttributeAndModes(1, te, osg::StateAttribute::ON);
			}

            // Top Left: combined (DECAL) with duplicated texcoord
            mt = new osg::MatrixTransform(
				osg::Matrix::translate( -3.5, 0., 17.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texCheck.get(), osg::StateAttribute::ON );
                ss->setTextureAttributeAndModes( 1, texDots.get(), osg::StateAttribute::ON );

				osg::ref_ptr<osg::TexEnv> te = new osg::TexEnv;
                te->setMode(osg::TexEnv::DECAL);
                ss->setTextureAttributeAndModes(1, te, osg::StateAttribute::ON);
			}

            // Top Right: Four layers, duplicated texcoords, ADD, BLEND, DECAL, MODULATE

			// setup texcoords on additional texture units
			sphereGeometry->setTexCoordArray(2, sphereGeometry->getTexCoordArray(0)); // duplicate texcoords from unit 0 to unit 2
			sphereGeometry->setTexCoordArray(3, sphereGeometry->getTexCoordArray(0)); // duplicate texcoords from unit 0 to unit 3

            mt = new osg::MatrixTransform(
				osg::Matrix::translate( 3.5, 0., 17.5 ) );
			mt->addChild( testSphere );
			root->addChild( mt );
			{
				osg::StateSet* ss = mt->getOrCreateStateSet();
                ss->setTextureAttributeAndModes( 0, texOne.get(), osg::StateAttribute::ON ); // MODULATE
				osg::ref_ptr<osg::TexEnv> te;

				ss->setTextureAttributeAndModes( 1, texTwo.get(), osg::StateAttribute::ON ); // DECAL
				te = new osg::TexEnv;
                te->setMode(osg::TexEnv::DECAL);
                ss->setTextureAttributeAndModes(1, te, osg::StateAttribute::ON);

				ss->setTextureAttributeAndModes( 2, texThree.get(), osg::StateAttribute::ON ); // BLEND
				te = new osg::TexEnv;
                te->setMode(osg::TexEnv::BLEND);
                ss->setTextureAttributeAndModes(2, te, osg::StateAttribute::ON);

				ss->setTextureAttributeAndModes( 3, texFour.get(), osg::StateAttribute::ON ); // ADD
				te = new osg::TexEnv;
                te->setMode(osg::TexEnv::ADD);
                ss->setTextureAttributeAndModes(3, te, osg::StateAttribute::ON);
			}

            break;
		} // 4 multitexture/texenv
	} // switch

}


int
main( int argc, char ** argv )
{
    int testPage( 0 );

	osg::ArgumentParser arguments( &argc, argv );

    //backdropFX::Manager::instance()->setDebugMode(backdropFX::BackdropCommon::debugShaders);
	osg::notify( osg::NOTICE ) << "  -cf\tCull front faces. Use this option to reproduce a bug with lighting back faces." << std::endl;
    bool cullFace( false );

	if( arguments.read( "-cf" ) )
        cullFace = true;

    osg::notify( osg::NOTICE ) << "  -p\tDefault scene page (test set). 0: lighting (default), 1: fog, 2: texgen boxes, 3: texgen spheres, 4: multitex/texenv spheres" << std::endl;
	if( arguments.read( "-p", testPage ) ) {}

	osg::ref_ptr< osg::Group > rootFFP, rootShader;
    rootFFP = new osg::Group;
    rootShader = new osg::Group;
    if( arguments.argc() > 1 )
    {
        // Yes, we really want two copies of the loaded scene graph.
        rootFFP->addChild( osgDB::readNodeFiles( arguments ) );
        rootShader->addChild( osgDB::readNodeFiles( arguments ) );
    }
    else
    {
        defaultScene( rootFFP.get(), testPage);
        defaultScene( rootShader.get(), testPage );
    }

    {
        // Convert FFP to shaders
        backdropFX::ShaderModuleVisitor smv;
        smv.setAttachMain( true );
        smv.setAttachTransform( true );
        rootShader->accept( smv );

        // Must run RebuildShaderSource any time the shader source changes:
        // Not just changing the source, but also setting the source for the
        // first time, inserting new nodes into the scene graph that have source,
        // deleteing nodes that have source, etc.
        backdropFX::RebuildShaderModules rsm;
        rootShader->accept( rsm );
    }

    if( cullFace )
    {
        osg::notify( osg::NOTICE ) << "-cf full front faces." << std::endl;

        osg::CullFace* cf = new osg::CullFace( osg::CullFace::FRONT );
        rootFFP->getOrCreateStateSet()->setAttributeAndModes( cf );
        rootShader->getOrCreateStateSet()->setAttributeAndModes( cf );
    }


    osgViewer::CompositeViewer viewer;
    viewer.setThreadingModel( osgViewer::ViewerBase::SingleThreaded );

    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    unsigned int width, height;
    wsi->getScreenResolution( osg::GraphicsContext::ScreenIdentifier( 0 ), width, height );
    const float aspect( (float)width / (float)( height * 2.f ) );
    const osg::Matrix proj( osg::Matrix::perspective( 50., aspect, 1., 10. ) );

    // Shared event handlers.
    osg::ref_ptr< KbdEventHandler > keh = new KbdEventHandler( rootFFP.get(), rootShader.get() );
    osg::ref_ptr< osgViewer::StatsHandler > sh = new osgViewer::StatsHandler;
    osg::ref_ptr< osgViewer::ThreadingHandler > th = new osgViewer::ThreadingHandler;
    osg::ref_ptr< osgViewer::WindowSizeHandler > wsh = new osgViewer::WindowSizeHandler;
    osg::ref_ptr< osgGA::TrackballManipulator > tb = new osgGA::TrackballManipulator;
    osg::ref_ptr< osgGA::StateSetManipulator > stateSetManipulator = new osgGA::StateSetManipulator;

    osg::ref_ptr< osg::StateSet > camSS;
    osg::ref_ptr<osg::GraphicsContext> gc;

    // view one
    {
        osgViewer::View* view = new osgViewer::View;
        view->setUpViewOnSingleScreen( 0 );
        viewer.addView( view );
        view->setSceneData( rootFFP.get() );

        view->addEventHandler( keh.get() );
        view->addEventHandler( sh.get() );
        view->addEventHandler( th.get() );
        view->addEventHandler( wsh.get() );
        view->setCameraManipulator( tb.get() );

        camSS = view->getCamera()->getOrCreateStateSet();
        stateSetManipulator->setStateSet( camSS.get() );
        view->addEventHandler( stateSetManipulator.get() );

        viewer.realize();
        gc = view->getCamera()->getGraphicsContext();
        view->getCamera()->setViewport( new osg::Viewport( 0, 0, width/2, height ) );
        view->getCamera()->setProjectionMatrix( proj );
    }

    // view two
    {
        osgViewer::View* view = new osgViewer::View;
        viewer.addView(view);
        view->setSceneData( rootShader.get() );

        view->addEventHandler( keh.get() );
        view->addEventHandler( sh.get() );
        view->addEventHandler( th.get() );
        view->addEventHandler( wsh.get() );
        view->setCameraManipulator( tb.get() );

        view->getCamera()->setStateSet( camSS.get() );
        view->addEventHandler( stateSetManipulator.get() );

        view->getCamera()->setViewport( new osg::Viewport( width/2, 0, width/2, height ) );
        view->getCamera()->setGraphicsContext( gc.get() );
        view->getCamera()->setProjectionMatrix( proj );
    }


    return( viewer.run() );
}

// Command line params:
//   The drawer:
//     usmc23_4019.asm.ive
//   Scaled cow:
//     cow.osg.(.25).scale


namespace backdropFX {


/** \page shaderffptest Test: shaderffp

The purpose of this test is to verify correct rendering when replacing FFP
with shader module equivalents. It also demonstrates how to toggle lighting
state via keyboard interaction.

Models to display can be specified on the command line. Otherwise, the test
loads the default scene, which verifies FFP lighting.

The test makes a deep copy of the scene, rendering the original on the left.
The test processes the scene graph copy using the ShaderModuleVisitor, which
replaces FFP state with shader modules and uniforms, and displays it on the
right. The two images should appear identical, but there are some minor
anomalies due to differences in default values, such as ambient scene light color.

Note that \c shaderffp doesn't use the Manager class, which is not typical usage.
Instead, it uses the shader modules feature standalone. This demonstrates that
the shader module feature could be broken out into a separate project if desired.

\section clp Command Line Parameters
<table border="0">
  <tr>
    <td><b>-cf</b></td>
    <td>Cull front faces. Use this option to reproduce a bug with lighting back faces.</td>
  </tr>
  <tr>
    <td><b>-p <pagenumber></b></td>
    <td>Choose the page of test samples to display. Defaults to zero. Pages: 0=lighting, 1=fog, 2=texgen boxes, 3=texgen spheres, 4=multitex/texenv spheres</td>
  </tr>
  <tr>
    <td><b><model> [<models>...]</b></td>
    <td>Model(s) to display. If no models are specified, this test displays several lit teapots.</td>
  </tr>
</table>

\section kbd Keyboard Commands

<table border="0">
  <tr>
    <td><b>L/l</b></td>
    <td>Cycle lighting through three states: Force off, force on, and default (on, but not forced).</td>
  </tr>
</table>

\section handlers Supported OSG Event Handlers
    \li osgViewer::StatsHandler
    \li osgViewer::ThreadingHandler
    \li osgViewer::WindowSizeHandler
    \li osgGA::StateSetManipulator

\section ss Screen Shots

\image html fig04-shaderffp.jpg

This shows the default scene. Each teapot uses a different type of lighting,
exercising sbsolute/relative reference frame positioning, point, directional,
color material on and off, specular highlights, and other FFP lighting effects.
The six teapots on the left are rendered using standard OpenGL 2.x FFP lighting.
The six on the right are 100% shader-based, using backdropFX shader modules to
emulate FFP rendering.

*/


// backdropFX
}
