local cjson = require "cjson"
local ffi = require "ffi"
local pack = require "ltspack"


function find_package(data, seek)
    local ne, magic_no

    while true do
        ne, magic_no = string.unpack(data, ">I", seek)

        if ne == seek or 0xE78F8A9D == magic_no then
            return ne, magic_no
        end

        seek = seek + 1
    end
end


function main()
    local sock, err = lts.socket.tcp()
    sock.connect("a.b.c")

    local rbuf = lts.ctx.pop_rbuf()
    print(rbuf)
    local ne, magic_no = find_package(rbuf, 1)
    if nil == magic_no then
        print("no valid package")
    	lts.ctx.push_sbuf(rbuf)
        return
    end

    local ne, version, ent_ofst, ent_len, checksum
        = string.unpack(rbuf, ">I4", ne)
    local ne, data = string.unpack(rbuf, string.format("A%d", ent_len), ne)
    print(data)

    lts.ctx.push_sbuf(rbuf)
end

main()
