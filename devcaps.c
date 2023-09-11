// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"

enum mlx5_cap_type {
	MLX5_CAP_GENERAL = 0,
	MLX5_CAP_ETHERNET_OFFLOADS,
	MLX5_CAP_ODP,
	MLX5_CAP_ATOMIC,
	MLX5_CAP_ROCE,
	MLX5_CAP_IPOIB_OFFLOADS,
	MLX5_CAP_IPOIB_ENHANCED_OFFLOADS,
	MLX5_CAP_FLOW_TABLE,
	MLX5_CAP_ESWITCH_FLOW_TABLE,
	MLX5_CAP_ESWITCH,
	MLX5_CAP_QOS = 0xc,
	MLX5_CAP_DEBUG,
	MLX5_CAP_RESERVED_14,
	MLX5_CAP_DEV_MEM,
	MLX5_CAP_RESERVED_16,
	MLX5_CAP_TLS,
	MLX5_CAP_VDPA_EMULATION = 0x13,
	MLX5_CAP_DEV_EVENT = 0x14,
	MLX5_CAP_IPSEC,
	MLX5_CAP_CRYPTO = 0x1a,
	MLX5_CAP_MACSEC = 0x1f,
	MLX5_CAP_GENERAL_2 = 0x20,
	MLX5_CAP_PORT_SELECTION = 0x25,
	MLX5_CAP_ADV_VIRTUALIZATION = 0x26,
	/* NUM OF CAP Types */
	MLX5_CAP_NUM
};

enum mlx5_cap_mode {
	HCA_CAP_OPMOD_GET_MAX	= 0,
	HCA_CAP_OPMOD_GET_CUR	= 1,
};

void *query_caps(struct mlx5u_dev *dev, u16 opmod, void *out)
{
	int out_sz = MLX5_ST_SZ_BYTES(query_hca_cap_out);
	u8 in[MLX5_ST_SZ_BYTES(query_hca_cap_in)] = {};
	int err;

	MLX5_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
	MLX5_SET(query_hca_cap_in, in, op_mod, opmod);
	err = mlx5u_cmd(dev, in, sizeof(in), out, out_sz);
	if (err) {
		printf("MLX5_CMD_OP_QUERY_HCA_CAP opmod 0x%x failed: %d\n", opmod, err);
		return NULL;
	}

	if (MLX5_GET(mbox_out, out, status)) {
		printf("command failed opcode 0x%x opmod 0x%x\n",
		       MLX5_GET(mbox_in, in, opcode), MLX5_GET(mbox_in, in, op_mod));
		printf("status: 0x%x\n", MLX5_GET(mbox_out, out, status));
		printf("syndrome: 0x%x\n", MLX5_GET(mbox_out, out, syndrome));
		return NULL;
	}

	return MLX5_ADDR_OF(query_hca_cap_out, out, capability);
}

