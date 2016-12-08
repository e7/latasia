local cjson = require "cjson"
local sjsonb = require "proto_sjsonb"
--local ffi = require "ffi"
--local pack = require "ltspack"
--
--
--function find_package(data, seek)
--    local ne, magic_no
--
--    while true do
--        ne, magic_no = string.unpack(data, ">I", seek)
--
--        if ne == seek or 0xE78F8A9D == magic_no then
--            return ne, magic_no
--        end
--
--        seek = seek + 1
--    end
--end


function main()
    print("main")
    local sock, err = lts.socket.tcp()

    if 200 ~= err then
        print("create socket failed", err)
        return
    end

    err = sock:connect("127.0.0.1", 30976)

    if 200 ~= err then
        print("connect failed", err)
        return
    end

    print(sock:send("asdfasf"))
    print("send returned")
    --local d1, err = sock:receive(4)
    --if 200 == err then
    --    print("d1", d1)
    --end
    --local d2, err = sock:receive(4)
    --if 200 == err then
    --    print("d2", d2)
    --end

    print("closing")
    sock:close()

    local rbuf = lts.ctx.pop_rbuf()
    print(rbuf)
    lts.ctx.push_sbuf(rbuf)
    --local ne, magic_no = find_package(rbuf, 1)
    --if nil == magic_no then
    --    print("no valid package")
    --	lts.ctx.push_sbuf(rbuf)
    --    return
    --end

    --local ne, version, ent_ofst, ent_len, checksum
    --    = string.unpack(rbuf, ">I4", ne)
    --local ne, data = string.unpack(rbuf, string.format("A%d", ent_len), ne)
    --print(data)

end

main()
