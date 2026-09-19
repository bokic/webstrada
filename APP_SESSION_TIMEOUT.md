# Application and Session Timeout Architecture (`APP_SESSION_TIMEOUT.md`)

This document outlines the architecture, behavior, and implementation specifications for handling application and session lifecycles, specifically timeout tracking, background reaper mechanisms, SQLite-backed state synchronization, and explicit invalidation in WebStrada according to Adobe ColdFusion (ACF) standards.

---

## 1. Overview and Requirements

ColdFusion's `Application.cfc` lifecycle defines two end-of-life callback handlers:
* `onSessionEnd(SessionScope, ApplicationScope)`: Invoked when a session expires due to inactivity or is explicitly invalidated.
* `onApplicationEnd(ApplicationScope)`: Invoked when an application shuts down due to inactivity timeout or server termination.

Because timeouts occur due to inactivity when **no HTTP request is reaching the server**, timeouts cannot rely on incoming request triggers alone. A dedicated background reaper mechanism is required.

---

## 2. Multi-Process Concurrency & Threading Architecture

### 2.1 Dedicated Forked Reaper Process (Process & Fault Isolation)
* Across the entire server instance (including multi-process worker deployments), lifecycle timeouts are handled by **exactly one dedicated forked Reaper child process** managed by the master supervisor daemon.
* **Fault & Crash Isolation:**
  * Because `onSessionEnd` and `onApplicationEnd` execute arbitrary user CFML/CFC code, any unhandled memory fault (e.g. `SIGSEGV`, stack overflow, or assertion failure) only terminates the child Reaper process.
  * The supervisor and HTTP request workers are completely isolated and never crash due to reaper callback failures.
* **Supervisor Auto-Restart:**
  * The master supervisor monitors the Reaper PID alongside HTTP worker PIDs via `waitpid()`.
  * If the Reaper process terminates unexpectedly, the supervisor logs the exit code/signal to `exception.log` and immediately forks a fresh Reaper process.
  * Because all state and expiration deadlines persist in SQLite (`expires_at`), the newly forked Reaper immediately queries `SELECT MIN(expires_at)` and seamlessly resumes without any state loss.
* Worker processes do not run reaper loops; workers handle incoming requests and interact with the database using lock-free conditional statements.

### 2.2 SQLite Storage & WAL Mode
* Sessions and applications are tracked in a shared SQLite database.
* SQLite is configured with **WAL (Write-Ahead Logging)** mode:
  ```sql
  PRAGMA journal_mode = WAL;
  PRAGMA busy_timeout = 3000;
  ```
  This allows concurrent readers and writers across worker processes without mutual lockout.

---

## 3. Database Schema

Tables use an absolute expiration timestamp (`expires_at = now() + timeout_duration`) instead of storing relative intervals or maintaining a mutable `status` flag. When an entity expires or is invalidated, its record is deleted.

```sql
CREATE TABLE sessions (
    session_id      TEXT PRIMARY KEY,
    app_name        TEXT NOT NULL,
    expires_at      INTEGER NOT NULL,   -- Absolute epoch/monotonic timestamp
    data            BLOB                -- Serialized SessionScope state
);

CREATE INDEX idx_sessions_expires_at ON sessions(expires_at);

CREATE TABLE applications (
    app_name        TEXT PRIMARY KEY,
    expires_at      INTEGER NOT NULL,   -- Absolute epoch/monotonic timestamp
    data            BLOB                -- Serialized ApplicationScope state
);

CREATE INDEX idx_apps_expires_at ON applications(expires_at);
```

---

## 4. Worker Request Handling (Lock-Free Updates)

Worker processes do not acquire manual table or database locks at request start. Instead, workers perform atomic conditional `UPDATE` statements and inspect the affected row count (`sqlite3_changes()`):

### 4.1 Session Validation & Touch at Request Start
```sql
UPDATE sessions
SET expires_at = :now + :session_timeout_seconds
WHERE session_id = :session_id
  AND expires_at > :now;
```

* **If `sqlite3_changes() == 1`:**
  The session exists and has not expired. Its lifespan has been extended atomically. The worker loads the session data and proceeds with request execution.
* **If `sqlite3_changes() == 0`:**
  The session has already expired, been deleted by the reaper, or does not exist.
  * The worker treats the session token as invalid.
  * A new session ID is generated.
  * `onSessionStart()` is executed for the new session, and a new session cookie is sent to the client.
  * A new record is inserted: `INSERT INTO sessions (session_id, app_name, expires_at, data) VALUES (...)`.

### 4.2 Application Validation at Request Start
```sql
UPDATE applications
SET expires_at = :now + :app_timeout_seconds
WHERE app_name = :app_name
  AND expires_at > :now;
```
* If `sqlite3_changes() == 0`, the application has expired (or never started). The worker initializes the application and invokes `onApplicationStart()`.

---

## 5. Background Reaper Architecture (Dedicated Forked Process)

The dedicated Reaper child process coordinates timeouts without busy polling and without holding database locks while CFML code runs.

