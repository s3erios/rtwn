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

#ifndef R12A_FW_CMD_H
#define R12A_FW_CMD_H

#include <dev/rtwn/rtl8188e/r88e_fw_cmd.h>

/*
 * Host to firmware commands.
 */
/* Note: some parts are shared with RTL8188EU. */
#define R12A_CMD_MSR_RPT		0x01
#define R12A_CMD_SET_PWRMODE		0x20
#define R12A_CMD_IQ_CALIBRATE		0x45

/* Structure for R12A_CMD_MSR_RPT. */
struct r12a_fw_cmd_msrrpt {
	uint8_t		msrb0;
#define R12A_MSRRPT_B0_DISASSOC		0x00
#define R12A_MSRRPT_B0_ASSOC		0x01
#define R12A_MSRRPT_B0_MACID_IND	0x02

	uint8_t		macid;
	uint8_t		macid_end;
} __packed;

/* Structure for R12A_CMD_SET_PWRMODE. */
struct r12a_fw_cmd_pwrmode {
	uint8_t		mode;
	uint8_t		pwrb1;
	uint8_t		bcn_pass;
	uint8_t		queue_uapsd;
	uint8_t		pwr_state;
	uint8_t		pwrb5;
#define R12A_PWRMODE_B5_NO_BTCOEX	0x40
} __packed;

/* Structure for R12A_CMD_IQ_CALIBRATE. */
struct r12a_fw_cmd_iq_calib {
	uint8_t		chan;
	uint8_t		band_bw;
#define RTWN_CMD_IQ_CHAN_WIDTH_20	0x01
#define RTWN_CMD_IQ_CHAN_WIDTH_40	0x02
#define RTWN_CMD_IQ_CHAN_WIDTH_80	0x04
#define RTWN_CMD_IQ_CHAN_WIDTH_160	0x08
#define RTWN_CMD_IQ_BAND_2GHZ		0x10
#define RTWN_CMD_IQ_BAND_5GHZ		0x20

	uint8_t		ext_5g_pa_lna;
#define RTWN_CMD_IQ_EXT_PA_5G(pa)	(pa)
#define RTWN_CMD_IQ_EXT_LNA_5G(lna)	((lna) << 1)
} __packed;


/*
 * C2H event types.
 */
#define R12A_C2H_DEBUG		0x00
#define R12A_C2H_TX_REPORT	0x03
#define R12A_C2H_BT_INFO	0x09
#define R12A_C2H_RA_REPORT	0x0c
#define R12A_C2H_IQK_FINISHED	0x11

/* Structure for R12A_C2H_TX_REPORT event. */
struct r12a_c2h_tx_rpt {
	uint8_t		txrptb0;
#define R12A_TXRPTB0_QSEL_M		0x1f
#define R12A_TXRPTB0_QSEL_S		0
#define R12A_TXRPTB0_BC			0x20
#define R12A_TXRPTB0_LIFE_EXPIRE	0x40
#define R12A_TXRPTB0_RETRY_OVER		0x80

	uint8_t		macid;
	uint8_t		txrptb2;
#define R12A_TXRPTB2_RETRY_CNT_M	0x3f
#define R12A_TXRPTB2_RETRY_CNT_S	0

	uint16_t	queue_time;	/* 256 msec unit */
	uint8_t		final_rate;
	uint16_t	reserved;
} __packed;

/* Structure for R12A_C2H_RA_REPORT event. */
struct r12a_c2h_ra_report {
	uint8_t		rarptb0;
#define R12A_RARPTB0_RATE_M	0x3f
#define R12A_RARPTB0_RATE_S	0

	uint8_t		macid;
	uint8_t		rarptb2;
#define R12A_RARPTB0_LDPC	0x01
#define R12A_RARPTB0_TXBF	0x02
#define R12A_RARPTB0_NOISE	0x04
} __packed;

#endif	/* R12A_FW_CMD_H */

