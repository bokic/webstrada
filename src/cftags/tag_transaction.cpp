/**
 * @file tag_transaction.cpp
 * @brief <cftransaction> tag runtime implementations.
 *
 * The transaction frame stack (g_txStack) and the shared
 * transaction_get_active / transaction_clear_all helpers live in common.cpp.
 * Transaction control is issued through the abstract database layer
 * (webstrada::db::DBConnection) so it works on every backend.
 */

#include "common.h"

#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/db.h>

#include <algorithm>
#include <string>
#include <vector>

namespace cfml {

void cf_transaction_begin(const cfvariant *attrs)
{
    (void)attrs;
    cfml::trace_record_event("DB_TRANSACTION_START", "", "", 0);
    g_txStack.emplace_back();
}

void cf_transaction_commit()
{
    if (g_txStack.empty()) return;
    TxFrame &f = g_txStack.back();
    if (f.conn) {
        if (f.inTransaction) f.conn->commit();
        f.conn = nullptr;
    }
    g_txStack.pop_back();
    cfml::trace_record_event("DB_TRANSACTION_COMMIT", "", "", 0);
}

void cf_transaction_rollback()
{
    if (g_txStack.empty()) return;
    TxFrame &f = g_txStack.back();
    if (f.conn) {
        if (f.inTransaction) f.conn->rollback();
        f.conn = nullptr;
    }
    g_txStack.pop_back();
    cfml::trace_record_event("DB_TRANSACTION_ROLLBACK", "", "", 0);
}

// TransactionSetSavePoint: name a point within the active transaction so a later
// transactionRollback(name) undoes everything after it. Maps to the backend's
// SAVEPOINT (the name is tracked on the frame; ROLLBACK TO releases only up to
// that savepoint, unlike transactionRollback() which ends the whole
// transaction).
void cf_transaction_setsavepoint(const std::string &name)
{
    if (g_txStack.empty()) {
        throw webstrada::exception("An active transaction is required before calling any transaction method.");
    }
    TxFrame &f = g_txStack.back();
    if (f.conn && f.inTransaction) {
        f.conn->setSavepoint(name);
    }
    f.savepoints.push_back(name);
}

// transactionRollback(name): roll back to the named savepoint (undo everything
// after it) and continue the transaction. Returns false when the savepoint is
// unknown (CF: "Specified savepoint does not exist or is invalid.").
bool cf_transaction_rollback_to(const std::string &name)
{
    if (g_txStack.empty()) {
        throw webstrada::exception("An active transaction is required before calling any transaction method.");
    }
    TxFrame &f = g_txStack.back();
    // Find the named savepoint; CF requires it to exist.
    auto it = std::find(f.savepoints.begin(), f.savepoints.end(), name);
    if (it == f.savepoints.end()) {
        throw webstrada::exception("Specified savepoint does not exist or is invalid.");
    }
    if (f.conn && f.inTransaction) {
        f.conn->rollbackTo(name);
    }
    // Savepoints created after `name` are gone (rolled back past).
    f.savepoints.erase(it + 1, f.savepoints.end());
    return true;
}

} // namespace cfml
