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

- List of currently enabled mlx5ctl devices
- info: Provide information on the current allocated UID
- devcap: Dump all device capabilities
- reg: Dump ConnectX registers by ID with pretty print
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
$ make install

$ mlx5ctl
Usage: mlx5ctl <mlx5ctl device> <command> [options]
Verbosity: mlx5ctl -v <mlx5ctl device> <command> [options]
Commands:
        info: Print device information
        devcap: Query FW and show some device caps
        reg: Dump access registers
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

to enable verbosity:
`mlx5ctl -v <command> [option]`

```bash
$ mlx5ctl
Usage: mlx5ctl <mlx5ctl device> <command> [options]
Verbosity: mlx5ctl -v <mlx5ctl device> <command> [options]
Commands:
        info: Print device information
        devcap: Query FW and show some device caps
        reg: Dump access registers
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
$ mlx5ctl list
Found 2 mlx5ctl devices:
/dev/mlx5ctl-mlx5_core.ctl.0
/dev/mlx5ctl-mlx5_core.ctl.1
```

#### Device Info
Get specific device info and debug UID info
```bash
$ mlx5ctl mlx5_core.ctl.0
mlx5dev: 0000:00:04.0
UCTX UID: 1
UCTX CAP: 0x3
DEV UCTX CAP: 0x3
USER CAP: 0x1d
Current PID: 778 FD 3
```

#### Device capabilities
```bash
$ mlx5ctl mlx5_core.ctl.0 devcap

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
# reg help menu and the list of current supported REG IDs and names
$ mlx5ctl mlx5_core.ctl.0 reg help
usage: mlx5ctl <device> reg <reg_id> <port> <argument>
QPTS 0x4002
QETCR 0x4005
QTCT 0x400a
QPDPM 0x4013
QCAM 0x4019
DCBX_PARAM 0x4020
DCBX_APP 0x4021
FPGA_CAP 0x4022
FPGA_CTRL 0x4023
FPGA_ACCESS_REG 0x4024
CORE_DUMP 0x402e
PCAP 0x5001
PMLP 0x5002
PMTU 0x5003
PTYS 0x5004 [PP]
PAOS 0x5006
PFCC 0x5007
PPCNT 0x5008
PUDE 0x5009
PPTB 0x500b
PBMC 0x500c
PELC 0x500e
PVLC 0x500f
PMPE 0x5010
PMAOS 0x5012
PPLM 0x5023
PDDR 0x5031
PCMR 0x5041
PCAM 0x507f
NODE_DESC 0x6001 [PP]
HOST_ENDIANNESS 0x7004
MTCAP 0x9009
MTMP 0x900a
MCIA 0x9014
MFRL 0x9028
MLCR 0x902b
MRTC 0x902d
MTRC_CAP 0x9040
MTRC_CONF 0x9041
MTRC_STDB 0x9042
MTRC_CTRL 0x9043
MPEIN 0x9050
MPCNT 0x9051
MTPPS 0x9053
MTPPSE 0x9054
MTUTC 0x9055
MPEGC 0x9056
MCQS 0x9060
MCQI 0x9061
MCC 0x9062
MCDA 0x9063
MCAM 0x907f [PP]
MIRC 0x9162
SBPR 0xb001
SBCM 0xb002
SBCAM 0xb01f
RESOURCE_DUMP 0xc000
DTOR 0xc00e [PP]
RCR 0xc00f [PP]
```
##### Example 1: Dump NODE_DESC register
```bash
$ mlx5ctl mlx5_core.ctl.0 reg NODE_DESC
INFO : dumping register NODE_DESC 0x6001 local_port 1 argumet 0x0
INFO : NODE_DESC 0x6001 register fields:
4d 54 34 31 32 35 20 43 6f 6e 6e 65 63 74 58 36
44 78 20 4d 65 6c 6c 61 6e 6f 78 20 54 65 63 68
6e 6f 6c 6f 67 69 65 73 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
...
00 00 00 00 00 00 00 00 00 00 00 00
INFO :
NODE_DESC 0x6001 fields:
Node description: MT4125 ConnectX6Dx Mellanox Technologies
```

