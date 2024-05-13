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

#include "gtpu_pdu.h"
#include "gtpu_tunnel_base_tx.h"
#include "srsran/adt/byte_buffer.h"
#include "srsran/gtpu/gtpu_config.h"
#include "srsran/gtpu/gtpu_tunnel_nru_tx.h"
#include "srsran/nru/nru_packing.h"
#include <arpa/inet.h>
#include <netinet/in.h>

namespace srsran {

/// Class used for transmitting GTP-U NR-U bearers, e.g. on F1-U interface.
class gtpu_tunnel_nru_tx_impl : public gtpu_tunnel_base_tx, public gtpu_tunnel_nru_tx_lower_layer_interface
{
public:
  gtpu_tunnel_nru_tx_impl(srs_cu_up::ue_index_t                             ue_index,
                          gtpu_tunnel_nru_config::gtpu_tunnel_nru_tx_config cfg_,
                          dlt_pcap&                                         gtpu_pcap_,
                          gtpu_tunnel_common_tx_upper_layer_notifier&       upper_dn_) :
    gtpu_tunnel_base_tx(gtpu_tunnel_log_prefix{ue_index, cfg_.peer_teid, "UL"}, gtpu_pcap_, upper_dn_),
    packer(logger.get_basic_logger()),
    cfg(cfg_)
  {
    to_sockaddr(peer_sockaddr, cfg.peer_addr.c_str(), cfg.peer_port);
    logger.log_info("GTPU NR-U Tx configured. {}", cfg);
  }

  /*
   * SDU/PDU handlers
   */

  void handle_sdu(nru_ul_message ul_message) final
  {
    gtpu_header hdr         = {};
    hdr.flags.version       = GTPU_FLAGS_VERSION_V1;
    hdr.flags.protocol_type = GTPU_FLAGS_GTP_PROTOCOL;
    hdr.flags.ext_hdr       = true;
    hdr.message_type        = GTPU_MSG_DATA_PDU;
    hdr.length              = 0; // this will be computed automatically
    hdr.teid                = cfg.peer_teid;
    hdr.next_ext_hdr_type   = gtpu_extension_header_type::nr_ran_container;

    if (!ul_message.data_delivery_status.has_value()) {
      logger.log_error("Dropped SDU, missing data_delivery_status. teid={}", hdr.teid);
      return;
    }

    byte_buffer ext_buf;
    if (!packer.pack(ext_buf, ul_message.data_delivery_status.value())) {
      logger.log_error("Dropped SDU, error writing NR RAN container to GTP-U extension header. teid={} ext_len={}",
                       hdr.teid,
                       ext_buf.length());
      return;
    }

    gtpu_extension_header ext;
    ext.extension_header_type = gtpu_extension_header_type::nr_ran_container;
    ext.container             = ext_buf;

    hdr.ext_list.push_back(ext);

    byte_buffer buf;
    if (ul_message.t_pdu.has_value()) {
      expected<byte_buffer> exp_buf = ul_message.t_pdu.value().deep_copy();
      if (!exp_buf.has_value()) {
        logger.log_error("Dropped SDU, failed to allocate PDU buffer. teid={}", hdr.teid);
        return;
      }
      buf = std::move(exp_buf.value());
    }
    bool write_ok = gtpu_write_header(buf, hdr, logger);

    if (!write_ok) {
      logger.log_error("Dropped SDU, error writing GTP-U header. teid={}", hdr.teid);
      return;
    }
    logger.log_info(buf.begin(), buf.end(), "TX PDU. pdu_len={} teid={}", buf.length(), hdr.teid);
    send_pdu(std::move(buf), peer_sockaddr);
  }

private:
  nru_packing packer;

  const gtpu_tunnel_nru_config::gtpu_tunnel_nru_tx_config cfg;
  sockaddr_storage                                        peer_sockaddr;
};
} // namespace srsran
