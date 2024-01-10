#!/usr/bin/python3
import subprocess, sys, smtplib, nslookup

from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

#############################################
# to change: recipient & max_successful_email 
#############################################
recipient  = "fue1@bfh.ch"
max_successful_email = 3

successful_email = 0

# list of the 21 most popular mail server (https://domar.com/pages/smtp_pop3_server)
array_domain = []
array_domain.append('gmail.com')
array_domain.append('live.com')
array_domain.append('office365.com')
array_domain.append('yahoo.com')
array_domain.append('o2.ie')
array_domain.append('ntlworld.com')
array_domain.append('btconnect.com')
array_domain.append('btopenworld.com')
array_domain.append('btinternet.com')
array_domain.append('orange.net')
array_domain.append('wanadoo.co.uk')
array_domain.append('o2online.de')
array_domain.append('t-online.de')
array_domain.append('1and1.com')
array_domain.append('1und1.de')
array_domain.append('comcast.net')
array_domain.append('verizon.net')
array_domain.append('zoho.com')
array_domain.append('mail.com')
array_domain.append('gmx.com')

for domain in array_domain:
    originator = "webhouse@" + domain

    print("\r\nFrom: "    + originator)
    print("To:"       + recipient)
    print("Domain: "  + domain)
    
    mx_records = []
    mx_values = {'pref' : 0, 'serv' : ''}

    # search every mail exchange server for a given domain
    p = subprocess.Popen('nslookup -type=mx ' + domain + ' 8.8.8.8', 
       shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    for line in p.stdout.readlines():
        line = line.decode().lower()  
        if line.find("mail exchange") !=-1 :
            for char in line:
                if str(char) in "\r\n\t":
                    line = line.replace(char, '')
            if line.find("mx preference") !=-1 :
                mx_parse = line.replace(' ', '').split(",")
                mx_values['pref'] = int(mx_parse[0].split("=")[1])
                mx_values['serv'] = mx_parse[1].split("=")[1]
            else:
                mx_parse = line.split(" = ")[1].split(" ")
                mx_values['pref'] = int(mx_parse[0])
                mx_values['serv'] = mx_parse[1]
            mx_records.append(mx_values.copy())
            print("\nline = " + line)

    retval = p.wait()
    
    if len(mx_records) == 0 :
        continue

    # sort the mail exchange server upon their priority
    print("\r\nSearch first priority mail exchange server for \"" + 
       domain + "\"") 
    
    def mx_pref_sortvalue(record):
        return record['pref']
    mx_records=sorted(mx_records, key=mx_pref_sortvalue)
    
    # take the mail exchange server with the higgest priority
    server = mx_records[0]['serv']
    
    # generate the mail message to send
    msg = MIMEMultipart()
    msg['From'] = originator
    msg['To'] = recipient
    msg['Subject'] = "IP-Address Webhouse"

    body = open('/root/email.txt', 'r').read()
    msg.attach(MIMEText(body, 'plain'))
    
    # send the email with the mail eschange server
    print("\r\nSending mail to: \"" + recipient + "\" via server: \"" + 
       server + "\"")
    
    try:
        smtp_send = smtplib.SMTP(server, 25)
        try:
            smtp_send.sendmail(originator, recipient, msg.as_string())
            successful_email += 1
        except:
            print("\r\nSending mail to: \"" + recipient + 
               "\" via server: \"" + server + "\" has failled!")
        finally:
            smtp_send.quit()
    
    except:
        print("\r\nConnection to the server \"" + server + 
           "\" has failed")    
        
    if successful_email == max_successful_email:
        sys.exit(0);

