local cjson = require "cjson"
local ffi = require "ffi"
local pack = require "pack"

function main()
    local lts = lts_context()
    local ne, magic_no = string.unpack(lts.rbuf, ">I", 1)
    print(ne, magic_no)
    lts.push_sbuf("asfsafdf")
    print("service")
end
print("init")
