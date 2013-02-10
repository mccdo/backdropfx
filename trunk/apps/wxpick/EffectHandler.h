// Copyright (c) 2010 Skew Matrix Software LLC. All rights reserved.

#ifndef __EFFECT_HANDLER_H__
#define __EFFECT_HANDLER_H__ 1

#include <osgGA/GUIEventHandler>
#include <wx/treectrl.h>

/** \cond */
class EffectHandler : public osgGA::GUIEventHandler
{
public:
    EffectHandler( wxTreeCtrl* tree );
    ~EffectHandler();

    bool handle( const osgGA::GUIEventAdapter& ea,
        osgGA::GUIActionAdapter& aa );

protected:
    wxTreeCtrl* _tree;

    void enableGlow( osg::Node* node );
    osg::Group* parentGroup( osg::Node* node );
};
/** \endcond */


// __EFFECT_HANDLER_H__
#endif
