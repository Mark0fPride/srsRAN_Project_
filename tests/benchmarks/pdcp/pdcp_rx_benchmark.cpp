/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/pdcp/pdcp_entity_rx.h"
#include "lib/pdcp/pdcp_entity_tx.h"
#include "srsran/support/benchmark_utils.h"
#include "srsran/support/executors/manual_task_worker.h"
#include <getopt.h>

using namespace srsran;

const std::array<uint8_t, 16> k_128_int =
    {0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31};
const std::array<uint8_t, 16> k_128_enc =
    {0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31};

/// Mocking class of the surrounding layers invoked by the PDCP.
class pdcp_tx_gen_frame : public pdcp_tx_lower_notifier, public pdcp_tx_upper_control_notifier
{
public:
  /// PDCP TX upper layer control notifier
  void on_max_count_reached() final {}
  void on_protocol_failure() final {}

  /// PDCP TX lower layer data notifier
  void on_new_pdu(pdcp_tx_pdu pdu) final
  {
    byte_buffer_chain buf{std::move(pdu.buf)};
    pdu_list.push_back(std::move(buf));
  }
  void                           on_discard_pdu(uint32_t pdcp_sn) final {}
  std::vector<byte_buffer_chain> pdu_list;
};

/// Mocking class of the surrounding layers invoked by the PDCP.
class pdcp_rx_test_frame : public pdcp_tx_status_handler,
                           public pdcp_rx_upper_data_notifier,
                           public pdcp_rx_upper_control_notifier
{
public:
  /// PDCP TX status handler
  void on_status_report(byte_buffer_chain status) override {}

  /// PDCP RX upper layer data notifier
  void on_new_sdu(byte_buffer sdu) override {}

  /// PDCP RX upper layer control notifier
  void on_integrity_failure() override {}
  void on_protocol_failure() override {}
  void on_max_count_reached() override {}
};

struct bench_params {
  unsigned nof_repetitions = 1000;
};

static void usage(const char* prog, const bench_params& params)
{
  fmt::print("Usage: {} [-R repetitions] [-s silent]\n", prog);
  fmt::print("\t-R Repetitions [Default {}]\n", params.nof_repetitions);
  fmt::print("\t-h Show this message\n");
}

static void parse_args(int argc, char** argv, bench_params& params)
{
  int opt = 0;
  while ((opt = getopt(argc, argv, "R:h")) != -1) {
    switch (opt) {
      case 'R':
        params.nof_repetitions = std::strtol(optarg, nullptr, 10);
        break;
      case 'h':
      default:
        usage(argv[0], params);
        exit(0);
    }
  }
}

std::vector<byte_buffer_chain> gen_pdu_list(security::integrity_enabled   int_enabled,
                                            security::ciphering_enabled   ciph_enabled,
                                            security::integrity_algorithm int_algo,
                                            security::ciphering_algorithm ciph_algo)
{
  timer_manager      timers;
  manual_task_worker worker{64};

  // Set TX config
  pdcp_tx_config config         = {};
  config.rb_type                = pdcp_rb_type::drb;
  config.rlc_mode               = pdcp_rlc_mode::am;
  config.sn_size                = pdcp_sn_size::size18bits;
  config.direction              = pdcp_security_direction::uplink;
  config.discard_timer          = pdcp_discard_timer::ms10;
  config.status_report_required = false;

  security::sec_128_as_config sec_cfg = {};

  // Set security domain
  sec_cfg.domain = security::sec_domain::up; // DRB

  // Set security keys
  sec_cfg.k_128_int = k_128_int;
  sec_cfg.k_128_enc = k_128_enc;

  // Set encription/integrity algorithms
  sec_cfg.integ_algo  = int_algo;
  sec_cfg.cipher_algo = ciph_algo;

  // Create test frame
  pdcp_tx_gen_frame frame = {};

  // Create PDCP entities
  std::unique_ptr<pdcp_entity_tx> pdcp_tx =
      std::make_unique<pdcp_entity_tx>(0, drb_id_t::drb1, config, frame, frame, timer_factory{timers, worker});
  pdcp_tx->configure_security(sec_cfg);
  pdcp_tx->set_integrity_protection(int_enabled);
  pdcp_tx->set_ciphering(ciph_enabled);

  // Prepare SDU list for benchmark
  int num_sdus  = 1000;
  int num_bytes = 1500;
  for (int i = 0; i < num_sdus; i++) {
    byte_buffer sdu_buf = {};
    for (int j = 0; j < num_bytes; ++j) {
      sdu_buf.append(rand());
    }
    pdcp_tx->handle_sdu(std::move(sdu_buf));
  }
  return std::move(frame.pdu_list);
}

