/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/scheduler/config/logical_channel_config_factory.h"

using namespace srsran;

logical_channel_config srsran::config_helpers::create_default_logical_channel_config(lcid_t lcid)
{
  logical_channel_config lc_ch;
  lc_ch.lcid                      = lcid;
  lc_ch.priority                  = is_srb(lcid) ? 1 : 2;
  lc_ch.lc_group                  = uint_to_lcg_id(is_srb(lcid) ? 0 : 1);
  lc_ch.lc_sr_mask                = false;
  lc_ch.lc_sr_delay_timer_applied = false;
  return lc_ch;
}
