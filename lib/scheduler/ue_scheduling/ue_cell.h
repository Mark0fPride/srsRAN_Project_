/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../support/bwp_helpers.h"
#include "harq_process.h"
#include "ue_configuration.h"
#include "srsran/ran/uci/uci_constants.h"
#include "srsran/scheduler/config/scheduler_expert_config.h"
#include "srsran/scheduler/scheduler_feedback_handler.h"

namespace srsran {

struct ul_crc_pdu_indication;

struct grant_prbs_mcs {
  /// MCS to use for the UE's PUSCH.
  sch_mcs_index mcs;
  /// Number of PRBs to be allocated for the UE's PUSCH.
  unsigned n_prbs;
};

struct ue_csi_report {
  optional<uint8_t> wb_cqi;
  optional<uint8_t> ri;
  optional<uint8_t> pmi;
};

/// \brief Context respective to a UE serving cell.
class ue_cell
{
public:
  struct metrics {
    /// Save the latest PUSCH SNR reported from PHY, in dB.
    // NOTE: the 0 is only used for initialization and will be overwritten by the first UL SNR report.
    double   pusch_snr_db          = 0.0;
    unsigned consecutive_pusch_kos = 0;
    // This gets initialized in the ue_cell constructor.
    unsigned latest_wb_cqi;
    unsigned latest_ri  = 0;
    unsigned latest_pmi = 0;
  };

  ue_cell(du_ue_index_t                     ue_index_,
          rnti_t                            crnti_val,
          const scheduler_ue_expert_config& expert_cfg_,
          const cell_configuration&         cell_cfg_common_,
          const serving_cell_config&        ue_serv_cell,
          ue_harq_timeout_notifier          harq_timeout_notifier);

  const du_ue_index_t   ue_index;
  const du_cell_index_t cell_index;

  harq_entity harqs;

  rnti_t rnti() const { return crnti_; }

  bwp_id_t active_bwp_id() const { return to_bwp_id(0); }
  bool     is_active() const { return true; }

  const ue_cell_configuration& cfg() const { return ue_cfg; }

  void handle_reconfiguration_request(const serving_cell_config& new_ue_cell_cfg);

  unsigned get_latest_wb_cqi() const { return ue_metrics.latest_wb_cqi; }

  /// Update UE with the latest CSI report for a given cell.
  void handle_csi_report(const uci_indication::uci_pdu::csi_report& csi);

  /// \brief Estimate the number of required DL PRBs to allocate the given number of bytes.
  grant_prbs_mcs required_dl_prbs(const pdsch_time_domain_resource_allocation& pdsch_td_cfg,
                                  unsigned                                     pending_bytes) const;

  /// \brief Estimate the number of required UL PRBs to allocate the given number of bytes.
  grant_prbs_mcs required_ul_prbs(const pusch_time_domain_resource_allocation& pusch_td_cfg,
                                  unsigned                                     pending_bytes,
                                  dci_ul_rnti_config_type                      type) const;

  uint8_t get_pdsch_rv(const dl_harq_process& h_dl) const
  {
    return expert_cfg.pdsch_rv_sequence[h_dl.tb(0).nof_retxs % expert_cfg.pdsch_rv_sequence.size()];
  }
  uint8_t get_pusch_rv(const ul_harq_process& h_ul) const
  {
    return expert_cfg.pusch_rv_sequence[h_ul.tb().nof_retxs % expert_cfg.pusch_rv_sequence.size()];
  }

  /// \brief Handle CRC PDU indication.
  int handle_crc_pdu(slot_point pusch_slot, const ul_crc_pdu_indication& crc_pdu);

  /// \brief Fetches number of layers to be used in DL based on reported RI.
  unsigned get_nof_dl_layers() const { return ue_metrics.latest_ri + 1; }

  /// \brief Fetches number of layers to be used in UL based on reported RI.
  unsigned get_nof_ul_layers() const { return 1; }

  /// \brief Get the current UE cell metrics.
  const metrics& get_metrics() const { return ue_metrics; }
  metrics&       get_metrics() { return ue_metrics; }

private:
  /// Update PUSCH SNR metric of the UE.
  void update_pusch_snr(optional<float> snr)
  {
    if (snr.has_value()) {
      ue_metrics.pusch_snr_db = static_cast<double>(snr.value());
    }
  }

  rnti_t                            crnti_;
  const scheduler_ue_expert_config& expert_cfg;
  ue_cell_configuration             ue_cfg;

  metrics ue_metrics;
};

} // namespace srsran