### 5.1 Precalculated Sleep Loop & Immediate Wakeup on Signals
In Linux, the Reaper process coordinates sleep and abort using standard file descriptors (e.g. `timerfd` and `signalfd` via `epoll_wait` / `poll`, or `pthread_cond_timedwait` on a stop-flag):

1. **Abort / Signal Wakeup (Ctrl+C / SIGINT / SIGTERM):**
   * When server shutdown or abort is signaled (e.g. supervisor sends `SIGTERM` / `SIGINT`), the signal is caught immediately via `signalfd` (or signal handler self-pipe).
   * The Reaper process instantly unblocks from its `poll()` / `epoll_wait()` without waiting for the timer to expire.
   * The Reaper initiates the clean shutdown sequence: executing `onApplicationEnd(ApplicationScope)` for all active applications, flushing caches, and exiting cleanly with code 0.

2. **Standard Timeout Sleep Loop:**
   * Query the earliest expiration across the system:
     ```sql
     SELECT MIN(expires_at) FROM sessions;
     ```
     *(With the `idx_sessions_expires_at` index, this is an $O(1)$ single-leaf index read).*
   * If the table is empty:
     Arm the Reaper process to sleep indefinitely (or for a default heartbeat period), waking only on `SIGTERM`/`SIGINT` or worker IPC trigger.
   * If `earliest_expires_at > now`:
     Arm `timerfd` for `earliest_expires_at - now`.
     Block on `poll()` / `epoll_wait()` monitoring both the `timerfd` and the signal descriptor.
   * If woken by signal (`SIGTERM`/`SIGINT`):
     Proceed immediately to graceful shutdown.
   * If woken by `timerfd` expiry (`earliest_expires_at <= now`):
     Proceed to extract expired records and fire callbacks.

### 5.2 Expired Record Extraction (Copy Data Before Deleting)
When invoking `onSessionEnd(SessionScope, ApplicationScope)`, ColdFusion requires the expiring session's scope data as well as the application's scope.

**Critical Rule:** The SQLite transaction must **never** be held while executing CFML code (`onSessionEnd` or `onApplicationEnd`), as CFML callbacks may run slow queries, HTTP requests, or external calls.

#### Execution Sequence:
1. **Atomically claim and delete expired records while extracting their data:**
   ```sql
   DELETE FROM sessions
   WHERE expires_at <= :now
   RETURNING session_id, app_name, data;
   ```
   *(Alternatively in an explicit transaction: `BEGIN IMMEDIATE; SELECT ...; DELETE ...; COMMIT;`)*
2. All expired records and their serialized scope `data` are copied into local memory (`std::vector<ExpiredSessionJob>`).
3. **Database lock is released immediately.**
4. For each expired session job:
   * Deserialize `data` into a `cfvariant` struct representing `SessionScope`.
   * Retrieve the corresponding `ApplicationScope` (from in-memory cache or quick DB read).
   * Invoke `onSessionEnd(SessionScope, ApplicationScope)` in the CFML runtime.
5. Apply the same sequence to `applications` table when `expires_at <= :now`, copying application `data` into memory before deleting, and invoking `onApplicationEnd(ApplicationScope)`.
6. Recalculate next `MIN(expires_at)` and resume sleep.

---

## 6. ColdFusion Execution Context & Quirks

When executing `onSessionEnd` and `onApplicationEnd`:

### 6.1 Argument Passing & Scopes
* **`onSessionEnd(SessionScope, ApplicationScope)`:**
  * The session scope and application scope are passed strictly as explicit arguments.
  * Inside `onSessionEnd`, the built-in `session` and `application` scopes are **not** implicitly bound in the global scope chain; user code must use `arguments.SessionScope` and `arguments.ApplicationScope`.
  * Request scopes (`CGI`, `URL`, `FORM`, `Request`) are empty or unbound because execution occurs outside of an active HTTP connection.
* **`onApplicationEnd(ApplicationScope)`:**
  * Passes the application scope as its sole argument (`arguments.ApplicationScope`).
  * Built-in `application` scope is similarly not bound directly.

### 6.2 Output and Logging
* Execution occurs outside of an active HTTP socket connection. Any output generated (e.g. `WriteOutput()`, raw HTML/text) is silently discarded.
* If an unhandled exception occurs inside `onSessionEnd` or `onApplicationEnd`:
  * It must be caught and written to the server error log (`application.log` / `exception.log`).
  * The error must not terminate or crash the background reaper thread.

---

## 7. Explicit Invalidation Flow

### 7.1 `SessionInvalidate()`
1. Worker identifies the active session for the current request.
2. Worker copies the active session data and application data to memory for callback execution.
3. Worker deletes the row from SQLite:
   ```sql
   DELETE FROM sessions WHERE session_id = :session_id;
   ```
4. Worker synchronously invokes `onSessionEnd(SessionScope, ApplicationScope)` in the current request thread.
5. Worker clears the request session context and unsets/clears the tracking cookie.

### 7.2 Application Shutdown / Server Stop
1. For each active application, copy application scope data to memory.
2. Delete application records from SQLite.
3. Synchronously invoke `onApplicationEnd(ApplicationScope)`.
4. Flush remaining caches and terminate.
