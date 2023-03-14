/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/fapi_adaptor/phy/messages/prach.h"
#include "srsran/phy/support/prach_buffer_context.h"

using namespace srsran;
using namespace fapi_adaptor;

void srsran::fapi_adaptor::convert_prach_fapi_to_phy(prach_buffer_context&       context,
                                                     const fapi::ul_prach_pdu&   fapi_pdu,
                                                     const fapi::prach_config&   prach_cfg,
                                                     const fapi::carrier_config& carrier_cfg,
                                                     unsigned                    sfn,
                                                     unsigned                    slot,
                                                     unsigned                    sector_id)
{
  srsran_assert(fapi_pdu.maintenance_v3.prach_config_scope == fapi::prach_config_scope_type::phy_context,
                "Common context not supported.");
  srsran_assert(fapi_pdu.maintenance_v3.prach_res_config_index == 0,
                "Only PRACH resource configuration index 0 supported.");
  srsran_assert(fapi_pdu.index_fd_ra == 0, "Only one FD occasion supported.");
  srsran_assert(fapi_pdu.num_prach_ocas == 1, "Only one PRACH occasion supported.");

  context.slot                 = slot_point(prach_cfg.prach_ul_bwp_pusch_scs, sfn, slot);
  context.sector               = sector_id;
  context.format               = fapi_pdu.prach_format;
  context.start_symbol         = fapi_pdu.prach_start_symbol;
  context.start_preamble_index = fapi_pdu.maintenance_v3.start_preamble_index;
  context.nof_preamble_indices = fapi_pdu.maintenance_v3.num_preamble_indices;

  context.pusch_scs       = prach_cfg.prach_ul_bwp_pusch_scs;
  context.restricted_set  = prach_cfg.restricted_set;
  context.nof_prb_ul_grid = carrier_cfg.ul_grid_size[to_numerology_value(context.pusch_scs)];

  srsran_assert(fapi_pdu.index_fd_ra < prach_cfg.fd_occasions.size(), "Index FD RA out of bounds");
  const fapi::prach_fd_occasion_config& fd_occas = prach_cfg.fd_occasions[fapi_pdu.index_fd_ra];
  context.rb_offset                              = fd_occas.prach_freq_offset;
  context.root_sequence_index                    = fd_occas.prach_root_sequence_index;
  context.zero_correlation_zone                  = fd_occas.prach_zero_corr_conf;

  // NOTE: set the port to 0 for now.
  context.port = 0;
}
