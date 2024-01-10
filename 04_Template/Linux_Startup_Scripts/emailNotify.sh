#!/bin/bash

# This script will be executed after all the other init script

# Wait for a valid IP-address during 1 minutes
count=0
while  ! ifconfig | grep -F "10.19."   > /dev/null  &&      # wlan0 BFH bfh-open
       ! ifconfig | grep -F "147.87."  > /dev/null  &&      # eth0  BFH Quellgasse 21
       ! ifconfig | grep -F "172.10."  > /dev/null  &&      # wlan0 SIP sip-guest
       ! ifconfig | grep -F "172.31."  > /dev/null  &&      # eth0  SIP class room
       ! ifconfig | grep -F "192.168." > /dev/null ;        # wlan0 home Swisscom
do
   count=$((count+1))
   if [ $count -eq 60 ]; then
      break
   fi
   sleep 1
done

# Reduce Subnet mask for the interent connections
# ifconfig eth0  '/sbin/ifconfig eth0  | grep "inet addr" | tr -s " " | cut -f 3 -d " " | cut -f 2 -d ":"' netmask 255.255.0.0 up
# ifconfig wlan0 '/sbin/ifconfig wlan0 | grep "inet addr" | tr -s " " | cut -f 3 -d " " | cut -f 2 -d ":"' netmask 255.255.0.0 up

# Define the email message
HOSTNAME=`hostname -f`
EIP=$(hostname -I)
LIP=$(hostname -i)

printf "Define email to send\n"
echo "$HOSTNAME online" >> /root/email.txt
echo >> /root/email.txt
echo "Ethernet IP address: $EIP" >> /root/email.txt
echo "Local    IP address: $LIP" >> /root/email.txt
echo >> /root/email.txt
date >> /root/email.txt
echo >> /root/email.txt

# Send the email message with mailutils
# mail -s "$HOSTNAME online"  fue1@bfh.ch < /root/email.txt

# Send the email with python
printf "Send the email with python\n"
sudo python /root/mailing.py

# Remore the email message
cat /root/email.txt
rm -rf /root/email.txt

# Store the IP-address on a USB-Stick for security

# Wait for a USB-stick during 5 minute
printf "Wait until the USB-stick is ready\n"
count=0
# until  [ -d "/media/." ]
until [ ! "$(ls -A "/media")" = "" ]
do
   count=$((count+1))
   if [ $count -eq 60 ]; then
      break
   fi
   printf "Sleep\n"
   sleep 1
done
echo "USB stick is ready"


printf "Create and save the file containg the IP-Addresses\n"
for path in $dir/media/pi/*; do
    filePath="$path"
    printf "$filePath\n"

    # Enter into the USB-stick directory
    cd $filePath

    # If ip_address.txt exists then delete it
    if  [ -f ip_address.txt ]; then
       rm -rf ip_address.txt
    fi

    # Generate the new ip_address.txt
    echo "$HOSTNAME online" >> ip_address.txt
    echo >> ip_address.txt
    echo "Ethernet IP address: $EIP" >> ip_address.txt
    echo "Local    IP address: $LIP" >> ip_address.txt
    echo >> ip_address.txt
    date >> ip_address.txt
    echo >> ip_address.txt

done

exit 0
