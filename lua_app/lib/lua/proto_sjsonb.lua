local ltspack = require "ltspack"

local _M = {_VERSION = '0.01'}


function _M.encode(proto_type, content)
    local rslt = string.pack(">I2S2I2A",
        0xE78F8A9D, 1000, proto_type, 20, #content, 0, content)
    return rslt
end


-- 返回未决数据偏移，内容类型，内容数据，错误信息
function _M.decode(bytebuffer, ofst)
    local find_pack = function (bytebuffer, ofst)
        local ne, magic_no

        while true do
            ne, magic_no = string.unpack(bytebuffer, ">I", ofst)
            if ne == ofst or 0xE78F8A9D == magic_no then
                return ne, magic_no
            end

            ofst = ofst + 1
        end
    end

    -- 寻找有效标识
    local ne, magic_no = find_pack(bytebuffer, 1)
    if nil == magic_no then
        return 1, 0, nil, "unknown data"
    end

    local next_ofst, version, ent_type, ent_ofst, ent_len, checksum =
        string.unpack(bytebuffer, ">IS2I2", ne)
    if ne == next_ofst then
        return ne, 0, nil, "no enough data"
    end

    -- 各种参数校验
    if 0 then
    end

    local ne, data = string.unpack(bytebuffer,
                                   string.format("A%d", ent_len), ne)
    return ne, ent_type, data, nil

end

return _M
