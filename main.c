#include <stdio.h>
#include "firehop.h"

/* Complain if argv[i + 1] is NULL, meaning that there isn't an argument after argv[i]. */
#define arg_complain(name, next) if (next == NULL)                                      \
                    {                                                                   \
                        fprintf(stderr, "Error: " name " requires an argument.\n");     \
                        return -1;                                                      \
                    }                                                                   \

/* Resolve a hostname and add it to an in_addr_t array that is on the stack in main(), incrementing
the index of it. */
in_addr_t resolve_whitelist(char *name, in_addr_t *list, size_t *length)
{
    if (*length >= MAX_WLISTIPS)
    {
        fprintf(stderr, "Whitelisted IPs have a maximum of %d in this build! Error!\n", 
                    MAX_WLISTIPS);
        
        return -1;
    }    

    struct hostent *resolution = gethostbyname(name);

    if (!resolution)
    {
        fprintf(stderr, "Error adding %s to the whitelist: %s\n", hstrerror(h_errno));
        return -1;
    }

    list[*length] = (*((struct in_addr*) resolution->h_addr_list[0])).s_addr;
    *length++;
    return 0;
}

void help()
{
    const char *msg =
    "firehop (for " SYSTEM ") - Version: " VERSION " - hop over firewalls\n"
    "USAGE\n"
    "firehop [args...]\n\n"
    "[Note]: Ports must be within the range of 0-65535 (theoretically). 0 - 1024 require root.\n"
    "--local, -l    [Required] Specifies the port to locally host on.\n"
    "--remote, -r   [Required] Specifies the port that should be port forwarded.\n"
    "--control, -c  [Required] Specifies the port that should also be port forwarded.\n"
    "--tcp, -t      [Default] Explicitly specifies that we should be in TCP mode.\n"
    "--udp, -u      Explicitly specifies that we should operate in UDP mode.\n"
    "--max, -m      Set the number of max connections. By default, the max is theoretically infinite.\n"
    "--wlistf, -wf  Only allow IPs from the file provided by --wlistf.\n"
    "--wlist, -w    In addition to above, only allow IPs from those provided by --wlist.\n"
    "[Note]: --wlistf and --wlist both, whenever used, activate whitelist mode.\n"
    "--quiet, -q    Output absolutely nothing to the console.\n"
    "--ipv6, -6     IPv6 mode? Currently not implemented!\n"
    "--help, -h     Display this menu, then exit successfully.\n";

    printf("%s\n", msg);
}

int main(int argc, char **argv, char **envp)
{
    /* Whitelisted, allowlisted, etc....  IP array */
    /* I am a minority, and I don't feel affected by these words... People are just reading */
    /* extensively deep into them! Even the word 'minority' can be construed as having connotation */
    /* as to belittle people of color in the sense that they are minor or less--but only if you */
    /* read into it that way! */ 

    in_addr_t wlistips[MAX_WLISTIPS] = {0};
    size_t wlistips_len = 0;

    enum Modes mode = UndefinedMode;

    int max = 0;
    char *wlist = NULL;

    int rport = 0;
    int cport = 0;
    int lport = 0;

    /* Go through each argument, skipping the first one, which is the program name. */
    for (size_t i = 1; i < argc; i++)
    {
        /* Only care if the first character is - */
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--local") || !strcmp(argv[i], "-l"))
            {
                arg_complain("--local/-l", argv[i + 1]);
                lport = atoi(argv[i + 1]);
            }

            else if (!strcmp(argv[i], "--remote") || !strcmp(argv[i], "-r"))
            {
                arg_complain("--remote/-r", argv[i + 1]);
                rport = atoi(argv[i + 1]);
            }

            else if (!strcmp(argv[i], "--control") || !strcmp(argv[i], "-c"))
            {
                arg_complain("--control/-c", argv[i + 1]);
                cport = atoi(argv[i + 1]);
            }

            else if (!strcmp(argv[i], "--tcp") || !strcmp(argv[i], "-t"))
            {
                mode = TCPMode;
            }

            else if (!strcmp(argv[i], "--udp") || !strcmp(argv[i], "-u"))
            {
                mode = UDPMode;
            }

            else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-q"))
            {
                /* stdout and stderr will now be /dev/null--a bitbucket. */
                FILE *fp = fopen("/dev/null", "wb");
                dup2(fileno(fp), STDOUT_FILENO);
                dup2(fileno(fp), STDERR_FILENO);
            }
            else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
            {
                help();
                return 0; 
            }
            else if (!strcmp(argv[i], "--wlist") || !strcmp(argv[i], "-w"))
            {
                arg_complain("--wlist/-w", argv[i + 1]);
                if (resolve_whitelist(argv[i + 1], wlistips, &wlistips_len) == -1)
                    return -9; 
            }
            else if (!strcmp(argv[i], "--wlistf") || !strcmp(argv[i], "-wf"))
            {
                arg_complain("--wlistf/-wf", argv[i + 1]);
                FILE *fp = fopen(argv[i + 1], "r");

                if (!fp)
                {
                    fprintf(stderr, "Error opening whitelist file '%s': %s\n", 
                                    argv[i + 1], strerror(errno));
                    
                    return -10;
                }

                char ip[64];

                /* fgets() for compatibility */
                while (fgets(ip, sizeof(ip), fp) != NULL)
                {
                    /* Get rid of fgets' newline. */
                    ip[strlen(ip) - 1] = '\0';

                    /* Invalid IP/domain; therefore, we shall exit with an error. */
                    if (resolve_whitelist(ip, wlistips, &wlistips_len) == -1)
                        return -11;
                }

                fclose(fp);
            }
            else if (!strcmp(argv[i], "--max") || !strcmp(argv[i], "-m"))
            {
                arg_complain("--max/-m", argv[i + 1]);
                max = atoi(argv[i + 1]);

                if (!max)
                {
                    fprintf(stderr, "Error: why did you not provide a valid number to --max/-m?\n");
                    return -12;
                }
            }
            else
            {
                fprintf(stderr, "Invalid argument passed to program. Check help menu?\n");
                help();
                
                return -1;
            }
            
        }
    }

    /* 0 means that the port didn't changed; additionally, port of zero isn't possible anyways. */
    if (!lport || !rport || !cport)
    {
        fprintf(stderr, "Error: a required port has not been defined. Please pass it via arguments.\n");
        fprintf(stderr, "Alternatively, non-numeric characters were entered for a port number.\n");
        return -2;
    }

    /* Why have this warning? I want to keep it, in spite of its redundance. */
    if (mode == UndefinedMode)
    {
        fprintf(stderr, "Warning: program mode not specified, assuming TCP mode as default.\n");
        mode = TCPMode;
    }

    Firehop *fh = firehop(cport, rport, lport, mode);

    if (fh == NULL)
    {
        fprintf(stderr, "Failed to initialize firehop server. Exiting...\n");
        return -4;
    }

    fh->max = max;

    /* If there are any entries in wlistips (that is, they aren't zero), enable whitelist mode. */
    fh->whitelist_mode = (wlistips[0] != 0);

    /* Pass the whitelist information over to the Firehop instance. */
    fh->wlist_ips = wlistips;
    fh->wlist_ips_len = wlistips_len;

    /* Start, then when Firehop exits, free Firehop. */
    firehop_start(fh);
    firehop_free(fh);

    return 0;
}