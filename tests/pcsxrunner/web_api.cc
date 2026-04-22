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

// Regression coverage for the -no-ui web/API surface PSoXide-2 depends on.
//
// PSoXide-2 drives Redux in -no-ui mode and pulls trusted-reference frames
// via GET /api/v1/gpu/vram/raw. This test exercises that exact end-to-end
// chain, inside a single pcsx-redux-tests binary:
//
//   MainInvoker (-no-ui -softgpu -interpreter -testmode)
//     -> PCSX.startWebServer(18484)              [Lua binding wraps
//                                                 g_emulator->m_webServer
//                                                 ->startServer(loop, port)]
//     -> luv TCP connect to 127.0.0.1:18484
//     -> GET /api/v1/gpu/vram/raw
//     -> WebServer accept -> WebClient -> llhttp parser -> findExecutor
//        -> VramExecutor::execute -> m_gpu->getVRAM() (SoftGPU m_vram16)
//     -> 1 MiB body + Content-Length: 1048576
//     -> PCSX.quit(0) on success / PCSX.quit(1) on any mismatch
//
// Failure modes this catches:
//   - web server failing to bind / accept under -no-ui (loop ownership,
//     SettingsLoaded bypass, TCP setup regressions)
//   - VramExecutor path regressing (route match, 200 framing, body size)
//   - soft-GPU getVRAM() shape regressing (not 1024*512*2 bytes)
//   - llhttp / uriparser / executor dispatch regressions on the request path
//   - the Lua startWebServer binding regressing (direct PSoXide-2 dep)
//
// Port 18484 is non-default (Redux default is 8080) to avoid colliding with
// any dev-local Redux instance a contributor may have running. No -luacov:
// same rationale as the other no-ui regressions in this directory — the
// path under test is C++/Lua only, and luacov would write into the repo
// root at shutdown, which is noise in sandboxed CI.

#include "gtest/gtest.h"
#include "main/main.h"

TEST(WebAPI, NoUIVramRawGet) {
    MainInvoker invoker("-no-ui", "-bios", "src/mips/openbios/openbios.bin", "-testmode", "-interpreter", "-softgpu",
                        "-dofile", "tests/pcsxrunner/web_api.lua");
    int ret = invoker.invoke();
    EXPECT_EQ(ret, 0);
}
