#!/bin/bash

# This script will be executed after all the other init script

# Wait for a valid IP-address
while ! ifconfig | grep "147" > /dev/null && 
      ! ifconfig | grep "192" > /dev/null;
do
   sleep 1
done

# Start the WebSocket server
sudo /home/pi/webhouse/local/Firouzi/Template
exit 0
