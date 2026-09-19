#include <webstrada/appserver.h>
#include <webstrada/template_cache.h>
#include <webstrada/config.h>
#include <webstrada/worker.h>
#include <webstrada/reaper.h>
#include <webstrada/cf8.h>

#include <vector>
#include <print>

#include <fcgio.h>

#include <sys/socket.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>


webstrada::appserver::appserver()
{

}

webstrada::appserver::~appserver()
{
    if (m_sock_fd > 0)
    {
        close(m_sock_fd);
    }
}

void webstrada::appserver::init(const char *socket_name, int backlog, int worker_num)
{
    // Load the server configuration (webstrada-config.json) into the config::*
    // globals before forking workers, so every prefork child starts with the
    // same effective settings (see webstrada/config.h). A missing file is
    // bootstrapped from the built-in defaults.
    webstrada::config::initialize();

    m_sock_fd = FCGX_OpenSocket(socket_name, backlog);
    m_worker_num = worker_num;

    // SO_REUSEPORT (https://lwn.net/Articles/542629/) lets separate processes
    // bind the same TCP port, so it only applies to the TCP form of the
    // FastCGI socket (a name starting with ':'). Unix domain socket paths
    // reject it with EOPNOTSUPP, so skip it there to avoid a misleading
    // startup warning. Note the prefork workers inherit this listening fd, so
    // the option is not required by this server's process model.
    if (socket_name && socket_name[0] == ':') {
        int optval = 1;
        int res = setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        if (res) {
            perror("Error calling setsockopt");
        }
    }
}

int webstrada::appserver::run()
{
    TemplateCache templates;

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = [](int sig, siginfo_t *si, void *ucontext) {
        fprintf(stderr, "catch SIGSEGV. si: %d, addr: %p\n", sig, si->si_addr);
        exit(1);
    };
    if(sigaction(SIGSEGV, &sa, NULL) == -1){
        perror("sigaction");
    }

    FCGX_Init();

    if (m_worker_num > 64)
        m_worker_num = 64;

    if (m_worker_num > 1)
    {
        std::vector<int> childs;

        auto fork_reaper = []() -> int {
            int pid = fork();
            if (pid == 0) {
#ifdef __linux__
                prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif
                printf("Starting dedicated reaper process. pid: %d\n", getpid());
                _exit(run_reaper_process());
            } else if (pid < 0) {
                perror("fork reaper");
                return -1;
            }
            return pid;
        };

        int reaper_pid = fork_reaper();

        for(int fork_cnt = 0; fork_cnt < m_worker_num; fork_cnt++)
        {
            int pid = fork();
            if (pid == 0) {
#ifdef __linux__
                prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif

                printf("Starting forked worker. pid: %d\n", getpid());

                return main();
            }
            else if (pid < 0) {
                perror("fork");
                return 1;
            }

            childs.push_back(pid);
        }

        printf("main process wait for signal..\n");

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGINT);
        sigaddset(&sigset, SIGTERM);
        sigaddset(&sigset, SIGCHLD);
        sigprocmask(SIG_BLOCK, &sigset, NULL);

        bool terminate = false;
        while (!terminate) {
            int sig = 0;
            sigwait(&sigset, &sig);
            if (sig == SIGINT || sig == SIGTERM) {
                terminate = true;
                break;
            } else if (sig == SIGCHLD) {
                int status;
                pid_t exited;
                while ((exited = waitpid(-1, &status, WNOHANG)) > 0) {
                    if (exited == reaper_pid && !terminate) {
                        fprintf(stderr, "[SUPERVISOR] Reaper process %d exited with status %d, restarting...\n",
                                reaper_pid, status);
                        reaper_pid = fork_reaper();
                    }
                }
            }
        }

        printf("\n");
        printf("ask children to terminate..\n");

        if (reaper_pid > 0) {
            kill(reaper_pid, SIGTERM);
        }
        for(const int& child : childs)
            kill(child, SIGUSR1);

        printf("wait for children to terminate..\n");

        int status;
        if (reaper_pid > 0) {
            waitpid(reaper_pid, &status, 0);
        }
        for(const int& child : childs)
            waitpid(child, &status, 0);


        printf("all children terminated..\n");
    }
    else
    {
        return main();
    }

    return 0;
}

int webstrada::appserver::main()
{
    // Re-seed rand() after the fork so each worker process mints its own
    // CreateUUID()/CreateGUID() sequence (the parent's seed is inherited).
    cfml::seed_rand();

    FCGX_Request request;

    if (FCGX_InitRequest(&request, m_sock_fd, 0))
    {
        perror("FCGX_InitRequest");
        return 1;
    }

    webstrada::worker worker;

    while(1)
    {

        if (FCGX_Accept_r(&request)) {
            perror("FCGX_Accept_r");
            break;
        }

        worker.process_request(&request); // break;
    }

    return 0;
}
