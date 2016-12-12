local cjson = require "cjson"
local sjsonb = require "proto_sjsonb"
local lfs = require "lfs"
local prefix = "/tmp"


function length_of_file(filename)
    local fh = assert(io.open(filename, "rb"))
    local len = assert(fh:seek("end"))
    fh:close()
    return len
end


function handle_request(content_type, data)
    local rsp = {}
    local basedir, path = nil, nil
    if 1 == content_type then
        if nil == lts.front.fd then
            -- just drop it
            return
        end

        lts.front.fd:write(data)
        local curlen = lts.front["curlen"] + #data
        lts.front["curlen"] = curlen
        if curlen >= lts.front["length"] then
            lts.front.fd:close()
            lts.front["fd"] = nil
            rsp = {error_no="200", error_msg="success"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
        end
        return
    elseif 3 == content_type then
        -- 客户端请求开始一个新的上传
        local obj = cjson.decode(data)

        if lts.front["time"] then
            if nil ~= lts.front.fd then
                -- 上一次传输未完成，删除文件
                print("del file")
                basedir = prefix .. "/" .. obj["deviceid"]
                path = basedir .. "/" .. obj["time"]
                lts.front.fd:close()
                os.execute("rm -f " .. path)
            end
        end
        lts.front["deviceid"] = obj["devicedid"]
        lts.front["time"] = obj["time"]
        lts.front["length"] = tonumber(obj["length"])
        lts.front["curlen"] = 0

        -- 创建目录及文件
        basedir = prefix .. "/" .. obj["deviceid"]
        path = basedir .. "/" .. obj["time"]
        lfs.mkdir(basedir)
        lts.front["fd"] = io.open(path, "ab")
        if nil == lts.front.fd then
            rsp = {error_no="500", error_msg="server failed"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
            return
        end

        rsp = {error_no="200", error_msg="success", offset=tostring(length_of_file(path))}
        lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))

        return
    else
        -- just drop it
        return
    end
end


function waiting_reqeust()
    local mn = ""
    while true do
        mn = lts.front.pop_rbuf(1, false)
        if 0xE7 == string.byte(mn, 1) then
            mn = lts.front.pop_rbuf(1, false)
            if 0x8F == string.byte(mn, 1) then
                mn = lts.front.pop_rbuf(1, false)
                if 0x8A == string.byte(mn, 1) then
                    mn = lts.front.pop_rbuf(1, false)
                    if 0x9D == string.byte(mn, 1) then
                        break
                    end
                end
            end
        end
    end

    local hd = lts.front.pop_rbuf(16, true) -- peek header
    local _, version, ent_type, ent_ofst, ent_len, checksum =
        string.unpack(hd, ">IS2I2", ne)

    if 1000 == version and ent_ofst >= 20 and ent_ofst <= 64 then
        lts.front.pop_rbuf(16, false)
        local data = lts.front.pop_rbuf(ent_len, false)
        return ent_type, data
    end
end


function main()
    while true do
        local ent_type, ent_data = waiting_reqeust()
        handle_request(ent_type, ent_data)
    end
end


main()
