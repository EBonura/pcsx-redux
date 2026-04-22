-- Regression driver for WebAPI.NoUIVramRawGet.
--
-- Brings up the web server under -no-ui and exercises the exact capture
-- endpoint PSoXide-2's trusted-reference pipeline calls to pull frames.
-- The request goes through the real TCP path, the HTTP parser, the
-- VramExecutor, and g_emulator->m_gpu->getVRAM() on the soft GPU.

local port = 18484

PCSX.startWebServer(port)

local client = luv.new_tcp()
local timer = luv.new_timer()
local buf = {}
local done = false

local function finish(code, msg)
    if done then return end
    done = true
    pcall(function() luv.timer_stop(timer); luv.close(timer) end)
    pcall(function() luv.close(client) end)
    if msg then io.stderr:write('WebAPI failure: ' .. msg .. '\n') end
    PCSX.quit(code)
end

luv.timer_start(timer, 5000, 0, function() finish(1, 'timed out waiting for response') end)

luv.tcp_connect(client, '127.0.0.1', port, function(connectErr)
    if connectErr then finish(1, 'connect: ' .. tostring(connectErr)); return end
    luv.write(client, 'GET /api/v1/gpu/vram/raw HTTP/1.1\r\nHost: localhost\r\n\r\n')
    luv.read_start(client, function(readErr, chunk)
        if readErr and readErr ~= 'EOF' then finish(1, 'read: ' .. tostring(readErr)); return end
        if chunk then
            buf[#buf + 1] = chunk
            return
        end
        -- EOF: validate
        local full = table.concat(buf)
        local headerEnd = full:find('\r\n\r\n', 1, true)
        if not headerEnd then finish(1, 'no header terminator; got ' .. #full .. ' bytes'); return end
        local headers = full:sub(1, headerEnd - 1)
        local body = full:sub(headerEnd + 4)
        local status = headers:match('^HTTP/1%.1 (%d+)')
        local clen = tonumber(headers:match('[Cc]ontent%-[Ll]ength:%s*(%d+)'))
        if status ~= '200' then finish(1, 'status ' .. tostring(status)); return end
        if clen ~= 1048576 then finish(1, 'content-length ' .. tostring(clen)); return end
        if #body ~= 1048576 then finish(1, 'body size ' .. #body .. ' != 1048576'); return end
        finish(0, nil)
    end)
end)