void benchmark_pdcp_rx(bench_params                  params,
                       security::integrity_enabled   int_enabled,
                       security::ciphering_enabled   ciph_enabled,
                       security::integrity_algorithm int_algo,
                       security::ciphering_algorithm ciph_algo)
{
  fmt::memory_buffer buffer;
  if (int_enabled == security::integrity_enabled::on || ciph_enabled == security::ciphering_enabled::on) {
    fmt::format_to(buffer, "Benchmark PDCP RX. NIA{} NEA{}", int_algo, ciph_algo);
  } else {
    fmt::format_to(buffer, "Benchmark PDCP RX. NIA0 NEA0");
  }

  std::vector<byte_buffer_chain> pdu_list = gen_pdu_list(int_enabled, ciph_enabled, int_algo, ciph_algo);

  std::unique_ptr<benchmarker> bm = std::make_unique<benchmarker>(to_c_str(buffer), params.nof_repetitions);

  timer_manager      timers;
  manual_task_worker worker{64};

  // Set TX config
  pdcp_rx_config config = {};
  config.rb_type        = pdcp_rb_type::drb;
  config.rlc_mode       = pdcp_rlc_mode::am;
  config.sn_size        = pdcp_sn_size::size18bits;
  config.direction      = pdcp_security_direction::downlink;
  config.t_reordering   = pdcp_t_reordering::ms100;

  security::sec_128_as_config sec_cfg = {};

  // Set security domain
  sec_cfg.domain = security::sec_domain::up; // DRB

  // Set security keys
  sec_cfg.k_128_int = k_128_int;
  sec_cfg.k_128_enc = k_128_enc;

  // Set encription/integrity algorithms
  sec_cfg.integ_algo  = int_algo;
  sec_cfg.cipher_algo = ciph_algo;

  // Create test frame
  pdcp_rx_test_frame frame = {};

  // Create PDCP entities
  std::unique_ptr<pdcp_entity_rx> pdcp_rx =
      std::make_unique<pdcp_entity_rx>(0, drb_id_t::drb1, config, frame, frame, timer_factory{timers, worker});
  pdcp_rx->configure_security(sec_cfg);
  pdcp_rx->set_integrity_protection(int_enabled);
  pdcp_rx->set_ciphering(ciph_enabled);

  // Prepare SDU list for benchmark
  std::vector<byte_buffer> sdu_list  = {};
  int                      num_sdus  = 1000;
  int                      num_bytes = 1500;
  for (int i = 0; i < num_sdus; i++) {
    byte_buffer sdu_buf = {};
    for (int j = 0; j < num_bytes; ++j) {
      sdu_buf.append(rand());
    }
    sdu_list.push_back(std::move(sdu_buf));
  }

  // Run benchmark.
  int  pdcp_sn = 0;
  auto measure = [&pdcp_rx, pdcp_sn, &pdu_list]() mutable {
    pdcp_rx->handle_pdu(std::move(pdu_list[pdcp_sn]));
    pdcp_sn++;
  };
  bm->new_measure("RX PDU", 1500 * 8, measure);

  // Output results.
  bm->print_percentiles_time();

  bm->print_percentiles_throughput(" bps");
}

int main(int argc, char** argv)
{
  srslog::fetch_basic_logger("PDCP").set_level(srslog::basic_levels::error);

  srslog::init();

  bench_params params{};
  parse_args(argc, argv, params);

  {
      // benchmark_pdcp_rx(params,
      //                   security::integrity_enabled::off,
      //                   security::ciphering_enabled::off,
      //                   security::integrity_algorithm::nia2,
      //                   security::ciphering_algorithm::nea0);
  } {
      //  benchmark_pdcp_rx(params,
      //                    security::integrity_enabled::on,
      //                    security::ciphering_enabled::on,
      //                    security::integrity_algorithm::nia1,
      //                    security::ciphering_algorithm::nea1);
  } {
    benchmark_pdcp_rx(params,
                      security::integrity_enabled::on,
                      security::ciphering_enabled::on,
                      security::integrity_algorithm::nia2,
                      security::ciphering_algorithm::nea2);
  }
  {
    //  benchmark_pdcp_rx(params,
    //                    security::integrity_enabled::on,
    //                    security::ciphering_enabled::on,
    //                    security::integrity_algorithm::nia3,
    //                    security::ciphering_algorithm::nea3);
  }
  srslog::flush();
}
