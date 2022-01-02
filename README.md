# firehop-linux
The server daemon that can help with hopping over firewalls, on Linux. Looking for the Windows version? It should be one of my GitHub repositories already, at this time. 

Firehop is a program which is similar to services such as *ngrok* or *ssh*, in that it can reverse tunnel any connection that you give it. Firehop, however, is not proprietary and allows for the self-hosting of the reverse connection onto any computer/server which serves the Firehop service. Firehop is broken down into two main programs: *firehop* and *server2server*. 

*firehop* is the program which you run on the machine you want to reverse the connection onto; *server2server* shall be run on the machine that has the connection you want to reverse. *server2server* needs to connect to an open port which the *firehop* service shall be listening and accepting requests on, which then will allow for the seamless reversing of connections. 

As of now, both programs are relatively limited in what they can accomplish, for:

 - They do not support reversing UDP connections at the moment.
 - They do not support a secure tunnel (there is no encryption). 
 - They are written within C, lending to the possibility of memory exploits threatening the security of those who run the programs

However, these programs do not mean to compete with *ngrok* or *ssh*, because they are clearly inept for doing so. Instead, they strive to be used because of the following traits:

 - They are extremely, extremely tiny by comparison. This also means that it is trivial to find bugs and the aforementioned memory exploits above (even though I have been particularly careful with buffer overflows and the like). 
 - They can also be statically compiled as native binaries for the operating system of choice.
 - As a result, they can be easily forked and modified to the heart's content of whoever wants to modify them.
 - They are written in C without any large external dependencies.
 - They can be easily ported.
 - They require absolutely no configuration files; instead, addresses and ports are handled through command-line arguments. 

To build firehop, run the *make.sh* script that is included within this repository, which you should check before you run. Eventually, I will make a makefile for this program; however, in all honesty, this program is so tiny that it barely requires one, as the *make.sh* file will reveal. To those building on a UNIX system other than Linux: this program requires *pthread*, so if it is not supported on your system, too bad--well, for now, since it is only a matter of time until I take the asynchronous I/O approach with select(), which this program is already fundamentally based upon. 

With that out of the way, let me quote the help menu from the *firehop* program:

   

     firehop (for Linux) - Version: 1.0b - hop over firewalls
    USAGE
    firehop [args...]
    
    --local, -l    [Required] Specifies the port to locally host on.
    --remote, -r   [Required] Specifies the port that should be port forwarded.
    --control, -c  [Required] Specifies the port that should also be port forwarded.
    --tcp, -t      [Default] Explicitly specifies that we should be in TCP mode.
    --udp, -u      Explicitly specifies that we should operate in UDP mode.
    --quiet, -q    Output absolutely nothing to the console.
    --help, -h     Display this menu, then exit successfully.

 
 The control port and the remote port must be accessible to whoever shall be running the *server2server* program. This usually means that they should be accessible to the open internet to the computer who wishes to reverse their connection to you. If you want to make this program effectively a proxy in all regards, then you may allow the local port to be accessible to the open internet as well: you will end up shielding the computer you are reversing.

Unlike my other projects, this project should be one of actual use. If you find a problem, a bug, a feature that should be added, or an exploit, please notify me immediately. 

