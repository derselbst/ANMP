#include "ThreadPriority.h"
#include "AtomicWrite.h"
#include <thread>
#include <cstring>

#ifdef _POSIX_C_SOURCE
#include <pthread.h>
#endif

struct ThreadPriority::Impl
{
#ifdef _POSIX_C_SOURCE
    sched_param sch;
    int policy;
#endif
};

ThreadPriority::ThreadPriority(Priority p) : d(std::make_unique<Impl>())
{
#ifdef _POSIX_C_SOURCE
    pthread_getschedparam(pthread_self(), &d->policy, &d->sch);
    
    sched_param newSch = d->sch;
    switch(p)
    {
        case Priority::High:
            newSch.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
            if (pthread_setschedparam(pthread_self(), SCHED_FIFO | SCHED_RESET_ON_FORK, &newSch))
            {
                CLOG(LogLevel_t::Info, "Failed to setschedparam SCHED_FIFO: " << std::strerror(errno));

                newSch.sched_priority = sched_get_priority_max(SCHED_RR) - 1;
                if (pthread_setschedparam(pthread_self(), SCHED_RR | SCHED_RESET_ON_FORK, &newSch))
                {
                    CLOG(LogLevel_t::Warning, "Failed to setschedparam SCHED_RR: " << std::strerror(errno));
                }
            }
            break;
        default:
            break;
    }
#endif
}

ThreadPriority::~ThreadPriority()
{
#ifdef _POSIX_C_SOURCE
    if (pthread_setschedparam(pthread_self(), d->policy, &d->sch))
    {
        CLOG(LogLevel_t::Error, "Failed to restore original Thread Priority: " << std::strerror(errno));
    }
#endif
}
