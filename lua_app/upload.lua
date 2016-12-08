local cjson = require "cjson"
local sjsonb = require "proto_sjsonb"


function main()
    print("main")
    local sjs = sjsonb.encode("lijia")
    print(sjs)
    local sock, err = lts.socket.tcp()

    if 200 ~= err then
        print("create socket failed", err)
        return
    end

    err = sock:connect("127.0.0.1", 55555)

    if 200 ~= err then
        print("connect failed", err)
        return
    end

    print(sock:send(sjs))
    print("send returned")
    sock:close()

    --local rbuf = lts.ctx.pop_rbuf()
    --print(rbuf)
    --lts.ctx.push_sbuf(rbuf)

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
