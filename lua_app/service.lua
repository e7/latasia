local cjson = require("cjson")

function main()
    local lts = lts_context();
    print(lts.rbuf)
    lts.push_sbuf()
    print("service")
end
print("init")
