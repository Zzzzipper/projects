#!/bin/sh

# rm ./server
# go build 

sshpass -p '1Q2w3e4rr' ssh -tt zipper@192.168.1.50 'echo 1Q2w3e4rr | sudo -S -s /bin/bash -c "service go-web stop;rm /home/zipper/go-web/server; rm /home/zipper/go-web/*.so.1.0.0; rm -rf /home/zipper/go-web/hosts"'
sshpass -p '1Q2w3e4rr' scp ./server zipper@192.168.1.50:~/go-web
# sshpass -p '1Q2w3e4rr' scp ../../x64/debug/libMdxWrapper_x64.so.1.0.0 zipper@192.168.1.50:~/go-web
# sshpass -p '1Q2w3e4rr' scp ../../x64/debug/libScdMdx_x64.so.1.0.0 zipper@192.168.1.50:~/go-web
sshpass -p '1Q2w3e4rr' scp -r ./hosts zipper@192.168.1.50:~/go-web
sshpass -p '1Q2w3e4rr' ssh -tt zipper@192.168.1.50 'echo 1Q2w3e4rr | sudo -S -s /bin/bash -c "service go-web start"'
# sshpass -p '1Q2w3e4rr' zipper@192.168.1.50 -C "sudo service server stop"

# sshpass -p '1Q2w3e4rr' scp -f ./server zipper@192.168.1.50:~/server
# scp -rf -f ./hosts zipper@192.168.1.50:~/server
