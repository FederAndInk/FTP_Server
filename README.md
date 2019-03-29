# FTP Server

Project in c to make a simple FTP client/server

## Compilation

mkdir build
cd build
cmake ..
make

Depends on:

- OpenSSL

## execute

build/src/ftp_client
build/src/ftp_server

## Features

- [x] Multi-client
- [x] Simple file transfert
- [x] file loading and writing by blocs (mmap/buffer)
- [x] command get
- [x] progress bar
- [X] detailed error handling (through errno)
- [X] crash client handling (resume download)
- [ ] load balancer (multiple server)
- [X] persistant connection
- [X] put file
- [ ] lock file on transfert (for other get/put)
- [ ] ls
- [ ] pwd
- [ ] cd
- [ ] mkdir
- [ ] rm
- [ ] rm -r
- [ ] cd (limited to the ftp server root)
- [ ] authentication
