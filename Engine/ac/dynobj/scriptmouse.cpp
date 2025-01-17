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
#include "ac/dynobj/scriptmouse.h"
#include "script/cc_common.h"

int32_t ScriptMouse::ReadInt32(void *address, intptr_t offset)
{
    switch (offset)
    {
    case 0: return x;
    case 4: return y;
    default:
        cc_error("ScriptMouse: unsupported variable offset %d", offset);
        return 0;
    }
}

void ScriptMouse::WriteInt32(void *address, intptr_t offset, int32_t val)
{
    switch (offset)
    {
    case 0:
    case 4:
        cc_error("ScriptMouse: attempt to write readonly variable at offset %d", offset);
        break;
    default:
        cc_error("ScriptMouse: unsupported variable offset %d", offset);
        break;
    }
}
