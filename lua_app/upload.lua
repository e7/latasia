local cjson = require "cjson"
local sjsonb = require "proto_sjsonb"


function main()
    local rbuf = lts.front.pop_rbuf()
    local proto_type, data, err = sjsonb.decode(rbuf)
    if err then
        -- just drop it
        return
    end

    print(proto_type)
    if 1 == proto_type then
        return
    elseif 3 == proto_type then
        local obj = cjson.decode(data)
        return
    else
        -- just drop it
        return
    end

    --print(rbuf)
    --lts.front.push_sbuf(rbuf)

    --local ne, magic_no = find_package(rbuf, 1)
    --if nil == magic_no then
    --    print("no valid package")
    --	lts.front.push_sbuf(rbuf)
    --    return
    --end

    --local ne, version, ent_ofst, ent_len, checksum
    --    = string.unpack(rbuf, ">I4", ne)
    --local ne, data = string.unpack(rbuf, string.format("A%d", ent_len), ne)
    --print(data)

end

main()
