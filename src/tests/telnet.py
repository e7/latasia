#! /usr/bin/python
# -*- coding:utf-8 -*-


import socket


if "__main__" == __name__:
    skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    skt.connect(("127.0.0.1", 6742))

    while True:
        try:
            data = input("> ").encode()
        except EOFError as e:
            break

        skt.send(data)
        rsp = skt.recv(4096).decode()
        if not rsp:
            break;
        print(": " + rsp)

    skt.close()
