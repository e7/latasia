#! /usr/bin/python


import sys
import json
import struct
import socket


def pack_sjsonb(ent_type, cargo):
    package = None
    if 3 == ent_type:
        str_cargo = json.dumps(cargo, separators=(",", ":"))
        package = struct.pack(
            "!2I2H2I{}s".format(len(str_cargo)), 0xe78f8a9d, 1000, ent_type, 20, len(str_cargo), 0, str_cargo
        )
    elif 1 == ent_type:
        package = struct.pack(
            "!2I2H2I{}s".format(len(cargo)), 0xe78f8a9d, 1000, ent_type, 20, len(cargo), 0, cargo
        )
    else:
        pass
    return package


def unpack_sjsonb(package):
    magic_no, _, ent_type, ent_offset, ent_sz, _ = struct.unpack("!2I2H2I", package[0:20])
    str_cargo = package[ent_offset : ent_offset + ent_sz]
    cargo = json.loads(str_cargo)
    return ent_type, cargo


if __name__ == "__main__":
    cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        cli.connect(("127.0.0.1", 4321))
    except socket.error as e:
        print e
        sys.exit(1)

    data = "who am i? you sure you wanna know? I am the spider man!\n"
    cli.send(pack_sjsonb(3, {"interface":"start", "deviceid":"123", "time":"20160101010101", "length":"100"}))
    cli.send(pack_sjsonb(1, data))
    cli.send(pack_sjsonb(1, data))
    buf = cli.recv(4096)
    print(buf)
    print unpack_sjsonb(buf)
