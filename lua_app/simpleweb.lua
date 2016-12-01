local cjson = require "cjson"
local ffi = require "ffi"

-- 分隔字符串
function string.split(self, sep)
    local sep, fields = sep or "\t", {}
    local pattern = string.format("([^%s]+)", sep)
    self:gsub(pattern, function(c) fields[#fields+1] = c end)
    return fields
end


function main(ctx)
    local rbuf = ctx.pop_rbuf()

    local hs, he = string.find(rbuf, "\r\n\r\n");
    local header = string.sub(rbuf, 1, hs - 1)
    local body = string.sub(rbuf, he + 1)

    --print(header)
    local lines = string.split(header, "\r\n")
    local req_line = string.split(lines[1], "%s")

    if "GET" == req_line[1] then
    elseif "POST" == req_line[1] then
    else
    end

    for i = 2, #lines, 1 do
    end
end
