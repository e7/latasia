#! /bin/bash

# extract
if [[ ! -x lua ]] ; then
    tar xvf lua-5.1.5.tar.gz
    ln -svf lua-5.1.5 lua
fi

# make
cd lua
make linux
cd ../..

# copy .h and .a file
header_dir="include/lua"
if [[ ! -d "${header_dir}" ]] ; then
    mkdir -p "${header_dir}"
fi

cp lib/lua/src/lua.h ${header_dir}
cp lib/lua/src/lualib.h ${header_dir}
cp lib/lua/src/luaconf.h ${header_dir}
cp lib/lua/src/lauxlib.h ${header_dir}
cp lib/lua/src/liblua.a lib
