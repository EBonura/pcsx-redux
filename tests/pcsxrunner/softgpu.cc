/***************************************************************************
 *   Copyright (C) 2026 PCSX-Redux authors                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

// Regression coverage for the -no-ui soft-GPU display paths.
//
// src/gpu/soft/gpu.cc::updateDisplay / changeDispOffsetsX / changeDispOffsetsY
// call glClearColor/glClear. Under -no-ui the backend is TUI, not GUI, and
// gl3w was never loaded — those calls used to SIGSEGV on null function
// pointers. The fix guards each site with dynamic_cast<GUI *>(m_ui).
//
// The MIPS payload drives disableDisplay + vblank (updateDisplay branch),
// setHorizontalRange (changeDispOffsetsX else branch), and setVerticalRange
// (changeDispOffsetsY branch). Without the guards this test crashes before
// pcsx_exit(0) is reached; with them it returns 0.
//
// -no-ui is essential — the crash only reproduces under the TUI backend
// where m_ui is not a GUI and gl3w is not loaded. -softgpu pins the soft
// renderer so the test does not depend on the default-GPU setting.
// -interpreter avoids dynarec JIT variance in the CI signal. No -luacov
// here: coverage is for Lua code and this path only touches C++/MIPS,
// and luacov writes luacov.stats.out into the repo root at shutdown,
// which is noise in sandboxed runs.

#include "gtest/gtest.h"
#include "main/main.h"

TEST(SoftGPU, NoUIInterpreter) {
    MainInvoker invoker("-no-ui", "-run", "-bios", "src/mips/openbios/openbios.bin", "-testmode", "-interpreter",
                        "-softgpu", "-loadexe", "src/mips/tests/softgpu/softgpu.ps-exe");
    int ret = invoker.invoke();
    EXPECT_EQ(ret, 0);
}
