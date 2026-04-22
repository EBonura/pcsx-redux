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

// Regression coverage for the -no-ui headless capture surface.
//
// PSoXide-2's trusted-reference pipeline drives PCSX-Redux in -no-ui mode and
// pulls frames via the web API (/api/v1/gpu/vram/raw and /api/v1/screen/still).
// Both endpoints reach into PCSX::SoftGPU::impl to read the software VRAM:
//   - VramExecutor  -> g_emulator->m_gpu->getVRAM()           (web-server.cc)
//   - ScreenExecutor-> g_emulator->m_gpu->takeScreenShot()    (web-server.cc)
// takeScreenShot is also exposed to Lua via PCSX.GPU.takeScreenShot (the
// Lua bootstrap path PSoXide-2 loads). This test exercises that exact Lua
// entry point, which covers the same soft-GPU data plane that backs the raw
// VRAM endpoint: an m_vram16 read with no OpenGL dependency.
//
// Failure modes this would catch:
//   - takeScreenShot regressing to touch gl3w under -no-ui (SIGSEGV, like the
//     updateDisplay guards we just added in softgpu.cc)
//   - the PCSX.GPU Lua FFI binding breaking under the TUI bootstrap
//   - the LuaScreenShot -> Slice wrapper breaking headless
//   - -no-ui launch itself regressing before Lua -exec runs
//
// -softgpu pins the software renderer so the test does not depend on the
// default GPU setting. -interpreter avoids dynarec JIT variance. -no-ui
// (without -cli) matches the exact launch mode PSoXide-2 uses, so a CLI-only
// regression would not hide a -no-ui breakage. No -luacov: this path is
// C++/Lua only, and luacov writes luacov.stats.out into the repo root at
// shutdown, which is noise in sandboxed runs.

#include "gtest/gtest.h"
#include "main/main.h"

TEST(HeadlessCapture, NoUISoftGPUScreenshot) {
    MainInvoker invoker("-no-ui", "-bios", "src/mips/openbios/openbios.bin", "-testmode", "-interpreter", "-softgpu",
                        "-dofile", "tests/pcsxrunner/headless_capture.lua");
    int ret = invoker.invoke();
    EXPECT_EQ(ret, 0);
}