static void print_hca_caps(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_GENERAL << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *hca_caps = query_caps(dev, opmod, out);

	if (!hca_caps)
		return;

#define MLX5_CAP_GEN(cap) MLX5_GET(cmd_hca_cap, hca_caps, cap)
#define MLX5_CAP_GEN64(cap) MLX5_GET64(cmd_hca_cap, hca_caps, cap)
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_GEN(cap))
#define printcap64(cap) printf("\t" #cap ": 0x%lx\n", MLX5_CAP_GEN64(cap))
	printf("MLX5_CAP_GENERAL:\n");
	printcap(shared_object_to_user_object_allowed);
	printcap(vhca_resource_manager);
	printcap(hca_cap_2);
	printcap(create_lag_when_not_master_up);
	printcap(dtor);
	printcap(event_on_vhca_state_teardown_request);
	printcap(event_on_vhca_state_in_use);
	printcap(event_on_vhca_state_active);
	printcap(event_on_vhca_state_allocated);
	printcap(event_on_vhca_state_invalid);
	printcap(vhca_id);
	printcap(log_max_srq_sz);
	printcap(log_max_qp_sz);
	printcap(event_cap);
	printcap(isolate_vl_tc_new);
	printcap(prio_tag_required);
	printcap(log_max_qp);
	printcap(ece_support);
	printcap(reg_c_preserve);
	printcap(log_max_srq);
	printcap(uplink_follow);
	printcap(ts_cqe_to_dest_cqn);
	printcap(shampo);
	printcap(max_sgl_for_optimized_performance);
	printcap(log_max_cq_sz);
	printcap(relaxed_ordering_write_umr);
	printcap(relaxed_ordering_read_umr);
	printcap(virtio_net_device_emualtion_manager);
	printcap(virtio_blk_device_emualtion_manager);
	printcap(log_max_cq);
	printcap(log_max_eq_sz);
	printcap(relaxed_ordering_write);
	printcap(relaxed_ordering_read);
	printcap(log_max_mkey);
	printcap(dump_fill_mkey);
	printcap(fast_teardown);
	printcap(log_max_eq);
	printcap(max_indirection);
	printcap(fixed_buffer_size);
	printcap(log_max_mrw_sz);
	printcap(force_teardown);
	printcap(log_max_bsf_list_size);
	printcap(umr_extended_translation_offset);
	printcap(null_mkey);
	printcap(log_max_klm_list_size);
	printcap(log_max_ra_req_dc);
	printcap(eth_wqe_too_small);
	printcap(vnic_env_cq_overrun);
	printcap(log_max_ra_res_dc);
	printcap(release_all_pages);
	printcap(must_not_use);
	printcap(roce_accl);
	printcap(log_max_ra_req_qp);
	printcap(log_max_ra_res_qp);
	printcap(end_pad);
	printcap(cc_query_allowed);
	printcap(cc_modify_allowed);
	printcap(start_pad);
	printcap(cache_line_128byte);
	printcap(rts2rts_qp_counters_set_id);
	printcap(vnic_env_int_rq_oob);
	printcap(sbcam_reg);
	printcap(qcam_reg);
	printcap(gid_table_size);
	printcap(out_of_seq_cnt);
	printcap(vport_counters);
	printcap(retransmission_q_counters);
	printcap(debug);
	printcap(modify_rq_counter_set_id);
	printcap(rq_delay_drop);
	printcap(max_qp_cnt);
	printcap(pkey_table_size);
	printcap(vport_group_manager);
	printcap(vhca_group_manager);
	printcap(ib_virt);
	printcap(eth_virt);
	printcap(vnic_env_queue_counters);
	printcap(ets);
	printcap(nic_flow_table);
	printcap(eswitch_manager);
	printcap(device_memory);
	printcap(mcam_reg);
	printcap(pcam_reg);
	printcap(local_ca_ack_delay);
	printcap(port_module_event);
	printcap(enhanced_error_q_counters);
	printcap(ports_check);
	printcap(disable_link_up);
	printcap(beacon_led);
	printcap(port_type);
	printcap(num_ports);
	printcap(pps);
	printcap(pps_modify);
	printcap(log_max_msg);
	printcap(max_tc);
	printcap(temp_warn_event);
	printcap(dcbx);
	printcap(general_notification_event);
	printcap(fpga);
	printcap(rol_s);
	printcap(rol_g);
	printcap(wol_s);
	printcap(wol_g);
	printcap(wol_a);
	printcap(wol_b);
	printcap(wol_m);
	printcap(wol_u);
	printcap(wol_p);
	printcap(stat_rate_support);
	printcap(pci_sync_for_fw_update_event);
	printcap(init2_lag_tx_port_affinity);
	printcap(cqe_version);
	printcap(compact_address_vector);
	printcap(striding_rq);
	printcap(ipoib_enhanced_offloads);
	printcap(ipoib_basic_offloads);
	printcap(repeated_block_disabled);
	printcap(umr_modify_entity_size_disabled);
	printcap(umr_modify_atomic_disabled);
	printcap(umr_indirect_mkey_disabled);
	printcap(umr_fence);
	printcap(dc_req_scat_data_cqe);
	printcap(drain_sigerr);
	printcap(cmdif_checksum);
	printcap(sigerr_cqe);
	printcap(wq_signature);
	printcap(sctr_data_cqe);
	printcap(sho);
	printcap(tph);
	printcap(rf);
	printcap(dct);
	printcap(qos);
	printcap(eth_net_offloads);
	printcap(roce);
	printcap(atomic);
	printcap(cq_oi);
	printcap(cq_resize);
	printcap(cq_moderation);
	printcap(cq_eq_remap);
	printcap(pg);
	printcap(block_lb_mc);
	printcap(scqe_break_moderation);
	printcap(cq_period_start_from_cqe);
	printcap(cd);
	printcap(apm);
	printcap(vector_calc);
	printcap(umr_ptr_rlky);
	printcap(imaicl);
	printcap(qp_packet_based);
	printcap(qkv);
	printcap(pkv);
	printcap(set_deth_sqpn);
	printcap(xrc);
	printcap(ud);
	printcap(uc);
	printcap(rc);
	printcap(uar_4k);
	printcap(uar_sz);
	printcap(port_selection_cap);
	printcap(umem_uid_0);
	printcap(log_pg_sz);
	printcap(bf);
	printcap(driver_version);
	printcap(pad_tx_eth_packet);
	printcap(mkey_by_name);
	printcap(log_bf_reg_size);
	printcap(lag_dct);
	printcap(lag_tx_port_affinity);
	printcap(lag_native_fdb_selection);
	printcap(lag_master);
	printcap(num_lag_ports);
	printcap(num_of_diagnostic_counters);
	printcap(max_wqe_sz_sq);
	printcap(max_wqe_sz_rq);
	printcap(max_flow_counter_31_16);
	printcap(max_wqe_sz_sq_dc);
	printcap(max_qp_mcg);
	printcap(flow_counter_bulk_alloc);
	printcap(log_max_mcg);
	printcap(log_max_transport_domain);
	printcap(log_max_pd);
	printcap(log_max_xrcd);
	printcap(nic_receive_steering_discard);
	printcap(receive_discard_vport_down);
	printcap(transmit_discard_vport_down);
	printcap(eq_overrun_count);
	printcap(invalid_command_count);
	printcap(quota_exceeded_count);
	printcap(log_max_flow_counter_bulk);
	printcap(max_flow_counter_15_0);
	printcap(log_max_rq);
	printcap(log_max_sq);
	printcap(log_max_tir);
	printcap(log_max_tis);
	printcap(basic_cyclic_rcv_wqe);
	printcap(log_max_rmp);
	printcap(log_max_rqt);
	printcap(mini_cqe_resp_stride_index);
	printcap(cqe_128_always);
	printcap(cqe_compression_128);
	printcap(cqe_compression);
	printcap(cqe_compression_timeout);
	printcap(cqe_compression_max_num);
	printcap(flex_parser_id_gtpu_dw_0);
	printcap(flex_parser_id_icmp_dw1);
	printcap(flex_parser_id_icmp_dw0);
	printcap(flex_parser_id_icmpv6_dw1);
	printcap(flex_parser_id_icmpv6_dw0);
	printcap(flex_parser_id_outer_first_mpls_over_gre);
	printcap(flex_parser_id_outer_first_mpls_over_udp_label);
	printcap(max_num_match_definer);
	printcap(sf_base_id);
	printcap(flex_parser_id_gtpu_dw_2);
	printcap(flex_parser_id_gtpu_first_ext_dw_0);
	printcap(num_total_dynamic_vf_msix);
	printcap(dynamic_msix_table_size);
	printcap(min_dynamic_vf_msix_table_size);
	printcap(max_dynamic_vf_msix_table_size);
	printcap64(vhca_tunnel_commands);
	printcap64(match_definer_format_supported);

