/**
 * @file fn_transaction.cpp
 * @brief CFML transactioncommit() / transactionrollback() / transactionSetSavepoint() built-ins.
 */

#include "common.h"

#include "../cftags/common.h"
#include <webstrada/cf8.h>
#include <webstrada/cfvariant.h>
#include <webstrada/exceptions.h>

namespace cfml {

cfvariant *cf_transactioncommit() {
    if (g_txStack.empty()) {
        throw webstrada::exception("An active transaction is required before calling any transaction method.");
    }
    cf_transaction_commit();
    return cfvariant_create_null();
}

cfvariant *cf_transactionrollback(const cfvariant *savepoint) {
    if (g_txStack.empty()) {
        throw webstrada::exception("An active transaction is required before calling any transaction method.");
    }
    if (savepoint) {
        std::string name = safe_to_std_string(*savepoint);
        cf_transaction_rollback_to(name);
    } else {
        cf_transaction_rollback();
    }
    return cfvariant_create_null();
}

cfvariant *cf_transactionsetsavepoint(const cfvariant *savepoint) {
    if (g_txStack.empty()) {
        throw webstrada::exception("An active transaction is required before calling any transaction method.");
    }
    if (!savepoint) {
        throw webstrada::exception("The savepoint argument is required.");
    }
    std::string name = safe_to_std_string(*savepoint);
    cf_transaction_setsavepoint(name);
    // TransactionSetSavePoint returns void (CF: "cannot be assigned because it
    // does not return a value"); the caller discards this value.
    return cfvariant_create_null();
}

} // namespace cfml
