//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Platform-independent Directory functions
//
//=============================================================================
#ifndef __AGS_CN_UTIL__DIRECTORY_H
#define __AGS_CN_UTIL__DIRECTORY_H

#include "util/string.h"

#if defined (WINDOWS_VERSION)
#undef GetCurrentDirectory  // undef the declaration from winbase.h
#endif

namespace AGS
{
namespace Common
{

namespace Directory
{
    // Sets current working directory, returns the resulting path
    String SetCurrentDirectory(const String &path);
    // Gets current working directory
    String GetCurrentDirectory();
} // namespace Directory

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__DIRECTORY_H