#undef MLX5_CAP_GEN
}

static void print_hca_caps2(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_GENERAL_2 << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *hca_caps = query_caps(dev, opmod, out);

	if (!hca_caps)
		return;

#define MLX5_CAP_GEN2(cap) MLX5_GET(cmd_hca_cap_2, hca_caps, cap)
#define MLX5_CAP_GEN2_64(cap) MLX5_GET64(cmd_hca_cap_2, hca_caps, cap)
#undef  printcap
#undef  printcap64
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_GEN2(cap))
#define printcap64(cap) printf("\t" #cap ": 0x%lx\n", MLX5_CAP_GEN2_64(cap))
	printf("MLX5_CAP_GENERAL_2:\n");
	printcap(max_reformat_insert_size);
	printcap(max_reformat_insert_offset);
	printcap(max_reformat_remove_size);
	printcap(max_reformat_remove_offset);
	printcap(log_min_mkey_entity_size);
	printcap(sw_vhca_id_valid);
	printcap(sw_vhca_id);
	printcap(ts_cqe_metadata_size2wqe_counter);
#undef MLX5_CAP_GEN2
}


static void print_eth_caps(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_ETHERNET_OFFLOADS << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *eth_caps = query_caps(dev, opmod, out);

	if (!eth_caps)
		return;
#define MLX5_CAP_ETH(cap) \
	MLX5_GET(per_protocol_networking_offload_caps, eth_caps, cap)
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_ETH(cap))
	printf("MLX5_CAP_ETHERNET_OFFLOADS:\n");

	printcap(csum_cap);
	printcap(vlan_cap);
	printcap(lro_cap);
	printcap(lro_psh_flag);
	printcap(lro_time_stamp);
	printcap(wqe_vlan_insert);
	printcap(self_lb_en_modifiable);
	printcap(max_lso_cap);
	printcap(multi_pkt_send_wqe);
	printcap(wqe_inline_mode);
	printcap(rss_ind_tbl_cap);
	printcap(reg_umr_sq);
	printcap(scatter_fcs);
	printcap(enhanced_multi_pkt_send_wqe);
	printcap(tunnel_lso_const_out_ip_id);
	printcap(tunnel_lro_gre);
	printcap(tunnel_lro_vxlan);
	printcap(tunnel_stateless_gre);
	printcap(tunnel_stateless_vxlan);
	printcap(swp);
	printcap(swp_csum);
	printcap(swp_lso);
	printcap(cqe_checksum_full);
	printcap(tunnel_stateless_geneve_tx);
	printcap(tunnel_stateless_mpls_over_udp);
	printcap(tunnel_stateless_mpls_over_gre);
	printcap(tunnel_stateless_vxlan_gpe);
	printcap(tunnel_stateless_ipv4_over_vxlan);
	printcap(tunnel_stateless_ip_over_ip);
	printcap(insert_trailer);
	printcap(tunnel_stateless_ip_over_ip_rx);
	printcap(tunnel_stateless_ip_over_ip_tx);
	printcap(max_vxlan_udp_ports);
	printcap(max_geneve_opt_len);
	printcap(tunnel_stateless_geneve_rx);
	printcap(lro_min_mss_size);
	printcap(lro_timer_supported_periods[0]);
	printcap(lro_timer_supported_periods[1]);
	printcap(lro_timer_supported_periods[2]);
	printcap(lro_timer_supported_periods[3]);
	printcap(enhanced_multi_pkt_send_wqe);
}

