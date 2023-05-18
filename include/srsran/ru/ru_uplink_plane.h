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

#include "srsran/ran/slot_point.h"

namespace srsran {

class prach_buffer;
struct prach_buffer_context;
class resource_grid;
struct resource_grid_context;
class resource_grid_reader;

/// Radio Unit uplink received symbol context.
struct ru_uplink_rx_symbol_context {
  /// Slot context.
  slot_point slot;
  /// Radio sector identifier.
  unsigned sector;
  /// Index, within the slot, of the last processed symbol.
  unsigned symbol_id;
};

/// \brief Radio Unit notifier for events related to received symbols.
///
/// The events generated by this interface are triggered by the requests handled by the \ref ru_downlink_plane_handler
/// interface.
class ru_uplink_plane_rx_symbol_notifier
{
public:
  /// Default destructor.
  virtual ~ru_uplink_plane_rx_symbol_notifier() = default;

  /// \brief Notifies the completion of an OFDM symbol for a given context.
  ///
  /// \param[in] context Notification context.
  /// \param[in] grid    Resource grid that belongs to the context.
  virtual void on_new_uplink_symbol(const ru_uplink_rx_symbol_context& context, const resource_grid_reader& grid) = 0;

  /// \brief Notifies the completion of a PRACH window.
  ///
  /// The Radio Unit uses this method to notify that the PRACH window identified by \c context has been written in \c
  /// buffer.
  ///
  /// \param[in] context PRACH context.
  /// \param[in] buffer  Read-only PRACH buffer.
  virtual void on_new_prach_window_data(const prach_buffer_context& context, const prach_buffer& buffer) = 0;
};

/// \brief Radio Unit uplink plane handler.
///
/// Handles PRACH and uplink data requests and captures uplink data. The uplink received data will be notified through
/// the \ref ru_uplink_plane_rx_symbol_notifier notifier.
class ru_uplink_plane_handler
{
public:
  /// Default destructor.
  virtual ~ru_uplink_plane_handler() = default;

  /// \brief Requests the Radio Unit to capture a PRACH window.
  ///
  /// The Radio Unit must capture a PHY window identified by \c context. The capture must start at slot \c context.slot
  /// and symbol \c context.start_symbol. The capture must finish once \c buffer.is_full() returns true.
  ///
  /// \param[in] context PRACH window context.
  /// \param[in] buffer  PRACH buffer used to write the PRACH window.
  virtual void handle_prach_occasion(const prach_buffer_context& context, prach_buffer& buffer) = 0;

  /// \brief Requests the Radio Unit to provide an uplink slot.
  ///
  /// The Radio Unit must process the slot described by \c context and notify the demodulation per symbol basis of the
  /// requested slot.
  ///
  /// \param[in] context Resource grid context.
  /// \param[in] buffer  Resource grid to store the processed slot.
  virtual void handle_new_uplink_slot(const resource_grid_context& context, resource_grid& grid) = 0;
};

} // namespace srsran
