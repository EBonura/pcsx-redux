-- Regression driver for HeadlessCapture.NoUISoftGPUScreenshot.
--
-- Exercises the same soft-GPU VRAM-read path that backs the web-server
-- capture endpoints (/api/v1/gpu/vram/raw, /api/v1/screen/still) which
-- PSoXide-2 calls to pull trusted-reference frames.

local ok, err = pcall(function()
    local ss = PCSX.GPU.takeScreenShot()
    assert(type(ss) == 'table', 'takeScreenShot did not return a table')
    assert(ss.data ~= nil, 'takeScreenShot data slice is nil')
    assert(type(ss.width) == 'number', 'width is not numeric')
    assert(type(ss.height) == 'number', 'height is not numeric')
    assert(ss.bpp == 0 or ss.bpp == 1, 'bpp must be BPP_16 (0) or BPP_24 (1)')
end)
if not ok then
    io.stderr:write('HeadlessCapture failure: ' .. tostring(err) .. '\n')
    PCSX.quit(1)
else
    PCSX.quit(0)
end