static void print_virtio_emulation_cap(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_VDPA_EMULATION << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *virtio_cap = query_caps(dev, opmod, out);

	if (!virtio_cap)
		return;
#define MLX5_CAP_VIRTIO(cap) \
	MLX5_GET(virtio_emulation_cap, virtio_cap, cap)
#define MLX5_CAP_VIRTIO64(cap) \
	MLX5_GET64(virtio_emulation_cap, virtio_cap, cap)
#undef  printcap
#undef  printcap64
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_VIRTIO(cap))
#define printcap64(cap) printf("\t" #cap ": %ld\n", MLX5_CAP_VIRTIO64(cap))

	printf("MLX5_CAP_VDPA_EMULATION:\n");
	printcap(desc_tunnel_offload_type);
	printcap(eth_frame_offload_type);
	printcap(virtio_version_1_0);
	printcap(device_features_bits_mask);
	printcap(event_mode);
	printcap(virtio_queue_type);
	printcap(max_tunnel_desc);
	printcap(log_doorbell_stride);
	printcap(log_doorbell_bar_size);
	printcap64(doorbell_bar_offset);
	printcap(max_emulated_devices);
	printcap(max_num_virtio_queues);
	printcap(umem_1_buffer_param_a);
	printcap(umem_1_buffer_param_b);
	printcap(umem_2_buffer_param_a);
	printcap(umem_2_buffer_param_b);
	printcap(umem_3_buffer_param_a);
	printcap(umem_3_buffer_param_b);
}

