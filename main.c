#include <stdio.h>

#include "firehop.h"

#define arg_complain(name, next) if (next == NULL)                                      \
                    {                                                                   \
                        fprintf(stderr, "Error: " name " requires an argument.\n");     \
                        return -1;                                                      \
                    }                                                                   \

void help()
{
    const char *msg =
    "firehop (for Linux) - Version: " VERSION " - hop over firewalls\n"
    "USAGE\n"
    "firehop [args...]\n\n"
    "--local, -l    [Required] Specifies the port to locally host on.\n"
    "--remote, -r   [Required] Specifies the port that should be port forwarded.\n"
    "--control, -c  [Required] Specifies the port that should also be port forwarded.\n"
    "--tcp, -t      [Default] Explicitly specifies that we should be in TCP mode.\n"
    "--udp, -u      Explicitly specifies that we should operate in UDP mode.\n"
    "--quiet, -q    Output absolutely nothing to the console.\n"
    "--help, -h     Display this menu, then exit successfully.\n";

    printf("%s\n", msg);
}

int main(int argc, char **argv, char **envp)
{
    enum Modes mode = UndefinedMode;

    int rport = 0;
    int cport = 0;
    int lport = 0;

    for (size_t i = 1; i < argc; i++)
    {
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
                FILE *fp = fopen("/dev/null", "wb");
                dup2(fileno(fp), STDOUT_FILENO);
                dup2(fileno(fp), STDERR_FILENO);
            }
            else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
            {
                help();
                return 0; 
            }
            else
            {
                fprintf(stderr, "Invalid argument passed to program. Check help menu?\n");
                help();
                
                return -1;
            }
            
        }
    }

    if (!lport || !rport || !cport)
    {
        fprintf(stderr, "Error: a required port has not been defined. Please pass it via arguments.\n");
        return -2;
    }

    if (mode == UndefinedMode)
    {
        fprintf(stderr, "Warning: program mode not specified, assuming TCP mode as default.\n");
        mode = TCPMode;
    }

    Firehop *fh = firehop(cport, rport, lport);

    if (fh == NULL)
    {
        fprintf(stderr, "Failed to initialize firehop server.\n");
        return -4;
    }

    firehop_start(fh);

    return 0;
}