##### Example 2: Dump DTOR (0xc00e) register
```bash
$ mlx5ctl mlx5_core.ctl.0 reg DTOR
INFO : dumping register DTOR 0xc00e local_port 1 argumet 0x0
INFO : DTOR 0xc00e register fields:
00 00 00 00 20 00 00 02 00 00 00 00 00 00 00 00
00 00 00 00 20 00 00 02 20 00 00 3c 20 00 00 3c
20 00 00 02 20 00 00 02 20 00 00 0a 20 00 00 05
20 00 00 05 20 00 00 78 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
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

#### Diagnostic counters
periodic sampling of diagnostic counters, the tool will provide the commands
to enabling sampling on selected counters and dumping the samples on demand.
```bash
# Show help and list of supported counters to be enabled
$ mlx5ctl mlx5_core.ctl.0 diagcnt help
Usage: help <command> [options]
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
$ mlx5ctl mlx5_core.ctl.0 diagcnt cap
diag counters:
        num_of_diagnostic_counters: 18
        single: 1
        repetitive: 1
        log_max_samples: 14
        log_min_sample_period: 0
        counter[0]: 0x401 sync(1) Back Pressure from Datalink to Transport Unit
        counter[1]: 0x402 sync(1) Outbound Stalled Writes
        counter[2]: 0x403 sync(1) Outbound Stalled Reads
        counter[4]: 0x406 sync(1) RX 128B Data
        counter[9]: 0x409 sync(1) PCI Read Stalled due to No Read Engines
        counter[10]: 0x40a sync(1) PCI Read Stalled due to No Completion Buffer
        counter[11]: 0x40b sync(1) PCI Read Stalled due to Ordering
        counter[28]: 0x1001 sync(1) RX Steering Packets
        counter[68]: 0x2001 sync(1) Send Queue Stopped due to Limited VL
        counter[73]: 0x2006 sync(1) TX Packets
        counter[88]: 0x2c02 sync(1) Line Transmitted Port 1
        counter[89]: 0x2c03 sync(1) Line Transmitted Port 2
        counter[90]: 0x2c04 sync(1) Line Transmitted Loop Back
```

##### Diagnostic counters set
enable selected counters to start sampling
```bash
# Show capabilities and list of supported counters to be enabled
$ mlx5ctl mlx5_core.ctl.0 diagcnt set help
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
#     3: counters to sample
#     0x0401,0x2006,0x040b: list of counters to sample
$ mlx5ctl mlx5_core.ctl.0 diagcnt set -cSs 4 10 3 0x0401,0x2006,0x040b
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
        sample_period: 1 ms
        dev_freq: 1000000 khz
        counter_id:
                [0] = 1025 (0x401)
                [1] = 8198 (0x2006)
                [2] = 1035 (0x40b)
sampling started..
```

##### Diagnostic counters dump
Dump the currently enabled counters sampling
```bash
$ mlx5ctl mlx5_core.ctl.0 diagcnt dump help
Usage: dump [num samples] [sample index]

# exmaple dump 6 samples starting sample index 4
$ mlx5ctl mlx5_core.ctl.0 diagcnt dump 6 4
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
$ mlx5ctl mlx5_core.ctl.0  diagcnt param 3
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
$ mlx5ctl mlx5_core.ctl.0 diagcnt disable
disabling diag counters and clearing HW buffer
```

#### Core dump
Dump internal debug information

Core dump needs a lot of memory space and the command only supports
umem mode (see below) so the dump can go efficiently into userspace
memory directly from the device.
The recommended memory size to use is 2MB to collect all of the core dump

```bash
# usage mlx5ctl mlx5_core.ctl.0 coredump [umem size]

$ mlx5ctl mlx5_core.ctl.0 coredump 2000000
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
$ mlx5ctl mlx5_core.ctl.0 rscdump help
Usage: help <command> [options]
Commands:
        menu: show which objects/segments are supported and thier requirements
        help: show this help
        rscdump [object_name|object_type] [index1] [index2] [num objects] [umem size]
            index1: the start object id to dump
            index2: Sub-object ID of the object at index1 (use na if not needed)
            num objects: un objects to dump starting from index1/index2
            umem size: if set use umem mode, see section below

# Menu of objects/segments that can be dumpped
$ mlx5ctl mlx5_core.ctl.0 rscdump menu
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
