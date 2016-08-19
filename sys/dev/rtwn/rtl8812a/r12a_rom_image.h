/*-
 * Copyright (c) 2016 Andriy Voskoboinyk <avos@FreeBSD.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $FreeBSD$
 */

#ifndef R12A_ROM_H
#define R12A_ROM_H

#include <dev/rtwn/rtl8812a/r12a_rom_defs.h>

#define R12A_DEF_TX_PWR_2G	0x2d
#define R12A_DEF_TX_PWR_5G	0xfe

struct r12a_tx_pwr_2g {
	uint8_t		cck[R12A_GROUP_2G];
	uint8_t		ht40[R12A_GROUP_2G - 1];
} __packed;

struct r12a_tx_pwr_diff_2g {
	uint8_t		ht20_ofdm;
	struct {
		uint8_t	ht40_ht20;
		uint8_t	ofdm_cck;
	} __packed	diff123[R12A_MAX_TX_COUNT - 1];
} __packed;

struct r12a_tx_pwr_5g {
	uint8_t		ht40[R12A_GROUP_5G];
} __packed;

struct r12a_tx_pwr_diff_5g {
	uint8_t		ht20_ofdm;
	uint8_t		ht40_ht20[R12A_MAX_TX_COUNT - 1];
	uint8_t		ofdm_ofdm[2];
	uint8_t		ht80_ht160[R12A_MAX_TX_COUNT];
} __packed;

struct r12a_tx_pwr {
	struct r12a_tx_pwr_2g		pwr_2g;
	struct r12a_tx_pwr_diff_2g	pwr_diff_2g;
	struct r12a_tx_pwr_5g		pwr_5g;
	struct r12a_tx_pwr_diff_5g	pwr_diff_5g;
} __packed;

/*
 * RTL8812AU/RTL8821AU ROM image.
 */
struct r12a_rom {
	uint8_t			reserved1[16];
	struct r12a_tx_pwr	tx_pwr[R12A_MAX_RF_PATH];
	uint8_t			channel_plan;
	uint8_t			crystalcap;
#define R12A_ROM_CRYSTALCAP_DEF		0x20

	uint8_t			thermal_meter;
	uint8_t			iqk_lck;
	uint8_t			pa_type;
#define R12A_ROM_IS_PA_EXT_2GHZ(pa_type)	(((pa_type) & 0x30) == 0x30)
#define R12A_ROM_IS_PA_EXT_5GHZ(pa_type)	(((pa_type) & 0x03) == 0x03)
#define R21A_ROM_IS_PA_EXT_2GHZ(pa_type)	(((pa_type) & 0x10) == 0x10)
#define R21A_ROM_IS_PA_EXT_5GHZ(pa_type)	(((pa_type) & 0x01) == 0x01)

	uint8_t			lna_type_2g;
#define R12A_ROM_IS_LNA_EXT(lna_type)		(((lna_type) & 0x88) == 0x88)
#define R21A_ROM_IS_LNA_EXT(lna_type)		(((lna_type) & 0x08) == 0x08)

#define R12A_GET_ROM_PA_TYPE(lna_type, chain)		\
	(((lna_type) >> ((chain) * 4 + 2)) & 0x01)
#define R12A_GET_ROM_LNA_TYPE(lna_type, chain)	\
	(((lna_type) >> ((chain) * 4)) & 0x03)

	uint8_t			reserved2;
	uint8_t			lna_type_5g;
	uint8_t			reserved3;
	uint8_t			rf_board_opt;
#define R12A_BOARD_TYPE_COMBO_MF	5

	uint8_t			rf_feature_opt;
	uint8_t			rf_bt_opt;
#define R12A_RF_BT_OPT_ANT_NUM		0x01

	uint8_t			version;
	uint8_t			customer_id;
	uint8_t			tx_bbswing_2g;
	uint8_t			tx_bbswing_5g;
	uint8_t			tx_pwr_calib_rate;
	uint8_t			rf_ant_opt;
	uint8_t			rfe_option;
	uint8_t			reserved4[5];
	uint16_t		vid_12a;
	uint16_t		pid_12a;
	uint8_t			reserved5[3];
	uint8_t			macaddr_12a[IEEE80211_ADDR_LEN];
	uint8_t			reserved6[35];
	uint16_t		vid_21a;
	uint16_t		pid_21a;
	uint8_t			reserved7[3];
	uint8_t			macaddr_21a[IEEE80211_ADDR_LEN];
	uint8_t			reserved8[2];
	/* XXX check on RTL8812AU. */
	uint8_t			string[8];	/* "Realtek " */
	uint8_t			reserved9[2];
	uint8_t			string_ven[23];	/* XXX variable length? */
	uint8_t			reserved10[208];
} __packed;

_Static_assert(sizeof(struct r12a_rom) == R12A_EFUSE_MAP_LEN,
    "R12A_EFUSE_MAP_LEN must be equal to sizeof(struct r12a_rom)!");

#endif	/* R12A_ROM_H */

