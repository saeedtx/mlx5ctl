# mlx5ctl: Userspace Linux Debug Utilities for mlx5 ConnectX Devices

[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

This project provides a set of diagnostic tools for mlx5 ConnectX devices
to be used in production and development environments.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
- [Usage](#usage)
  - [Help and list devices](#help-and-list-devices)
  - [Device Info](#device-info)
  - [Device capabilities](#device-capabilities)
  - [Register dump](#register-dump)
  - [Object dump](#object-dump)
  - [Diagnostic counters](#diagnostic-counters)
  - [Resource dump](#resource-dump)
  - [Core dump](#core-dump)
  - [Umem mode](#umem-mode)

- [Future work](#future-work)
- [License](#license)

## Overview

ConnectX devices are complex and provide a vast set of features and components
(SmartNiCs SoCs, Multi-protocol Network Adapters with Ethernent, Infiniband, Storage, DPU,
and many acceleration and offload capabilities), this project will provide
unified tool set to access, debug, diagnose and monitor those devices using the
ConnectX architecture and onboard processors and firmware.

The ConnectX HW family supported by the mlx5 drivers uses an architecture
where a FW component executes "mailbox RPCs" issued by the driver to make
changes to the device. This results in a complex debugging environment
where the FW component has information and low level configuration that
needs to be accessed to userspace for debugging purposes.

mlx5ctl uAPI provides a safe and secure interface to access such debug
information, the tools in this project will make use of the mlx5ctl ioctl
interface to make it easier for users to diagnose and monitor production
systems out of the box, especially on systems with kernel lockdown enabled.

A special FW UID (user context ID) restricted to debug RPCs only, will be allocated
per user access, where all user debug requests will be executed under this UID.

The tools use the public ConnectX PRM [Programming Reference Manual](https://network.nvidia.com/files/doc-2020/ethernet-adapters-programming-manual.pdf)
See Chapter 12 "Command Reference".

## Features

This tool provides a set of utilities to access and debug ConnectX devices
and dump various device resources and objects, it is designed to
provide human readable output and supports raw hex and binary format clean
dumps to be used in conjunction with other parsing tools such as
"parseadb"

- List of currently enabled mlx5ctl devices
- info: Provide information on the current allocated UID
- cap: Dump device capabilities
- reg: Dump ConnectX registers by ID
- obj: Dump ConnectX objects by ID
- diagcnt: Enable high frequency debug sampling of diagnostic counters by ID
- rscdump: Batched object resource dump, Dump multiple contexts at once
- coredump: Internal Firmware and onboard core dumps
- umem mode: allow dumps to go directly into userspace buffers efficiently

## Getting Started

### Prerequisites

#### Drivers needed
  - mlx5_core.ko (ConnectX device core driver)
  - mlx5ctl.ko (ConnectX debug uAPI driver)

### Installation

```bash
$ make
$ sudo make install

$ mlx5ctl
Usage: mlx5ctl <mlx5ctl device> <command> [options]
Verbosity: mlx5ctl -v <mlx5ctl device> <command> [options]
Commands:
        info: Print device information
        cap: Query FW and show some device caps
        reg: Dump access registers
        obj: Query and dump objects
        diagcnt: Dump diagnostic counters
        rscdump: Dump resources
        coredump: CR core dump
        help: Show this help
        sleep: sleep
Found 2 mlx5ctl devices:
/dev/mlx5ctl-mlx5_core.ctl.0
/dev/mlx5ctl-mlx5_core.ctl.1

```

### Usage
mlx5ctl hosts multiple commands, each command can extend it's own sub-commands and options,
for every command: `mlx5ctl <command> help` will provide the command and sub-command help menu
Many subcommands provide raw binary dumps that can be parsed using the parseadb tool.

to enable verbosity:
`mlx5ctl -v <command> [option]`

```bash
$ mlx5ctl
Usage: mlx5ctl <mlx5ctl device> <command> [options]
Verbosity: mlx5ctl -v <mlx5ctl device> <command> [options]
Commands:
        info: Print device information
        cap: Query device caps
        reg: Dump access registers
        obj: Query and dump objects
        diagcnt: Dump diagnostic counters
        rscdump: Dump resources
        coredump: CR core dump
        help: Show this help
        sleep: sleep
Found 2 mlx5ctl devices:
/dev/mlx5ctl-mlx5_core.ctl.0
/dev/mlx5ctl-mlx5_core.ctl.1
```

#### Help and list devices
make sure mlx5ctl.ko is loaded,
misc mlx5ctl linux devices can be discovered under:
```bash
$ ls -1 /dev/mlx5ctl-*
/dev/mlx5ctl-mlx5_core.ctl.0
/dev/mlx5ctl-mlx5_core.ctl.1
```
to list the devices using the mlx5ctl tool:
```bash
$ mlx5ctl | sed -n '/^Found/,$p'
Found 2 mlx5ctl devices:
/dev/mlx5ctl-mlx5_core.ctl.0
/dev/mlx5ctl-mlx5_core.ctl.1
```

#### Device Info
Get specific device info and debug UID info
```bash
$ sudo mlx5ctl mlx5_core.ctl.0
mlx5dev: 0000:00:04.0
UCTX UID: 1
UCTX CAP: 0x3
DEV UCTX CAP: 0x3
USER CAP: 0x1d
Current PID: 778 FD 3
```

#### Device capabilities
```bash
$ mlx5ctl mlx5_core.ctl.0 cap --help
mlx5ctl <device> cap --id=<cap_type> --mode=[cur|max] -[B|H|P]
Query device capabilities, outputs PRM struct of the specific cap type
        --id=<cap_type> - cap type id or name
        --mode=[cur|max] - cap mode
        -B - binary output
        -H - hex output
        -P - pretty output
        -h - help
Supported cap types:
        GENERAL type=0x0
        ETHERNET_OFFLOADS type=0x1
        ROCE type=0x4
        GENERAL_2 type=0x20
        PORT_SELECTION type=0x25
        ADV_VIRTUALIZATION type=0x26
        DEBUG type=0xd
        ODP type=0x2
        ATOMIC type=0x3
        IPOIB_OFFLOADS type=0x5
        IPOIB_ENHANCED_OFFLOADS type=0x6
        FLOW_TABLE type=0x7
        ESWITCH_FLOW_TABLE type=0x8
        ESWITCH type=0x9
        QOS type=0xc
        DEV_MEM type=0xf
        TLS type=0x11
        VDPA_EMULATION type=0x13
        DEV_EVENT type=0x14
        IPSEC type=0x15
        CRYPTO type=0x1a
        MACSEC type=0x1f

```

##### Example 1: Dump ETHERNET_OFFLOADS cap with pretty output
```bash
$ mlx5ctl mlx5_core.ctl.0 cap --id=ETHERNET_OFFLOADS -P
        csum_cap: 1
        vlan_cap: 1
        lro_cap: 1
        lro_psh_flag: 0
        lro_time_stamp: 1
        wqe_vlan_insert: 0
        self_lb_en_modifiable: 1
        max_lso_cap: 18
        multi_pkt_send_wqe: 1
        wqe_inline_mode: 2
        rss_ind_tbl_cap: 11
        reg_umr_sq: 1
        scatter_fcs: 1
        enhanced_multi_pkt_send_wqe: 1
        tunnel_lso_const_out_ip_id: 0
        tunnel_lro_gre: 0
        tunnel_lro_vxlan: 0
        tunnel_stateless_gre: 1
        tunnel_stateless_vxlan: 1
        swp: 1
        swp_csum: 1
        swp_lso: 1
        cqe_checksum_full: 0
        tunnel_stateless_geneve_tx: 1
        tunnel_stateless_mpls_over_udp: 0
        tunnel_stateless_mpls_over_gre: 0
        tunnel_stateless_vxlan_gpe: 1
        tunnel_stateless_ipv4_over_vxlan: 1
        tunnel_stateless_ip_over_ip: 1
        insert_trailer: 1
        tunnel_stateless_ip_over_ip_rx: 1
        tunnel_stateless_ip_over_ip_tx: 1
        max_vxlan_udp_ports: 0
        max_geneve_opt_len: 1
        tunnel_stateless_geneve_rx: 1
        lro_min_mss_size: 1348
        lro_timer_supported_periods[0]: 8
        lro_timer_supported_periods[1]: 16
        lro_timer_supported_periods[2]: 32
        lro_timer_supported_periods[3]: 1024
        enhanced_multi_pkt_send_wqe: 1
```

##### Example 2: Dump QOS cap with hex output and binary output
```bash
$ mlx5ctl mlx5_core.ctl.0 cap --id=QOS
ff f1 0c 2a 42 a8 00 00 05 f5 e1 00 00 00 00 01
00 04 00 80 00 0f 00 07 00 00 00 08 00 00 00 64
00 11 00 01 06 06 17 00 0d 0d 00 00 00 00 00 00
...

# Use parseadb to parse the binary output
$ mlx5ctl mlx5_core.ctl.0 cap --id=QOS -B | parseadb qos_caps
ff f1 0c 2a 42 a8 00 00 05 f5 e1 00 00 00 00 01
00 04 00 80 00 0f 00 07 00 00 00 08 00 00 00 64
00 11 00 01 06 06 17 00 0d 0d 00 00 00 00 00 00
Node: qos_caps
        packet_pacing: 0x1 (1)
        esw_scheduling: 0x1 (1)
        esw_bw_share: 0x1 (1)
        esw_rate_limit: 0x1 (1)
        hll: 0x1 (1)
        packet_pacing_burst_bound: 0x1 (1)
        packet_pacing_typical_size: 0x1 (1)
        flow_meter_old: 0x1 (1)
        nic_sq_scheduling: 0x1 (1)
        nic_bw_share: 0x1 (1)
        nic_rate_limit: 0x1 (1)
        packet_pacing_uid: 0x1 (1)
        log_esw_max_sched_depth: 0x1 (1)
        log_max_flow_meter: 0xc (12)
        flow_meter_reg_id: 0x2a (42)
        wqe_rate_pp: 0x0 (0)
        nic_qp_scheduling: 0x1 (1)
        log_nic_max_sched_depth: 0x2 (2)
        flow_meter: 0x1 (1)
        qos_remap_pp: 0x1 (1)
        log_max_qos_nic_queue_group: 0x8 (8)
        max_flow_meter_bs_exponent: 0x0 (0)
        packet_pacing_max_rate: 0x5f5e100 (100000000)
        packet_pacing_min_rate: 0x1 (1)
        log_esw_max_rate_limit: 0x4 (4)
        packet_pacing_rate_table_size: 0x80 (128)
        esw_element_type: 0xf (15)
        esw_tsar_type: 0x7 (7)
        max_qos_para_vport: 0x0 (0)
        max_qos_para_vport_old: 0x8 (8)
        max_tsar_bw_share: 0x64 (100)
        nic_element_type: 0x11 (17)
        nic_tsar_type: 0x1 (1)
        log_meter_aso_granularity: 0x6 (6)
        log_meter_aso_max_alloc: 0x6 (6)
        log_max_num_meter_aso: 0x17 (23)
        log_max_qos_nic_scheduling_element: 0xd (13)
        log_max_qos_esw_scheduling_element: 0xd (13)
```

##### Example 3: Dump all caps that support pretty output
```bash
# when provided with no --id= parameter, dumps all caps that support pretty output
$ mlx5ctl mlx5_core.ctl.0 cap

MLX5_CAP_GENERAL:
        shared_object_to_user_object_allowed: 0
        vhca_resource_manager: 1
        hca_cap_2: 1
        create_lag_when_not_master_up: 0
        dtor: 1
        event_on_vhca_state_teardown_request: 1
        event_on_vhca_state_in_use: 1
        event_on_vhca_state_active: 1
        event_on_vhca_state_allocated: 1
        event_on_vhca_state_invalid: 0
        vhca_id: 0
        log_max_srq_sz: 15
        log_max_qp_sz: 15
        event_cap: 1
        isolate_vl_tc_new: 1
        prio_tag_required: 0
        log_max_qp: 17
        ece_support: 1
        reg_c_preserve: 1
        log_max_srq: 23
        uplink_follow: 0
        ts_cqe_to_dest_cqn: 1
        shampo: 0
        max_sgl_for_optimized_performance: 3
        log_max_cq_sz: 22
        relaxed_ordering_write_umr: 0
        relaxed_ordering_read_umr: 0
        virtio_net_device_emualtion_manager: 0
        virtio_blk_device_emualtion_manager: 0
        log_max_cq: 24
        log_max_eq_sz: 22
        relaxed_ordering_write: 1
        relaxed_ordering_read: 1
        log_max_mkey: 24
        dump_fill_mkey: 1
        fast_teardown: 1
        log_max_eq: 7
        max_indirection: 4
        fixed_buffer_size: 1
        log_max_mrw_sz: 64
        force_teardown: 1
        log_max_bsf_list_size: 16
        umr_extended_translation_offset: 1
        null_mkey: 1
        log_max_klm_list_size: 16
        log_max_ra_req_dc: 4
        eth_wqe_too_small: 1
        vnic_env_cq_overrun: 1
        log_max_ra_res_dc: 4
        release_all_pages: 1
        must_not_use: 0
        roce_accl: 1
        log_max_ra_req_qp: 4
        log_max_ra_res_qp: 4
        end_pad: 1
        cc_query_allowed: 1
        cc_modify_allowed: 1
        start_pad: 1
        cache_line_128byte: 0
        rts2rts_qp_counters_set_id: 1
        vnic_env_int_rq_oob: 1
        sbcam_reg: 1
        qcam_reg: 1
        gid_table_size: 0
        out_of_seq_cnt: 1
        vport_counters: 1
        retransmission_q_counters: 1
        debug: 1
        modify_rq_counter_set_id: 1
        rq_delay_drop: 1
        max_qp_cnt: 127
        pkey_table_size: 0
        vport_group_manager: 1
        vhca_group_manager: 1
        ib_virt: 1
        eth_virt: 1
        vnic_env_queue_counters: 1
        ets: 1
        nic_flow_table: 1
        eswitch_manager: 1
        device_memory: 1
        mcam_reg: 1
        pcam_reg: 1
        local_ca_ack_delay: 16
        port_module_event: 1
        enhanced_error_q_counters: 1
        ports_check: 1
        disable_link_up: 0
        beacon_led: 1
        port_type: 1
        num_ports: 1
        pps: 1
        pps_modify: 1
        log_max_msg: 30
        max_tc: 0
        temp_warn_event: 1
        dcbx: 0
        general_notification_event: 1
        fpga: 0
        rol_s: 0
        rol_g: 0
        wol_s: 0
        wol_g: 0
        wol_a: 0
        wol_b: 0
        wol_m: 0
        wol_u: 0
        wol_p: 0
        stat_rate_support: 1
        pci_sync_for_fw_update_event: 1
        init2_lag_tx_port_affinity: 1
        cqe_version: 1
        compact_address_vector: 1
        striding_rq: 1
        ipoib_enhanced_offloads: 0
        ipoib_basic_offloads: 0
        repeated_block_disabled: 0
        umr_modify_entity_size_disabled: 0
        umr_modify_atomic_disabled: 0
        umr_indirect_mkey_disabled: 0
        umr_fence: 1
        dc_req_scat_data_cqe: 1
        drain_sigerr: 1
        cmdif_checksum: 0
        sigerr_cqe: 1
        wq_signature: 1
        sctr_data_cqe: 1
        sho: 1
        tph: 0
        rf: 0
        dct: 1
        qos: 1
        eth_net_offloads: 1
        roce: 1
        atomic: 1
        cq_oi: 1
        cq_resize: 1
        cq_moderation: 1
        cq_eq_remap: 1
        pg: 1
        block_lb_mc: 1
        scqe_break_moderation: 1
        cq_period_start_from_cqe: 1
        cd: 1
        apm: 1
        vector_calc: 1
        umr_ptr_rlky: 1
        imaicl: 1
        qp_packet_based: 0
        qkv: 1
        pkv: 1
        set_deth_sqpn: 1
        xrc: 1
        ud: 1
        uc: 1
        rc: 1
        uar_4k: 0
        uar_sz: 5
        port_selection_cap: 1
        umem_uid_0: 1
        log_pg_sz: 12
        bf: 1
        driver_version: 1
        pad_tx_eth_packet: 1
        mkey_by_name: 0
        log_bf_reg_size: 9
        lag_dct: 1
        lag_tx_port_affinity: 1
        lag_native_fdb_selection: 1
        lag_master: 1
        num_lag_ports: 2
        num_of_diagnostic_counters: 175
        max_wqe_sz_sq: 1024
        max_wqe_sz_rq: 512
        max_flow_counter_31_16: 128
        max_wqe_sz_sq_dc: 256
        max_qp_mcg: 240
        flow_counter_bulk_alloc: 4
        log_max_mcg: 21
        log_max_transport_domain: 16
        log_max_pd: 23
        log_max_xrcd: 24
        nic_receive_steering_discard: 1
        receive_discard_vport_down: 1
        transmit_discard_vport_down: 1
        eq_overrun_count: 1
        invalid_command_count: 1
        quota_exceeded_count: 1
        log_max_flow_counter_bulk: 23
        max_flow_counter_15_0: 65344
        log_max_rq: 23
        log_max_sq: 23
        log_max_tir: 16
        log_max_tis: 23
        basic_cyclic_rcv_wqe: 1
        log_max_rmp: 23
        log_max_rqt: 20
        mini_cqe_resp_stride_index: 1
        cqe_128_always: 1
        cqe_compression_128: 1
        cqe_compression: 1
        cqe_compression_timeout: 2
        cqe_compression_max_num: 64
        flex_parser_id_gtpu_dw_0: 1
        flex_parser_id_icmp_dw1: 6
        flex_parser_id_icmp_dw0: 5
        flex_parser_id_icmpv6_dw1: 6
        flex_parser_id_icmpv6_dw0: 5
        flex_parser_id_outer_first_mpls_over_gre: 0
        flex_parser_id_outer_first_mpls_over_udp_label: 0
        max_num_match_definer: 231
        sf_base_id: 0
        flex_parser_id_gtpu_dw_2: 2
        flex_parser_id_gtpu_first_ext_dw_0: 3
        num_total_dynamic_vf_msix: 0
        dynamic_msix_table_size: 0
        min_dynamic_vf_msix_table_size: 0
        max_dynamic_vf_msix_table_size: 0
        vhca_tunnel_commands: 0x0
        match_definer_format_supported: 0x80000003ffc9e7ff
MLX5_CAP_GENERAL_2:
        max_reformat_insert_size: 128
        max_reformat_insert_offset: 254
        max_reformat_remove_size: 128
        max_reformat_remove_offset: 254
        log_min_mkey_entity_size: 0
        sw_vhca_id_valid: 0
        sw_vhca_id: 0
        ts_cqe_metadata_size2wqe_counter: 0
MLX5_CAP_ETHERNET_OFFLOADS:
        csum_cap: 1
        vlan_cap: 1
        lro_cap: 1
        lro_psh_flag: 0
        lro_time_stamp: 1
        wqe_vlan_insert: 0
        self_lb_en_modifiable: 1
        max_lso_cap: 18
        multi_pkt_send_wqe: 1
        wqe_inline_mode: 2
        rss_ind_tbl_cap: 11
        reg_umr_sq: 1
        scatter_fcs: 1
        enhanced_multi_pkt_send_wqe: 1
        tunnel_lso_const_out_ip_id: 0
        tunnel_lro_gre: 0
        tunnel_lro_vxlan: 0
        tunnel_stateless_gre: 1
        tunnel_stateless_vxlan: 1
        swp: 1
        swp_csum: 1
        swp_lso: 1
        cqe_checksum_full: 0
        tunnel_stateless_geneve_tx: 1
        tunnel_stateless_mpls_over_udp: 0
        tunnel_stateless_mpls_over_gre: 0
        tunnel_stateless_vxlan_gpe: 1
        tunnel_stateless_ipv4_over_vxlan: 1
        tunnel_stateless_ip_over_ip: 1
        insert_trailer: 1
        tunnel_stateless_ip_over_ip_rx: 1
        tunnel_stateless_ip_over_ip_tx: 1
        max_vxlan_udp_ports: 0
        max_geneve_opt_len: 1
        tunnel_stateless_geneve_rx: 1
        lro_min_mss_size: 1348
        lro_timer_supported_periods[0]: 8
        lro_timer_supported_periods[1]: 16
        lro_timer_supported_periods[2]: 32
        lro_timer_supported_periods[3]: 1024
        enhanced_multi_pkt_send_wqe: 1
MLX5_CAP_VDPA_EMULATION:
        desc_tunnel_offload_type: 0
        eth_frame_offload_type: 1
        virtio_version_1_0: 1
        device_features_bits_mask: 7680
        event_mode: 3
        virtio_queue_type: 1
        max_tunnel_desc: 0
        log_doorbell_stride: 12
        log_doorbell_bar_size: 0
        doorbell_bar_offset: 12288
        max_emulated_devices: 0
        max_num_virtio_queues: 256
        umem_1_buffer_param_a: 128
        umem_1_buffer_param_b: 4168
        umem_2_buffer_param_a: 128
        umem_2_buffer_param_b: 8200
        umem_3_buffer_param_a: 32
        umem_3_buffer_param_b: 16392
MLX5_CAP_ROCE:
        roce_apm: 0
        sw_r_roce_src_udp_port: 1
        fl_rc_qp_when_roce_disabled: 1
        fl_rc_qp_when_roce_enabled: 1
        qp_ts_format: 0
        l3_type: 7
        roce_version: 5
        r_roce_dest_udp_port: 4791
        r_roce_max_src_udp_port: 65535
        r_roce_min_src_udp_port: 49152
        roce_address_table_size: 255
MLX5_CAP_DEBUG:
        core_dump_general: 1
        core_dump_qp: 1
        log_cr_dump_to_mem_size: 8
        resource_dump: 1
        log_min_resource_dump_eq: 4
        log_max_samples: 14
        single: 1
        repetitive: 1
        health_mon_rx_activity: 1
        diag_counter_tracer_dump: 1
        log_min_sample_period: 0
MLX5_CAP_PORT_SELECTION:
        port_select_flow_table: 1
        port_select_flow_table_bypass: 0
```

#### Register Dump
```bash
$ mlx5ctl mlx5_core.ctl.0 reg --help
mlx5ctl <device> reg --id=<reg_id> [--port=port] [--argument=argument]
        --id=<reg_id> - register id in hex or decimal
        --port=<port> - port number, default 1
        --argument=<argument> - register argument, default 0
        --bin - print register in binary format
        --hex - print register in hex format
        --pretty - print register in pretty format
        --help - print this help

Known Registers:
        PTYS 0x5004 (pretty print) , dump PRM name: ptys_reg
        DTOR 0xc00e (pretty print) , dump PRM name: dtor_reg
        RCR 0xc00f (pretty print) , dump PRM name: rcr_reg
        MCAM 0x907f (pretty print) , dump PRM name: mcam_reg
        NODE_DESC 0x6001 (pretty print) , dump PRM name: node_desc_reg
        QPTS 0x4002 , dump PRM name: qpts_reg
        QTCT 0x400a , dump PRM name: qtct_reg
        QPDPM 0x4013 , dump PRM name: qpdpm_reg
        QCAM 0x4019 , dump PRM name: qcam_reg
        CORE_DUMP 0x402e , dump PRM name: core_dump_reg
        PCAP 0x5001 , dump PRM name: pcap_reg
        PMTU 0x5003 , dump PRM name: pmtu_reg
        PAOS 0x5006 , dump PRM name: paos_reg
        PFCC 0x5007 , dump PRM name: pfcc_reg
        PPCNT 0x5008 , dump PRM name: ppcnt_reg
        PPTB 0x500b , dump PRM name: pptb_reg
        PBMC 0x500c , dump PRM name: pbmc_reg
        PMAOS 0x5012 , dump PRM name: pmaos_reg
        PUDE 0x5009 , dump PRM name: pude_reg
        PMPE 0x5010 , dump PRM name: pmpe_reg
        PELC 0x500e , dump PRM name: pelc_reg
        PVLC 0x500f , dump PRM name: pvlc_reg
        PCMR 0x5041 , dump PRM name: pcmr_reg
        PDDR 0x5031 , dump PRM name: pddr_reg
        PMLP 0x5002 , dump PRM name: pmlp_reg
        PPLM 0x5023 , dump PRM name: pplm_reg
        PCAM 0x507f , dump PRM name: pcam_reg
        MCIA 0x9014 , dump PRM name: mcia_reg
        MFRL 0x9028 , dump PRM name: mfrl_reg
        MLCR 0x902b , dump PRM name: mlcr_reg
        MRTC 0x902d , dump PRM name: mrtc_reg
        MPEIN 0x9050 , dump PRM name: mpein_reg
        MPCNT 0x9051 , dump PRM name: mpcnt_reg
        MTUTC 0x9055 , dump PRM name: mtutc_reg
        MPEGC 0x9056 , dump PRM name: mpegc_reg
        MCQS 0x9060 , dump PRM name: mcqs_reg
        MCQI 0x9061 , dump PRM name: mcqi_reg
        MCC 0x9062 , dump PRM name: mcc_reg
        MCDA 0x9063 , dump PRM name: mcda_reg
        MIRC 0x9162 , dump PRM name: mirc_reg
        SBCAM 0xb01f , dump PRM name: sbcam_reg
        DCBX_PARAM 0x4020 , dump PRM name: dcbx_param_reg
        SBPR 0xb001 , dump PRM name: sbpr_reg
        SBCM 0xb002 , dump PRM name: sbcm_reg
        QETCR 0x4005 , dump PRM name: qetcr_reg
        DCBX_APP 0x4021 , dump PRM name: dcbx_app_reg
        FPGA_CAP 0x4022 , dump PRM name: fpga_cap_reg
        FPGA_CTRL 0x4023 , dump PRM name: fpga_ctrl_reg
        HOST_ENDIANNESS 0x7004 , dump PRM name: host_endianness_reg
        MTCAP 0x9009 , dump PRM name: mtcap_reg
        MTMP 0x900a , dump PRM name: mtmp_reg
        MTRC_CAP 0x9040 , dump PRM name: mtrc_cap_reg
        MTRC_CONF 0x9041 , dump PRM name: mtrc_conf_reg
        MTRC_STDB 0x9042 , dump PRM name: mtrc_stdb_reg
        MTRC_CTRL 0x9043 , dump PRM name: mtrc_ctrl_reg
        MTPPS 0x9053 , dump PRM name: mtpps_reg
        MTPPSE 0x9054 , dump PRM name: mtppse_reg
        RESOURCE_DUMP 0xc000 , dump PRM name: resource_dump_reg
        FPGA_ACCESS_REG 0x4024 , dump PRM name: fpga_access_reg_reg
```

##### Example 1: Dump NODE_DESC register
```bash
$ mlx5ctl mlx5_core.ctl.0 reg --id=NODE_DESC -P
INFO : NODE_DESC 0x6001 register fields:
INFO :
NODE_DESC 0x6001 fields:
Node description: MT4125 ConnectX6Dx Mellanox Technologies

```

##### Example 2: Dump DTOR (0xc00e) register
```bash
$ mlx5ctl mlx5_core.ctl.0 reg --id=DTOR
00 00 00 00 20 00 00 02 00 00 00 00 00 00 00 00
00 00 00 00 20 00 00 02 20 00 00 3c 20 00 00 3c
20 00 00 02 20 00 00 02 20 00 00 0a 20 00 00 05
20 00 00 05 20 00 00 78 00 00 00 00 00 00 00 00
...
00 00 00 00 00 00 00 00 00 00 00 00

INFO :
DTOR 0xc00e fields:
        pcie_toggle_to: val 2 mult 1
        full_crdump_to: val 60 mult 1
        fw_reset_to: val 60 mult 1
        flush_on_err_to: val 2 mult 1
        pci_sync_update_to: val 2 mult 1
        tear_down_to: val 10 mult 1
        fsm_reactivate_to: val 5 mult 1
        reclaim_pages_to: val 5 mult 1
        reclaim_vfs_pages_to: val 120 mult 1
```

##### Example 3: Dump PPCNT (Physical Port counters register) and parse using parseadb
```bash
$ mlx5ctl mlx5_core.ctl.0 reg --id=PPCNT --port=1 -B | parseadb ppcnt_reg
<huge output of all counter sets of PPCNT> :-)
```

#### Object dump
```bash
$ mlx5ctl mlx5_core.ctl.0 obj --help
Usage: mlx5ctl <device> obj <obj_name> --id=<obj_id> [--op_mod=op_mod] [--bin]
executes PRM command query_<obj_name>_in
hex dumps query_<obj_name>_out, unless [--bin|-B], then binary dump
Supported obj_names:
        eq --id=eqn, dump PRM name: query_eq_out
        cq --id=cqn, dump PRM name: query_cq_out
        qp --id=qpn, dump PRM name: query_qp_out
        sq --id=sqn, dump PRM name: query_sq_out
        rq --id=rqn, dump PRM name: query_rq_out
        tis --id=tisn, dump PRM name: query_tis_out
        tir --id=tirn, dump PRM name: query_tir_out
        rqt --id=rqtn, dump PRM name: query_rqt_out
        rmp --id=rmpn, dump PRM name: query_rmp_out
        dct --id=dctn, dump PRM name: query_dct_out
        srq --id=srqn, dump PRM name: query_srq_out
        xrq --id=xrqn, dump PRM name: query_xrq_out
        xrc_srq --id=xrc_srqn, dump PRM name: query_xrc_srq_out
        q_counter --id=counter_set_id, dump PRM name: query_q_counter_out
        mkey --id=mkey_index, dump PRM name: query_mkey_out
        pages --id=function_id, dump PRM name: query_pages_out
        l2_table_entry --id=table_index, dump PRM name: query_l2_table_entry_out
        issi --op_mod=op_mod, dump PRM name: query_issi_out
        vport_state --id=vport_number (other_vport=1), dump PRM name: query_vport_state_out
        esw_vport_context --id=vport_number (other_vport=1), dump PRM name: query_esw_vport_context_out
        vport_counter --id=vport_number (other_vport=1), dump PRM name: query_vport_counter_out
        vnic_env --op_mod=op_mod, dump PRM name: query_vnic_env_out
        packet_reformat_context --id=packet_reformat_id, dump PRM name: query_packet_reformat_context_out
        special_contexts --op_mod=op_mod, dump PRM name: query_special_contexts_out
        mad_demux --op_mod=op_mod, dump PRM name: query_mad_demux_out
        cong_statistics --op_mod=op_mod, dump PRM name: query_cong_statistics_out
        cong_params --id=cong_protocol, dump PRM name: query_cong_params_out
        cong_status --id=priority:cong_protocol, dump PRM name: query_cong_status_out
        adapter --op_mod=op_mod, dump PRM name: query_adapter_out
        wol_rol --op_mod=op_mod, dump PRM name: query_wol_rol_out
        lag --op_mod=op_mod, dump PRM name: query_lag_out
        esw_functions --op_mod=op_mod, dump PRM name: query_esw_functions_out
        vhca_migration_state --id=vhca_id, dump PRM name: query_vhca_migration_state_out
```

##### Example: Dump CQ object (parse using parseadb)
```bash
$ mlx5ctl mlx5_core.ctl.0 obj cq --id=1135 -B | parseadb query_cq_out
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 09 00 00 00 00 00 00 00 00 00 0a 00 00 80
00 08 00 80 00 00 00 16 00 00 00 00 00 00 00 00
00 ff ff ff 00 ff ff ff 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 01 1b b1 e6 80
...
Node: query_cq_out
        status: 0x0 (0)
        syndrome: 0x0 (0)
        cq_context:
                Node: cqc
                        status: 0x0 (0) OK=0x0
                        as_notify: 0x0 (0)
                        initiator_src_dct: 0x0 (0)
                        dbr_umem_valid: 0x0 (0)
                        ext_element: 0x0 (0)
                        cqe_sz: 0x0 (0) BYTES_64=0x0
                        cc: 0x0 (0)
                        scqe_break_moderation_en: 0x0 (0)
                        oi: 0x0 (0)
                        cq_period_mode: 0x0 (0) upon_event=0x0
                        cqe_compression_en: 0x0 (0)
                        mini_cqe_res_format_1_0: 0x0 (0)
                        st: 0x9 (9) NOTIFICATION_REQUEST_ARMED=0x9
                        always_armed_cq: 0x0 (0)
                        ext_element_type: 0x0 (0) DPA_THREAD=0x0
                        cqe_compression_layout: 0x0 (0) BASIC_CQE_COMPRESSION=0x0
                        dbr_umem_id: 0x0 (0)
                        page_offset: 0x0 (0)
                        mini_cqe_res_format_3_2: 0x0 (0)
                        cq_time_stamp_format: 0x0 (0) INTERNAL_TIMER=0x0
                        log_cq_size: 0xa (10)
                        uar_page: 0x80 (128)
                        cq_period: 0x8 (8)
                        cq_max_count: 0x80 (128)
                        c_eqn_or_ext_element: 0x16 (22)
                        log_page_size: 0x0 (0)
                        last_notified_index: 0xffffff (16777215)
                        last_solicit_index: 0xffffff (16777215)
                        consumer_counter: 0x0 (0)
                        producer_counter: 0x0 (0)
                        as_notify_params:
                                Node: as_notify_params
                                        local_partition_id: 0x0 (0)
                                        process_id: 0x0 (0)
                                        thread_id: 0x0 (0)
                        dbr_addr: 0x1bb1e680 (464643712)
        e_mtt_pointer_or_cq_umem_offset:
                Node: cmd_e_mtt_pointer
                        mtt_index_h: 0x0 (0)
                        mtt_index_l: 0x0 (0)
        cq_umem_id: 0x0 (0)
        cq_umem_valid: 0x0 (0)
        pas:
                Node: cmd_pas
                        pa_h: 0x0 (0)
                        pa_l: 0x0 (0)
```

#### Diagnostic counters
periodic sampling of diagnostic counters, the tool will provide the commands
to enabling sampling on selected counters and dumping the samples on demand.
```bash
# Show help and list of supported counters to be enabled
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt help
Usage: diagcnt <command> [options]
Commands:
        cap: show diag counters cap
        help: show this help
        set: set param
        disable: disable diag counters
        param: query param
        dump: dump samples
```

##### Diagnostic counters capabilities
```bash
# Show capabilities and list of supported counters to be enabled
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt cap
diag counters:
        num_of_diagnostic_counters: 18
        single: 1
        repetitive: 1
        log_max_samples: 14
        log_min_sample_period: 0
        dev_freq: 1000000 kHz
        counter[0]: 0x401 sync(1)
        counter[1]: 0x402 sync(1)
        counter[2]: 0x403 sync(1)
        counter[4]: 0x406 sync(1)
        counter[9]: 0x409 sync(1)
        counter[10]: 0x40a sync(1)
        counter[11]: 0x40b sync(1)
        counter[28]: 0x1001 sync(1)
        counter[68]: 0x2001 sync(1)
        counter[73]: 0x2006 sync(1)
        counter[88]: 0x2c02 sync(1)
        counter[89]: 0x2c03 sync(1)
        counter[90]: 0x2c04 sync(1)
```

##### Diagnostic counters set
enable selected counters to start sampling
```bash
# Show capabilities and list of supported counters to be enabled
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt set help
Usage: set [flags] <log num of samples> <sample period> <counter id1>,<counter id2>...
        flags: -Scsro = (S)ync (c)lear (s)ingle (r)epetitive (o)n_demand

# example: check if there's a back pressure from data link on TX
# Enable monitoring/sampling of the following counters:
#  - Back Pressure from Datalink to Transport Unit
#  - TX Packets and Line Transmitted Port 1
# arguments:
#     -cSs: start from clear, Sync and sample for single round
#     4: 2^4 samples
#     10: 2^10 sample periods in device clocks
#     0x0401,0x2006,0x040b: list of counters to sample
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt set -cSs 4 10 0x0401,0x2006,0x040b
setting params:
        single: 1 (0x1)
        repetitive: 0 (0x0)
        sync: 1
        clear: 1
        on_demand: 0
        enable: 1
        num_of_counters: 3 (0x3)
        log_num_of_samples: 4 (0x4)
        log_sample_period: 10 (0xa)
        sample_period: 1 μs
        dev_freq: 1000000 kHz
        counter_id:
                [0] = 1025 (0x401)
                [1] = 8198 (0x2006)
                [2] = 1035 (0x40b)
sampling started..
```

##### Diagnostic counters dump
Dump the currently enabled counters sampling
```bash
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt dump help
Usage: dump [num samples] [sample index]

# exmaple dump 6 samples starting sample index 4
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt dump 6 4
query diag counters: 6 samples 4 sample_index
counter_id: 0x0401, sample_id: 0000000005, time_stamp: 2692409595 counter_value: 0
counter_id: 0x2006, sample_id: 0000000005, time_stamp: 2692409595 counter_value: 0
counter_id: 0x040b, sample_id: 0000000005, time_stamp: 2692409595 counter_value: 0
counter_id: 0x0401, sample_id: 0000000006, time_stamp: 2692502841 counter_value: 0
counter_id: 0x2006, sample_id: 0000000006, time_stamp: 2692502841 counter_value: 0
counter_id: 0x040b, sample_id: 0000000006, time_stamp: 2692502841 counter_value: 0
```

##### Diagnostic query param
Query the currently set parameters from the latest set command
```bash
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt param 3
query diag counter params with 3 counters
diag params:
        sync: 1
        clear: 1
        enable: 1
        single: 1
        repetitive: 0
        on_demand: 0
        log_num_of_samples: 4
        log_sample_period: 10
        counter[0]: 0x401
        counter[1]: 0x2006
        counter[2]: 0x40b
```

##### disable sampling
```bash
$ sudo mlx5ctl mlx5_core.ctl.0 diagcnt disable
disabling diag counters and clearing HW buffer
```

#### Core dump
Dump internal debug information

Core dump needs a lot of memory space and the command only supports
umem mode (see below) so the dump can go efficiently into userspace
memory directly from the device.
The recommended memory size to use is 2MB to collect all of the core dump

```bash
# usage mlx5ctl mlx5_core.ctl.0 coredump [--umem=<size KB>] [--help]

$ sudo mlx5ctl mlx5_core.ctl.0 coredump --umem=2000
00 00 00 00 01 00 20 00 00 00 00 04 00 00 48 ec
00 00 00 08 00 00 00 00 00 00 00 0c 00 00 00 03
00 00 00 10 00 00 00 00 00 00 00 14 00 00 00 00
00 00 00 18 54 30 6f 6c 00 00 00 1c 00 10 40 00
...
00 50 0b 14 00 00 00 00 00 50 0b 18 00 00 00 00
00 50 0b 1c 00 00 00 00 00 50 0b 20 00 00 00 00
00 50 0b 24 00 00 00 00 00 50 0b 28 00 00 00 00
00 50 0b 2c 00 00 00 00 00 50 0b 30 00 00 00 00
00 50 0b 34 00 00 00 00 00 50 0b 38 00 00 00 00
00 50 0b 3c 00 00 00 00 00 50 0b 40 00 00 00 00
00 50 0b 44 00 00 00 00 00 50 0b 48 00 00 00 00
00 50 0c 00 00 00 00 00
INFO : Core dump done
INFO : Core dump size 831304
INFO : Core dump address 0x0
INFO : Core dump cookie 0x500c04
INFO : More Dump 0
```

#### Resource dump
Dump internal objects and resources by name/id/type.

```bash
$ sudo mlx5ctl mlx5_core.ctl.0 rscdump --help
Usage: rscdump [--umem=<size KB>] [--type=<type>] [--idx1=<index1>] [--idx2=<index2>] [--vhcaid=<vhca_id>] [--help]

# Menu of objects/segments that can be dumpped
$ sudo mlx5ctl mlx5_core.ctl.0 rscdump
INFO : Resource dump menu size 2388 num of records 45
Menu Record 0
        segment_type: 0x2
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: HW_CQPC
        index1_name: QPN
        index2_name: N/A
Menu Record 1
        segment_type: 0x3
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: HW_SQPC
        index1_name: QPN
        index2_name: N/A
Menu Record 2
        segment_type: 0x4
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: HW_RQPC
        index1_name: QPN
        index2_name: N/A
Menu Record 3
        segment_type: 0x1100
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FULL_SRQC
        index1_name: SRQN
        index2_name: N/A
Menu Record 4
        segment_type: 0x1200
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FULL_CQC
        index1_name: CQN
        index2_name: N/A
Menu Record 5
        segment_type: 0x1300
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FULL_EQC
        index1_name: EQN
        index2_name: N/A
Menu Record 6
        segment_type: 0x1000
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FULL_QPC
        index1_name: QPN
        index2_name: N/A
Menu Record 7
        segment_type: 0x1001
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 1
        num_of_obj2_supports_active: 0
        segment_name: SND_BUFF
        index1_name: QPN
        index2_name: WQEBB
Menu Record 8
        segment_type: 0x1002
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 1
        num_of_obj2_supports_active: 0
        segment_name: RCV_BUFF
        index1_name: QPN
        index2_name: DS
Menu Record 9
        segment_type: 0x1101
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 1
        num_of_obj2_supports_active: 0
        segment_name: SRQ_BUFF
        index1_name: SRQN
        index2_name: DS
Menu Record 10
        segment_type: 0x1201
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 1
        num_of_obj2_supports_active: 0
        segment_name: CQ_BUFF
        index1_name: CQN
        index2_name: CQE
Menu Record 11
        segment_type: 0x1301
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 1
        num_of_obj2_supports_active: 0
        segment_name: EQ_BUFF
        index1_name: EQN
        index2_name: EQE
Menu Record 12
        segment_type: 0x3000
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: SX_SLICE
        index1_name: SLICE
        index2_name: N/A
Menu Record 13
        segment_type: 0x3001
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: SX_SLICE_ALL
        index1_name: N/A
        index2_name: N/A
Menu Record 14
        segment_type: 0x2000
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: RX_SLICE
        index1_name: SLICE
        index2_name: N/A
Menu Record 15
        segment_type: 0x2001
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: RX_SLICE_ALL
        index1_name: N/A
        index2_name: N/A
Menu Record 16
        segment_type: 0x21
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: MKEY_CONTEXT
        index1_name: MKEY_IX
        index2_name: N/A
Menu Record 17
        segment_type: 0x5
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 1
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: RDB_CONTEXT
        index1_name: QPN
        index2_name: RDB_IX
Menu Record 18
        segment_type: 0x1310
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: STE_RANGE
        index1_name: STE_IX
        index2_name: N/A
Menu Record 19
        segment_type: 0x1311
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: STE_ALL
        index1_name: N/A
        index2_name: N/A
Menu Record 20
        segment_type: 0x1010
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FUNC_SYND_REC
        index1_name: N/A
        index2_name: N/A
Menu Record 21
        segment_type: 0x1312
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: STE_GLOBAL_RNG
        index1_name: STE_IX
        index2_name: N/A
Menu Record 22
        segment_type: 0x1313
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: STE_ROOT
        index1_name: ROOT_IX
        index2_name: N/A
Menu Record 23
        segment_type: 0x2020
        support_index1: 1
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 1
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: ICMC_INFO
        index1_name: SET
        index2_name: N/A
Menu Record 24
        segment_type: 0x4000
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PRM_QUERY_QP
        index1_name: QPN
        index2_name: N/A
Menu Record 25
        segment_type: 0x4020
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PRM_QUERY_CQ
        index1_name: CQN
        index2_name: N/A
Menu Record 26
        segment_type: 0x4030
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PRM_QUERY_MKEY
        index1_name: MKEY_IDX
        index2_name: N/A
Menu Record 27
        segment_type: 0x4008
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_VNIC_ENV
        index1_name: N/A
        index2_name: N/A
Menu Record 28
        segment_type: 0x1020
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 0
        support_num_of_obj2: 1
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: MDFY_HDR_RNG
        index1_name: MDFY_IX
        index2_name: N/A
Menu Record 29
        segment_type: 0x1021
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: MDFY_HDR_ALL
        index1_name: N/A
        index2_name: N/A
Menu Record 30
        segment_type: 0x4010
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FT
        index1_name: TABLE_ID
        index2_name: N/A
Menu Record 31
        segment_type: 0x4011
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FT_ALL
        index1_name: N/A
        index2_name: N/A
Menu Record 32
        segment_type: 0x4012
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 1
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FG
        index1_name: TABLE_ID
        index2_name: GROUP_ID
Menu Record 33
        segment_type: 0x4013
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FG_ALL
        index1_name: TABLE_ID
        index2_name: N/A
Menu Record 34
        segment_type: 0x4014
        support_index1: 1
        must_have_index1: 1
        support_index2: 1
        must_have_index2: 1
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FTE
        index1_name: TABLE_ID
        index2_name: FLOW_IX
Menu Record 35
        segment_type: 0x4015
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: QUERY_FTE_ALL
        index1_name: TABLE_ID
        index2_name: N/A
Menu Record 36
        segment_type: 0x1318
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: FLOW_COUNTER
        index1_name: CNTR ID
        index2_name: N/A
Menu Record 37
        segment_type: 0x1320
        support_index1: 1
        must_have_index1: 1
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PKT_REFORMAT
        index1_name: CTX ID
        index2_name: N/A
Menu Record 38
        segment_type: 0x1030
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: SHARED_BUFFER
        index1_name: N/A
        index2_name: N/A
Menu Record 39
        segment_type: 0x103c
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PAGE_HISTOGRAM
        index1_name: N/A
        index2_name: N/A
Menu Record 40
        segment_type: 0x1040
        support_index1: 1
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 1
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: SCHEDQ_INFO
        index1_name: SCHEDQ_IX
        index2_name: N/A
Menu Record 41
        segment_type: 0x104b
        support_index1: 1
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: VQOS_TREE
        index1_name: SKIP_LOCK
        index2_name: N/A
Menu Record 42
        segment_type: 0x1050
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: VQOS_DISTLIST
        index1_name: N/A
        index2_name: N/A
Menu Record 43
        segment_type: 0x1048
        support_index1: 0
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 0
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 0
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: VLQ_GVMI_VLQ_SQ
        index1_name: N/A
        index2_name: N/A
Menu Record 44
        segment_type: 0x1053
        support_index1: 1
        must_have_index1: 0
        support_index2: 0
        must_have_index2: 0
        support_num_of_obj1: 1
        must_have_num_of_obj1: 0
        support_num_of_obj2: 0
        must_have_num_of_obj2: 0
        num_of_obj1_supports_all: 0
        num_of_obj1_supports_active: 1
        num_of_obj2_supports_all: 0
        num_of_obj2_supports_active: 0
        segment_name: PACKET_PACING
        index1_name: PP_IX
        index2_name: N/A
```

##### Umem mode

Both core dump and rscdump support umem mode, if enabled it will allow the dumps
to go directly into userspace buffers, via mlx5ctl umem interface, since those dumps
can be very large, several MBytes, it is highly recommended to use umem mode for such
commands on critical debug.

#### Future work
Note: Check PRM for the following topics
 - umem mode for diag counters
 - Query objects contexts and WQE/CQE/EQE buffers (QP, CQ, EQ)
 - HW tracer
 - FW tracer
 - Health Monitoring
 - More register pretty print support
 - Query nvinfo

#### License
mlx5ctl tools is released under the BSD 3-Clause License. See the [LICENSE](LICENSE) file for details.
