/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "gtpu_teid.h"
#include "fmt/format.h"
#include <chrono>
#include <cstdint>
#include <string>

namespace srsran {

/// Port specified for Encapsulated T-PDUs,
/// TS 29.281 Sec. 4.4.2.3
constexpr unsigned GTPU_PORT = 2152;

/// \brief Configurable parameters for GTP-U NG-U tunnels
struct gtpu_tunnel_ngu_config {
  struct gtpu_tunnel_ngu_rx_config {
    gtpu_teid_t               local_teid;
    std::chrono::milliseconds t_reordering = {};
  } rx;
  struct gtpu_tunnel_ngu_tx_config {
    gtpu_teid_t peer_teid;
    std::string peer_addr;
    uint16_t    peer_port;
  } tx;
};

/// \brief Configurable parameters for GTP-U NR-U tunnels
struct gtpu_tunnel_nru_config {
  struct gtpu_tunnel_nru_rx_config {
    gtpu_teid_t local_teid;
  } rx;
  struct gtpu_tunnel_nru_tx_config {
    gtpu_teid_t peer_teid;
    std::string peer_addr;
    uint16_t    peer_port;
  } tx;
};

} // namespace srsran

//
// Formatters
//
namespace fmt {

// GTP-U NG-U RX config
template <>
struct formatter<srsran::gtpu_tunnel_ngu_config::gtpu_tunnel_ngu_rx_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_ngu_config::gtpu_tunnel_ngu_rx_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "local_teid={} t_reordering={}", cfg.local_teid, cfg.t_reordering);
  }
};

// GTP-U NG-U TX config
template <>
struct formatter<srsran::gtpu_tunnel_ngu_config::gtpu_tunnel_ngu_tx_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_ngu_config::gtpu_tunnel_ngu_tx_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "peer_teid={} peer_addr={} peer_port={}", cfg.peer_teid, cfg.peer_addr, cfg.peer_port);
  }
};

// GTP-U NG-U config
template <>
struct formatter<srsran::gtpu_tunnel_ngu_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_ngu_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "{} {}", cfg.rx, cfg.tx);
  }
};

// GTP-U NR-U RX config
template <>
struct formatter<srsran::gtpu_tunnel_nru_config::gtpu_tunnel_nru_rx_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_nru_config::gtpu_tunnel_nru_rx_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "local_teid={}", cfg.local_teid);
  }
};

// GTP-U NR-U TX config
template <>
struct formatter<srsran::gtpu_tunnel_nru_config::gtpu_tunnel_nru_tx_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_nru_config::gtpu_tunnel_nru_tx_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "peer_teid={} peer_addr={} peer_port={}", cfg.peer_teid, cfg.peer_addr, cfg.peer_port);
  }
};

// GTP-U NR-U config
template <>
struct formatter<srsran::gtpu_tunnel_nru_config> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::gtpu_tunnel_nru_config& cfg, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "{} {}", cfg.rx, cfg.tx);
  }
};

} // namespace fmt
