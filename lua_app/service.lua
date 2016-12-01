local cjson = require "cjson"
local ffi = require "ffi"
local pack = require "ltspack"


local function find_package(data, seek)
    local ne, magic_no

    while true do
        ne, magic_no = string.unpack(data, ">I", seek)

        if ne == seek or 0xE78F8A9D == magic_no then
            return ne, magic_no
        end

        seek = seek + 1
    end
end


function main(ctx)
    local rbuf = ctx.pop_rbuf()

    local ne, magic_no = find_package(rbuf, 1)
    if 1 == ne then
        print("no valid package")
    end

    local ne, version, ent_ofst, ent_len, checksum
        = string.unpack(rbuf, ">I4", ne)
    local ne, data = string.unpack(rbuf, string.format("A%d", ent_len), ne)
    print(data)

    ctx.push_sbuf(rbuf)
end
