/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "dummy_mac_result_notifier.h"

using namespace srsran;

phy_cell_test_dummy::phy_cell_test_dummy(task_executor& exec_) : test_exec(exec_) {}

void phy_cell_test_dummy::on_new_downlink_scheduler_results(const mac_dl_sched_result& dl_res)
{
  cached_dl_res = dl_res;
}

void phy_cell_test_dummy::on_new_downlink_data(const mac_dl_data_result& dl_data)
{
  cached_dl_data = dl_data;
}

void phy_cell_test_dummy::on_new_uplink_scheduler_results(const mac_ul_sched_result& ul_res)
{
  cached_ul_res = ul_res;
}

void phy_cell_test_dummy::on_cell_results_completion(slot_point slot)
{
  bool result =
      test_exec.execute([this,
                         slot,
                         dl_sched_res = cached_dl_res.has_value() ? *cached_dl_res->dl_res : dl_sched_result{},
                         ul_sched_res = cached_ul_res.has_value() ? *cached_ul_res->ul_res : ul_sched_result{},
                         dl_res_copy  = cached_dl_res,
                         dl_data_copy = cached_dl_data,
                         ul_res_copy  = cached_ul_res]() mutable {
        last_slot_res = slot;
        last_dl_res   = dl_res_copy;
        last_dl_data  = dl_data_copy;
        last_ul_res   = ul_res_copy;
        if (last_dl_res.has_value()) {
          last_dl_sched_res   = dl_sched_res;
          last_dl_res->dl_res = &last_dl_sched_res;
        }
        if (last_ul_res.has_value()) {
          last_ul_sched_res   = ul_sched_res;
          last_ul_res->ul_res = &last_ul_sched_res;
        }
      });
  srsran_assert(result, "Failed to execute task");
  cached_dl_res  = {};
  cached_dl_data = {};
  cached_ul_res  = {};
}
