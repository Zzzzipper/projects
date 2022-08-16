#!/bin/bash

if [ "$1" == "upload" ]; then
    echo "-- Create archive.." 
    tar czf tester.tar.gz ./bin ./assets/ monitor.html
    echo "-- Done.." 
fi

# "devel@193.161.86.133" "epozdnyakov@193.161.86.135" "devel@193.161.86.141" "devel@193.161.86.154"

hosts=("devel@193.161.86.132" "epozdnyakov@193.161.86.135" "devel@193.161.86.141")

for d in ${hosts[@]}; do
if [ "$1" == "upload" ]; then
    echo "-- Copy archive to host $d.." 
    scp ./tester.tar.gz $d:~
    echo "-- Done.." 
    ssh $d 'mkdir -p ~/tester'
    ssh $d 'mkdir -p ~/tester/htdocs'
    ssh $d 'cd ~ && tar xzf tester.tar.gz -C ~/tester'
#     ssh $d 'mv ~/tester/bin/test.html ~/tester/htdocs'
    ssh $d 'mkdir -p ~/tester/htdocs/assets'
    ssh $d 'mkdir -p ~/tester/logs'
    ssh $d 'cp -R ~/tester/assets/* ~/tester/htdocs/assets'
    ssh $d 'rm -R ~/tester/assets'
    ssh $d 'mv ~/tester/monitor.html ~/tester/htdocs'
fi


if [ "$1" == "upload" ] || [ "$1" == "restart" ]; then
    echo "-- Restart tester on $d.."
    ssh -t $d 'echo $(cat ~/.credentials) |  sudo -S systemctl restart tester.service'
    echo "-- Success.."
fi
done

echo "-- All done.." 