static void print_roce_cap(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_ROCE << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *roce_cap = query_caps(dev, opmod, out);

	if (!roce_cap)
		return;
#define MLX5_CAP_ROCE(cap) \
	MLX5_GET(roce_cap, roce_cap, cap)
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_ROCE(cap))

	printf("MLX5_CAP_ROCE:\n");
	printcap(roce_apm);
	printcap(sw_r_roce_src_udp_port);
	printcap(fl_rc_qp_when_roce_disabled);
	printcap(fl_rc_qp_when_roce_enabled);
	printcap(qp_ts_format);
	printcap(l3_type);
	printcap(roce_version);
	printcap(r_roce_dest_udp_port);
	printcap(r_roce_max_src_udp_port);
	printcap(r_roce_min_src_udp_port);
	printcap(roce_address_table_size);
}

static void print_debug_caps(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_DEBUG << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *debug_cap = query_caps(dev, opmod, out);

	if (!debug_cap)
		return;
#define MLX5_CAP_DEBUG(cap) \
	MLX5_GET(debug_cap, debug_cap, cap)
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_DEBUG(cap))

	printf("MLX5_CAP_DEBUG:\n");
	printcap(core_dump_general);
	printcap(core_dump_qp);
	printcap(log_cr_dump_to_mem_size);
	printcap(resource_dump);
	printcap(log_min_resource_dump_eq);
	printcap(log_max_samples);
	printcap(single);
	printcap(repetitive);
	printcap(health_mon_rx_activity);
	printcap(diag_counter_tracer_dump);
	printcap(log_min_sample_period);
}

static void print_port_selection_caps(struct mlx5u_dev *dev, void *out)
{
	u16 opmod = (MLX5_CAP_PORT_SELECTION << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	void *port_selection_cap = query_caps(dev, opmod, out);

	if (!port_selection_cap)
		return;
#define MLX5_CAP_PORT_SELECTION(cap) \
	MLX5_GET(port_selection_cap, port_selection_cap, cap)
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_PORT_SELECTION(cap))

	printf("MLX5_CAP_PORT_SELECTION:\n");
	printcap(port_select_flow_table);
	printcap(port_select_flow_table_bypass);
}

int do_devcap(struct mlx5u_dev *dev, int argc, char *argv[])
{
	int out_sz = MLX5_ST_SZ_BYTES(query_hca_cap_out);
	void *out;

	out = malloc(out_sz);
	if (!out)
		return ENOMEM;

	print_hca_caps(dev, out);
	print_hca_caps2(dev, out);
	print_eth_caps(dev, out);
	print_virtio_emulation_cap(dev, out);
	print_roce_cap(dev, out);
	print_debug_caps(dev, out);
	print_port_selection_caps(dev, out);

	free(out);
	return 0;
}
