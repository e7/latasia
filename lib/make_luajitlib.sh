#! /bin/bash

# extract
tar xvf LuaJIT-2.0.4.tar.gz
ln -svf LuaJIT-2.0.4 luajit

# make
cd luajit
make
cd ../..

# copy .h and .a file
header_dir="include/luajit"
if [[ ! -d "${header_dir}" ]] ; then
    mkdir -p "${header_dir}"
fi

cp lib/luajit/src/lua.h ${header_dir}
cp lib/luajit/src/lualib.h ${header_dir}
cp lib/luajit/src/luaconf.h ${header_dir}
cp lib/luajit/src/lauxlib.h ${header_dir}
cp lib/luajit/src/libluajit.a lib
