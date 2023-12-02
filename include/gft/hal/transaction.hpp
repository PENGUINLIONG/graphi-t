#pragma once
#include "gft/hal/context.hpp"

namespace liong {
namespace hal {

struct Transaction;
typedef std::shared_ptr<Transaction> TransactionRef;

struct TransactionInfo {
  std::string label;
};
struct Transaction : std::enable_shared_from_this<Transaction> {
  const TransactionInfo info;

  Transaction(TransactionInfo&& info) : info(std::move(info)) {}
  virtual ~Transaction() {}

  // Check whether the transaction is finished. `true` is returned if so.
  virtual bool is_done() = 0;
  // Wait the invocation submitted to device for execution. Returns immediately
  // if the invocation has already been waited.
  virtual void wait() = 0;
};

} // namespace hal
} // namespace liong
