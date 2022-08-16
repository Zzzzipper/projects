#!/bin/bash

if [ "$1" == "upload" ]; then
    echo "-- Create archive.." 
    tar czf alm.receiver.tar.gz ./bin ./assets/ monitor.html
    echo "-- Done.." 
fi
  
hosts=("reciver@5.43.224.93")
key='-i ~/.ssh/id_rsa'
port='2722'

for d in ${hosts[@]}; do
if [ "$1" == "upload" ]; then
    echo "-- Copy archive to host $d.." 
    scp -P $port ./alm.receiver.tar.gz $d:~
    echo "-- Done.." 
    ssh $key -p $port $d 'mkdir -p ~/receiver'
    ssh $key -p $port $d 'mkdir -p ~/receiver/htdocs'
    ssh $key -p $port $d 'cd ~ && tar xzf alm.receiver.tar.gz -C ~/receiver'
#   ssh $d 'mv ~/receiver/bin/test.html ~/receiver/htdocs'
#   ssh $key -p $port $d 'rm -rf ~/receiver/bin'
#   ssh $key -p $port $d 'mv ~/receiver/bin ~/receiver/bin'
    ssh $key -p $port $d 'mkdir -p ~/receiver/htdocs/assets'
    ssh $key -p $port $d 'mkdir -p ~/receiver/logs'
    ssh $key -p $port $d 'cp -R ~/receiver/assets/* ~/receiver/htdocs/assets'
    ssh $key -p $port $d 'rm -R ~/receiver/assets'
    ssh $key -p $port $d 'mv ~/receiver/monitor.html ~/receiver/htdocs'
    ssh $key -p $port $d '~/run.sh stop'
    ssh $key -p $port $d '~/run.sh'
fi

# if [ "$1" == "upload" ] || [ "$1" == "restart" ]; then
#     echo "-- Restart tester on $d.."
#     ssh -t $d 'echo $(cat ~/.credentials) |  sudo -S systemctl restart tester.service'
#     echo "-- Success.."
# fi
done

echo "-- All done.." 
