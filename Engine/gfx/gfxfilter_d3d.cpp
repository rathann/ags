//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_HAS_DIRECT3D

#include <d3d9.h>
#include "gfx/gfxfilter_d3d.h"

namespace AGS
{
namespace Engine
{
namespace D3D
{

const GfxFilterInfo D3DGfxFilter::FilterInfo = GfxFilterInfo("StdScale", "Nearest-neighbour");

const GfxFilterInfo &D3DGfxFilter::GetInfo() const
{
    return FilterInfo;
}

int D3DGfxFilter::GetSamplerStateForStandardSprite()
{
    return D3DTEXF_POINT;
}

void D3DGfxFilter::SetSamplerStateForStandardSprite(void *direct3ddevice9)
{
    IDirect3DDevice9* d3d9 = ((IDirect3DDevice9*)direct3ddevice9);
    d3d9->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    d3d9->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
}

bool D3DGfxFilter::NeedToColourEdgeLines()
{
    return false;
}

} // namespace D3D
} // namespace Engine
} // namespace AGS

#endif // AGS_HAS_DIRECT3D
