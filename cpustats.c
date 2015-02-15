/* processor stats from proc filesystem
 *
 * Copyright 2015 Vincent Sanders <vince@kyllikki.org>
 *
 * Released under the MIT licence
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define STAT_BUF_SIZE 256 /* plenty for the first line even if every jiffy value is 20 digits long */

struct cpustat {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long total;
};


int get_cpustats(int statfd, char *statbuf, struct cpustat* cstat)
{
    char* b;

    lseek(statfd, 0L, SEEK_SET);
    read(statfd, statbuf, STAT_BUF_SIZE - 1);
    statbuf[STAT_BUF_SIZE - 1] = 0;

    b = strstr(statbuf, "cpu ");
    if (b == NULL) {
        return -1;
    }

    if (sscanf(b, "cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", 
               &cstat->user,
               &cstat->nice,
               &cstat->system,
               &cstat->idle,
               &cstat->iowait,
               &cstat->irq,
               &cstat->softirq,
               &cstat->steal) != 8) {
        return -1;
    }

#if DEBUG_PROC_READ
    printf("cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu\n",
           cstat->user,
           cstat->nice,
           cstat->system,
           cstat->idle,
           cstat->iowait,
           cstat->irq,
           cstat->softirq,
           cstat->steal);
#endif
    return 0;
}

/* subtracts stats in b from a and returns in c */
int sub_cpustats(struct cpustat *a, struct cpustat *b, struct cpustat *c)
{
    c->user = a->user - b->user;
    c->nice = a->nice - b->nice;
    c->system = a->system - b->system;
    c->idle = a->idle - b->idle;
    c->iowait = a->iowait - b->iowait;
    c->irq = a->irq - b->irq;
    c->softirq = a->softirq - b->softirq;
    c->steal = a->steal - b->steal;
    c->total = c->user + c->nice + c->system + c->idle + c->iowait + c->irq + c->softirq + c->steal;

#if DEBUG_SUBTRACTION
    printf("cpu  %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu = %Lu\n",
           c->user,
           c->nice,
           c->system,
           c->idle,
           c->iowait,
           c->irq,
           c->softirq,
           c->steal,
           c->total);
#endif

    return 0;
}

int relative_cpustats(struct cpustat *c)
{
    unsigned long long halftot;

    halftot = c->total/2;
    c->user = ((100 * c->user) + halftot) / c->total;
    c->nice = ((100 * c->nice) + halftot) / c->total;
    c->system = ((100 * c->system) + halftot) / c->total;
    c->idle = ((100 * c->idle) + halftot) / c->total;
    c->iowait = ((100 * c->iowait) + halftot) / c->total;
    c->irq = ((100 * c->irq) + halftot) / c->total;
    c->softirq = ((100 * c->softirq) + halftot) / c->total;
    c->steal = ((100 * c->steal) + halftot) / c->total;

    c->total = c->user + c->nice + c->system + c->idle + c->iowait + c->irq + c->softirq + c->steal;

    return 0;
}

int main(int argc, char **argv, char **environ)
{
    int statfd;
    char *statbuf;
    struct cpustat laststat;
    struct cpustat curstat;
    struct cpustat secstat;

    statbuf = malloc(STAT_BUF_SIZE);

    statfd = open("/proc/stat", O_RDONLY, 0);
    if (statfd == -1) {
        fprintf(stderr, "Unable to read from /proc/stat");
        return 1;
    }

    memset(&laststat, 0, sizeof(laststat));

    printf("us ni sy id io iq sq st\n");

    while (get_cpustats(statfd, statbuf, &curstat) >= 0) {
        sub_cpustats(&curstat, &laststat, &secstat);
        laststat = curstat;
        relative_cpustats(&secstat);
        printf("%2Lu %2Lu %2Lu %2Lu %2Lu %2Lu %2Lu %2Lu\n",
               secstat.user,
               secstat.nice,
               secstat.system,
               secstat.idle,
               secstat.iowait,
               secstat.irq,
               secstat.softirq,
               secstat.steal);
        sleep(1);
    }

    close(statfd);

    return 0;
}
