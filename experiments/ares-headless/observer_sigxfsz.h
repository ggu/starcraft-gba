/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_OBSERVER_SIGXFSZ_H
#define SC64_OBSERVER_SIGXFSZ_H
#if !defined(__APPLE__) || !defined(__aarch64__)
#error "SIGXFSZ diagnostic requires Darwin arm64"
#endif
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ucontext.h>
#include <mach/arm/_structs.h>
/* Fixed-size host diagnostic, never a guest PC or a file identity. */
struct Sc64ObserverSignalRecord {
    uint32_t magic, version, signal_number, signal_code;
    uint64_t host_pc, host_lr, host_sp;
};
static int sc64_observer_signal_fd = -1;
static void sc64_observer_signal_handler(int sig, siginfo_t *info, void *context)
{
    struct Sc64ObserverSignalRecord r = {0x53433634u,1u,(uint32_t)sig,
                                         info ? (uint32_t)info->si_code : 0u,0,0,0};
    ucontext_t *uc=(ucontext_t *)context;
    if (uc && uc->uc_mcontext) {
        r.host_pc=(uint64_t)__darwin_arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
        r.host_lr=(uint64_t)__darwin_arm_thread_state64_get_lr(uc->uc_mcontext->__ss);
        r.host_sp=(uint64_t)__darwin_arm_thread_state64_get_sp(uc->uc_mcontext->__ss);
    }
    if (sc64_observer_signal_fd>=0) (void)pwrite(sc64_observer_signal_fd,&r,sizeof r,0);
#ifdef SC64_OBSERVER_STOP_ON_SIGXFSZ
    /* Stop this owned process; never resume guest execution in the supervisor. */
    (void)kill(getpid(),SIGSTOP);
#endif
    _exit(128+sig);
}
static int sc64_observer_signal_install(int fd)
{
    struct sigaction action = {};
    if (fd<0) return 0;
    sc64_observer_signal_fd=fd;
    action.sa_sigaction=sc64_observer_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags=SA_SIGINFO;
    return sigaction(SIGXFSZ,&action,0)==0;
}
#endif
