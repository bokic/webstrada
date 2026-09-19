#include "webstrada/reaper.h"
#include "webstrada/cf8.h"
#include "webstrada/component.h"
#include "webstrada/config.h"

#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <cerrno>
#include <ctime>

#ifdef __linux__
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#endif

namespace webstrada {

static ComponentInfo *reaper_component_loader(const char *path, void *opaque)
{
    auto *cache = static_cast<TemplateCache*>(opaque);
    if (!cache) return nullptr;
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return nullptr;
    }
    return cache->get_component(string(path));
}

size_t reap_expired_scopes(ScopeStore &store, int64_t now, TemplateCache *templates)
{
    std::vector<ScopeStore::ExpiredScopeRecord> expired;
    if (!store.claimExpired(now, expired) || expired.empty()) {
        return 0;
    }

    // Index claimed applications so expiring sessions whose application also expired
    // can resolve their application scope data directly from memory.
    std::unordered_map<std::string, const ScopeStore::ExpiredScopeRecord*> claimedApps;
    for (const auto &rec : expired) {
        if (rec.scopeKind == "APPLICATION") {
            claimedApps[rec.appName] = &rec;
        }
    }

    cfml::VariantCleanupGuard guard;
    cfml::IncludeRuntime includeRuntime;
    if (templates) {
        includeRuntime.loaderOpaque = templates;
        includeRuntime.componentLoader = &reaper_component_loader;
        includeRuntime.componentLoaderOpaque = templates;
        cfml::include_begin(&includeRuntime);
    }

    // 1. Process expired sessions first (before onApplicationEnd).
    for (const auto &rec : expired) {
        if (rec.scopeKind != "SESSION") continue;

        cfvariant sessionScope(cfvariant::Struct);
        if (!rec.data.empty()) {
            cfml::scope_json_deserialize(rec.data, sessionScope);
        }

        cfvariant applicationScope(cfvariant::Struct);
        auto it = claimedApps.find(rec.appName);
        if (it != claimedApps.end() && !it->second->data.empty()) {
            cfml::scope_json_deserialize(it->second->data, applicationScope);
        } else {
            std::string appJson;
            if (store.loadApplication(rec.appName, now, appJson)) {
                cfml::scope_json_deserialize(appJson, applicationScope);
            }
        }

        if (templates && !rec.cfcPath.empty()) {
            ComponentInfo *info = reaper_component_loader(rec.cfcPath.c_str(), templates);
            if (info) {
                cfml::cf_invoke_on_session_end_with_info(info, &sessionScope, &applicationScope);
                component_info_release(info);
            }
        }
    }

    // 2. Process expired applications.
    for (const auto &rec : expired) {
        if (rec.scopeKind != "APPLICATION") continue;

        cfvariant applicationScope(cfvariant::Struct);
        if (!rec.data.empty()) {
            cfml::scope_json_deserialize(rec.data, applicationScope);
        }

        if (templates && !rec.cfcPath.empty()) {
            ComponentInfo *info = reaper_component_loader(rec.cfcPath.c_str(), templates);
            if (info) {
                cfml::cf_invoke_on_application_end_with_info(info, &applicationScope);
                component_info_release(info);
            }
        }
    }

    if (templates) {
        cfml::include_end();
    }

    return expired.size();
}

std::string resolve_scope_db_path()
{
    std::string dbPath = webstrada::config::scopeDbPath;
    if (dbPath.empty()) {
        char exe[4096];
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n > 0) {
            exe[n] = '\0';
            std::string path(exe);
            size_t slash = path.find_last_of('/');
            dbPath = (slash != std::string::npos)
                ? path.substr(0, slash + 1) + "WebStrada-scopes.sqlite"
                : "WebStrada-scopes.sqlite";
        } else {
            dbPath = "WebStrada-scopes.sqlite";
        }
    }
    return dbPath;
}

int run_reaper_process(const std::string &scopeDbPath)
{
    cfml::seed_rand();

    std::string dbPath = scopeDbPath.empty() ? resolve_scope_db_path() : scopeDbPath;
    ScopeStore store;
    if (!store.open(dbPath)) {
        fprintf(stderr, "[REAPER] Failed to open scope store at '%s': %s\n",
                dbPath.c_str(), store.lastError().c_str());
        return 1;
    }

    TemplateCache templates;

    // Block SIGINT and SIGTERM so they are received via signalfd
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

#ifdef __linux__
    int sigFd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    int timerFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);

    struct pollfd pfd[2];
    pfd[0].fd = sigFd;
    pfd[0].events = POLLIN;
    pfd[1].fd = timerFd;
    pfd[1].events = POLLIN;

    bool running = true;
    while (running) {
        int64_t now = static_cast<int64_t>(std::time(nullptr));
        int64_t earliest = 0;
        bool hasExpiry = store.earliestExpiry(earliest);

        struct itimerspec its{};
        if (hasExpiry) {
            if (earliest <= now) {
                reap_expired_scopes(store, now, &templates);
                continue;
            }
            its.it_value.tv_sec = earliest;
            its.it_value.tv_nsec = 0;
        } else {
            // Heartbeat: check every 60 seconds
            its.it_value.tv_sec = now + 60;
            its.it_value.tv_nsec = 0;
        }
        timerfd_settime(timerFd, TFD_TIMER_ABSTIME, &its, nullptr);

        int prc = poll(pfd, 2, -1);
        if (prc < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (pfd[0].revents & POLLIN) {
            // Signal received!
            struct signalfd_siginfo fdsi{};
            ssize_t s = read(sigFd, &fdsi, sizeof(fdsi));
            (void)s;
            running = false;
            break;
        }

        if (pfd[1].revents & POLLIN) {
            uint64_t exp = 0;
            ssize_t s = read(timerFd, &exp, sizeof(exp));
            (void)s;
            now = static_cast<int64_t>(std::time(nullptr));
            reap_expired_scopes(store, now, &templates);
        }
    }

    if (timerFd >= 0) close(timerFd);
    if (sigFd >= 0) close(sigFd);
#else
    bool running = true;
    while (running) {
        int64_t now = static_cast<int64_t>(std::time(nullptr));
        int64_t earliest = 0;
        bool hasExpiry = store.earliestExpiry(earliest);
        int sleepSec = 60;
        if (hasExpiry) {
            if (earliest <= now) {
                reap_expired_scopes(store, now, &templates);
                continue;
            }
            sleepSec = static_cast<int>(earliest - now);
            if (sleepSec > 60) sleepSec = 60;
        }
        sleep(sleepSec);
        now = static_cast<int64_t>(std::time(nullptr));
        reap_expired_scopes(store, now, &templates);
    }
#endif

    // Graceful shutdown: reap remaining expired scopes and close
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    reap_expired_scopes(store, now, &templates);
    templates.clear();
    store.close();
    return 0;
}

} // namespace webstrada
