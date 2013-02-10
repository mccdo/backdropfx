/*************** <auto-copyright.pl BEGIN do not edit this line> **************
 *
 * backdropFX is (C) Copyright 2009-2011 by Kenneth Mark Bryden
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *************** <auto-copyright.pl END do not edit this line> ***************/

#ifndef __BACKDROPFX_VERSION_H__
#define __BACKDROPFX_VERSION_H__ 1

#include "backdropFX/Export.h"
#include <string>


namespace backdropFX {


/** \defgroup Version Version Utilities */
/*@{*/

#define BACKDROPFX_MAJOR_VERSION 0
#define BACKDROPFX_MINOR_VERSION 2
#define BACKDROPFX_SUB_VERSION 1

/** \brief backdropFX version number as an integer.

C preprocessor integrated version number.
The form is Mmmss, where:
   \li M is the major version.
   \li mm is the minor version (zero-padded).
   \li ss is the sub version (zero padded).

Use this in version-specific code, for example:
\code
   #if( BACKDROPFX_VERSION < 10500 )
      ... code specific to releases before v1.05
   #endif
\endcode
*/
#define BACKDROPFX_VERSION ( \
        ( BACKDROPFX_MAJOR_VERSION * 10000 ) + \
        ( BACKDROPFX_MINOR_VERSION * 100 ) + \
        BACKDROPFX_SUB_VERSION )

/** \brief Run-time access to the backdropFX version number.

Returns BACKDROPFX_VERSION, which is the backdropFX version number as an integer.
\see BACKDROPFX_VERSION
*/
unsigned int BACKDROPFX_EXPORT getVersionNumber();

/** \brief backdropFX version number as a string

Example:
\code
backdropFX version 1.1.0 (10100)
\endcode
*/
std::string BACKDROPFX_EXPORT getVersionString();

/*@}*/


// namespace backdropFX
}

// __BACKDROPFX_VERSION_H__
#endif
