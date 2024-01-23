/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mock_cu_up.h"
#include "srsran/e1ap/common/e1ap_common.h"
#include "srsran/e1ap/common/e1ap_message.h"

using namespace srsran;
using namespace srs_cu_cp;

namespace {

/// \brief Mock class for the interface between CU-UP and CU-CP that accounts for the fact that the CU-CP may push
/// PDUs from different threads.
class synchronized_mock_cu_up : public mock_cu_up
{
public:
  synchronized_mock_cu_up(cu_up_repository& cu_cp_e1_handler_) : cu_cp_node(&cu_cp_e1_handler_)
  {
    tx_pdu_notifier = cu_cp_node->handle_new_cu_up_connection(std::make_unique<rx_pdu_notifier>(*this));
  }

  bool connected() const { return tx_pdu_notifier != nullptr; }

  bool request_cu_cp_connection() override
  {
    class sink_e1ap_message_notifier : public e1ap_message_notifier
    {
    public:
      sink_e1ap_message_notifier(synchronized_mock_cu_up& cu_up_) : cu_up(cu_up_) {}
      void on_new_message(const e1ap_message& msg) override { cu_up.handle_rx_pdu(msg); }

    private:
      synchronized_mock_cu_up& cu_up;
    };

    // Create notifier that will save the PDUs sent by the CU-CP.
    auto cu_cp_tx_notifier = std::make_unique<sink_e1ap_message_notifier>(*this);

    // Start CU-UP connection to CU-CP.
    tx_pdu_notifier = cu_cp_node->handle_new_cu_up_connection(std::move(cu_cp_tx_notifier));
    return tx_pdu_notifier != nullptr;
  }

  void push_tx_pdu(const e1ap_message& rx_pdu) override
  {
    report_fatal_error_if_not(tx_pdu_notifier != nullptr, "CU-UP is not connected to CU-CP");
    tx_pdu_notifier->on_new_message(rx_pdu);
  }

  bool try_pop_rx_pdu(e1ap_message& pdu) override { return rx_pdus.try_pop(pdu); }

private:
  using rx_pdu_queue = concurrent_queue<e1ap_message,
                                        concurrent_queue_policy::locking_mpmc,
                                        concurrent_queue_wait_policy::condition_variable>;

  struct rx_pdu_notifier : public e1ap_message_notifier {
    rx_pdu_notifier(synchronized_mock_cu_up& parent_) : parent(parent_) {}

    void on_new_message(const e1ap_message& msg) override { parent.handle_rx_pdu(msg); }

  private:
    synchronized_mock_cu_up& parent;
  };

  void handle_rx_pdu(const e1ap_message& rx_pdu)
  {
    report_fatal_error_if_not(rx_pdus.push_blocking(rx_pdu), "queue is full");
  }

  cu_up_repository* cu_cp_node = nullptr;

  rx_pdu_queue rx_pdus{1024};

  // Notifier used by the CU-UP to send PDUs to the CU-CP.
  std::unique_ptr<e1ap_message_notifier> tx_pdu_notifier;
};

} // namespace

std::unique_ptr<mock_cu_up> srsran::srs_cu_cp::create_mock_cu_up(cu_up_repository& cu_cp_e1_handler)
{
  return std::make_unique<synchronized_mock_cu_up>(cu_cp_e1_handler);
}
