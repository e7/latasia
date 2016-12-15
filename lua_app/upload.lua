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
    local homedir, path = nil, nil
    if 1 == content_type then
        if not lts.front.transfer or not lts.front.transfer.fd then
            -- just drop it
            return
        end

        lts.front.transfer.fd:write(data)
        local lastlen = tonumber(lts.front.transfer.curlen) + #data
        if lastlen < tonumber(lts.front.transfer.length) then
            lts.front.transfer.curlen = lastlen
        else
            print("upload finished")
            lts.front.transfer.fd:close()
            rsp = {
                error_no="201", error_msg="end",
                time=lts.front.transfer.time
            }
            lts.front["transfer"] = nil
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
        end
        return
    elseif 3 == content_type then
        local obj = cjson.decode(data)

        -- 参数检查
        if "start" ~= obj.interface then
            rsp = {error_no="400", error_msg="unsupport request"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
            return
        end

        if not obj.deviceid or not obj.time or not obj.length then
            rsp = {error_no="401", error_msg="missing argument"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
            return
        end

        if nil ~= lts.front.transfer then
            -- 上一个文件未完成
            rsp = {error_no="400", error_msg="unsupport request"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
            return
        end

        lts.front["transfer"] = {
            deviceid=obj.devicedid, time=obj.time, length=obj.length, curlen="0"
        }

        -- 创建目录及文件
        local date = string.sub(obj.time, 1, 8)
        local basedir = prefix .. "/" .. obj.deviceid
        lfs.mkdir(basedir)

        homedir = basedir .. "/" .. date
        path = homedir .. "/" .. obj.time .. ".mp4"
        lfs.mkdir(homedir)
        lts.front.transfer["fd"] = io.open(path, "ab")
        if nil == lts.front.transfer.fd then
            rsp = {error_no="500", error_msg="server failed"}
            lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))
            return
        end

        local last_file_len = length_of_file(path)
        lts.front.transfer["curlen"] = tostring(last_file_len)
        rsp = {error_no="200", error_msg="success", offset=tostring(last_file_len)}
        lts.front.push_sbuf(sjsonb.encode(3, cjson.encode(rsp)))

        return
    else
        -- just drop it
        print("unknown content type")
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

    if 1000 == version
        and ent_ofst >= 20 and ent_ofst <= 64
        and ent_len > 0 and ent_len < 12000 then
        lts.front.pop_rbuf(16, false)
        local data = lts.front.pop_rbuf(ent_len, false)
        return ent_type, data
    end
end


function main()
    while true do
        if lts.front.closed then
            print("connection closed by peer")
            break
        end

        local ent_type, ent_data = waiting_reqeust()
        handle_request(ent_type, ent_data)
    end
end


main()
