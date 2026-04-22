/*

MIT License

Copyright (c) 2026 PCSX-Redux authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdint.h>

#include "common/hardware/gpu.h"
#include "common/hardware/hwregs.h"
#include "common/hardware/irq.h"
#include "common/syscalls/syscalls.h"

#undef unix
#define CESTER_NO_SIGNAL
#define CESTER_NO_TIME
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#include "exotic/cester.h"

// Drive the soft-GPU display paths that, under -no-ui, dereference gl3w
// function pointers that are never loaded when there is no GUI. Without
// the dynamic_cast<GUI *> guards in src/gpu/soft/gpu.cc, running these
// under -no-ui SIGSEGVs before pcsx_exit(0) is reached.
//
// Guarded sites exercised:
//   1. updateDisplay() — hit when vblank fires with m_doVSyncUpdate=true
//      and m_softDisplay.Disabled=true (display-disable branch).
//   2. changeDispOffsetsX() inner else — hit by setHorizontalRange with
//      lx < DisplayMode.x (320 * 0x500 / 2560 = 160 < 320).
//   3. changeDispOffsetsY() — hit by setVerticalRange that changes Height,
//      which in turn changes Range.y0 vs the previous value.

#define WAIT_VBLANK_EDGE() do {                        \
    uint32_t _imask = IMASK;                           \
    IMASK = _imask | IRQ_VBLANK;                       \
    while ((IREG & IRQ_VBLANK) == 0);                  \
    IREG &= ~IRQ_VBLANK;                               \
    IMASK = _imask;                                    \
} while (0)

CESTER_BEFORE_ALL(softgpu_inst,
    struct DisplayModeConfig cfg = {
        .hResolution = HR_320,
        .vResolution = VR_240,
        .videoMode = VM_NTSC,
        .colorDepth = CD_15BITS,
        .videoInterlace = VI_OFF,
        .hResolutionExtended = HRE_NORMAL,
    };
    setDisplayMode(&cfg);
    setDisplayArea(0, 0);
)

// write1(CtrlHorizontalDisplayRange) -> changeDispOffsetsX()
// lx = 320 * 0x500 / 2560 = 160 < 320, so the inner else branch runs.
CESTER_TEST(softgpu_changeDispOffsetsX_no_crash, softgpu_inst,
    setHorizontalRange(0, 0x500);
    setHorizontalRange(0, 0x400);
    cester_assert_equal(0, 0);
)

// write1(CtrlVerticalDisplayRange) -> changeDispOffsetsY() when Height
// changes. Two distinct ranges ensure iO != Range.y0 on the second call.
CESTER_TEST(softgpu_changeDispOffsetsY_no_crash, softgpu_inst,
    setVerticalRange(16, 255);
    setVerticalRange(48, 224);
    cester_assert_equal(0, 0);
)

// disableDisplay + vblank -> updateDisplay() with Disabled=true.
// m_doVSyncUpdate is kicked by setDisplayArea, so vblank runs the branch.
CESTER_TEST(softgpu_updateDisplay_disabled_no_crash, softgpu_inst,
    disableDisplay();
    setDisplayArea(0, 0);
    WAIT_VBLANK_EDGE();
    WAIT_VBLANK_EDGE();
    enableDisplay();
    cester_assert_equal(0, 0);
)
