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

#ifndef R12A_RX_DESC_H
#define R12A_RX_DESC_H

#include <dev/rtwn/rtl8192c/r92c_rx_desc.h>

/* Rx MAC descriptor defines (chip-specific). */
/* Rx dword 1 */
#define R12A_RXDW1_AMSDU	0x00002000
#define R12A_RXDW1_CKSUM_ERR	0x00100000
#define R12A_RXDW1_IPV6		0x00200000
#define R12A_RXDW1_UDP		0x00400000
#define R12A_RXDW1_CKSUM	0x00800000
/* Rx dword 2 */
#define R12A_RXDW2_RPT_C2H	0x10000000

/* Rx PHY descriptor. */
struct r12a_rx_phystat {
	uint8_t		gain_trsw[2];
	uint16_t	phyw1;
#define R12A_PHYW1_CHAN_M	0x03ff
#define R12A_PHYW1_CHAN_S	0
#define R12A_PHYW1_CHAN_EXT_M	0x3c00
#define R12A_PHYW1_CHAN_EXT_S	10
#define R12A_PHYW1_RFMOD_M	0xc000
#define R12A_PHYW1_RFMOD_S	14

	uint8_t		pwdb_all;
	uint8_t		cfosho[4];
	uint8_t		cfotail[4];
	uint8_t		rxevm[2];
	uint8_t		rxsnr[2];
	uint8_t		pcts_msk_rpt[2];
	uint8_t		pdsnr[2];
	uint8_t		csi_current[2];
	uint8_t		rx_gain_c;
	uint8_t		rx_gain_d;
	uint8_t		sigevm;
	uint16_t	phyw13;
#define R12A_PHYW13_ANTIDX_A_M	0x0700
#define R12A_PHYW13_ANTIDX_A_S	8
#define R12A_PHYW13_ANTIDX_B_M	0x3800
#define R12A_PHYW13_ANTIDX_B_S	11
} __packed;

#endif	/* R12A_RX_DESC_H */

