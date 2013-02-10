//
// Copyright (c) 2008 Blue Newt Software LLC and Skew Matrix Software LLC.
// All rights reserved.
//


#ifndef _WXSIMPLEVIEWERWX_H_
#define _WXSIMPLEVIEWERWX_H_

#include <wx/cmdline.h>
#include <wx/defs.h>
#include <wx/app.h>
#include <wx/cursor.h>
#include <wx/glcanvas.h>
#include <wx/statusbr.h>
#include <osgViewer/Viewer>
#include <string>

#include <osgWxTree/TreeControl.h>
#include <osgWxTree/PickHandler.h>


/** \cond */

class GraphicsWindowWX;

class OSGCanvas
    : public wxGLCanvas
{
public:
    OSGCanvas( wxWindow * parent, wxWindowID id = wxID_ANY,
               const wxPoint & pos = wxDefaultPosition,
               const wxSize & size = wxDefaultSize, long style = 0,
               const wxString & name = wxT( "TestGLCanvas" ),
               int * attributes = 0 );

    virtual ~OSGCanvas();

    void SetGraphicsWindow( osgViewer::GraphicsWindow * gw )   { _graphics_window = gw; }

    void OnPaint( wxPaintEvent & event );
    void OnSize( wxSizeEvent & event );
    void OnEraseBackground( wxEraseEvent & event );

    void OnChar( wxKeyEvent & event );
    void OnKeyUp( wxKeyEvent & event );

    void OnMouseEnter( wxMouseEvent & event );
    void OnMouseDown( wxMouseEvent & event );
    void OnMouseUp( wxMouseEvent & event );
    void OnMouseMotion( wxMouseEvent & event );

    void UseCursor( bool value );
private:
    DECLARE_EVENT_TABLE()

    osg::ref_ptr< osgViewer::GraphicsWindow > _graphics_window;

    wxCursor _oldCursor;
};

class GraphicsWindowWX
    : public osgViewer::GraphicsWindow
{
public:
    GraphicsWindowWX( OSGCanvas * canvas );
    ~GraphicsWindowWX();

    void init();

    //
    // GraphicsWindow interface
    //
    void grabFocus();
    void grabFocusIfPointerInWindow();
    void useCursor( bool cursorOn );

    bool makeCurrentImplementation();
    void swapBuffersImplementation();

    // not implemented yet...just use dummy implementation to get working.
    virtual bool valid() const { return( true ); }

    virtual bool realizeImplementation() { return( true ); }

    virtual bool isRealizedImplementation() const { return( true ); }

    virtual void closeImplementation() {}

    virtual bool releaseContextImplementation() { return( true ); }

private:
    // XXX need to set _canvas to NULL when the canvas is deleted by
    // its parent. for this, need to add event handler in OSGCanvas
    OSGCanvas *  _canvas;
};


class MainFrame
    : public wxFrame
{
public:
    MainFrame( wxFrame * frame, const wxString & title, const wxPoint & pos,
               const wxSize & size, long style = wxDEFAULT_FRAME_STYLE );

    void SetViewer( osgViewer::Viewer * viewer );
    void OnIdle( wxIdleEvent & event );
private:
    wxStatusBar *                   statusbar_;
    osg::ref_ptr< osgViewer::Viewer > viewer_;

    DECLARE_EVENT_TABLE()
};

/* Define a new application type */
class wxOsgApp
    : public wxApp
{
public:
    virtual bool OnInit();
    virtual void OnInitCmdLine( wxCmdLineParser& parser );
    virtual bool OnCmdLineParsed( wxCmdLineParser& parser );

private:
    bool flat_mode_;
    std::vector< std::string > files_;

    osgWxTree::TreeControl* _tree;
};

DECLARE_APP( wxOsgApp )

/** \endcond */

#endif // _WXSIMPLEVIEWERWX_H_
