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

#ifndef R12A_TX_DESC_H
#define R12A_TX_DESC_H

/* Tx MAC descriptor (USB). */
struct r12au_tx_desc {
	uint16_t	pktlen;
	uint8_t		offset;
	uint8_t		flags0;
#define R12A_FLAGS0_BMCAST	0x01
#define R12A_FLAGS0_LSG		0x04
#define R12A_FLAGS0_FSG		0x08
#define R12A_FLAGS0_OWN		0x80

	uint32_t	txdw1;
#define R12A_TXDW1_MACID_M	0x0000003f
#define R12A_TXDW1_MACID_S	0
#define R12A_TXDW1_QSEL_M	0x00001f00
#define R12A_TXDW1_QSEL_S	8

#define R12A_TXDW1_QSEL_BE	0x00	/* or 0x03 */
#define R12A_TXDW1_QSEL_BK	0x01	/* or 0x02 */
#define R12A_TXDW1_QSEL_VI	0x04	/* or 0x05 */
#define R12A_TXDW1_QSEL_VO	0x06	/* or 0x07 */
#define RTWN_MAX_TID		8

#define R12A_TXDW1_QSEL_BEACON	0x10
#define R12A_TXDW1_QSEL_MGNT	0x12

#define R12A_TXDW1_RAID_M	0x001f0000
#define R12A_TXDW1_RAID_S	16
#define R12A_TXDW1_CIPHER_M	0x00c00000
#define R12A_TXDW1_CIPHER_S	22
#define R12A_TXDW1_CIPHER_NONE	0
#define R12A_TXDW1_CIPHER_RC4	1
#define R12A_TXDW1_CIPHER_SM4	2
#define R12A_TXDW1_CIPHER_AES	3
#define R12A_TXDW1_PKTOFF_M	0x1f000000
#define R12A_TXDW1_PKTOFF_S	24

	uint32_t	txdw2;
#define R12A_TXDW2_AGGEN	0x00001000
#define R12A_TXDW2_AGGBK	0x00010000
#define R12A_TXDW2_MOREFRAG	0x00020000
#define R12A_TXDW2_SPE_RPT	0x00080000
#define R12A_TXDW2_AMPDU_DEN_M	0x00700000
#define R12A_TXDW2_AMPDU_DEN_S	20

	uint32_t	txdw3;
#define R12A_TXDW3_SEQ_SEL_M	0x000000c0
#define R12A_TXDW3_SEQ_SEL_S	6
#define R12A_TXDW3_DRVRATE	0x00000100
#define R12A_TXDW3_DISRTSFB	0x00000200
#define R12A_TXDW3_DISDATAFB	0x00000400
#define R12A_TXDW3_CTS2SELF	0x00000800
#define R12A_TXDW3_RTSEN	0x00001000
#define R12A_TXDW3_HWRTSEN	0x00002000
#define R12A_TXDW3_MAX_AGG_M	0x003e0000
#define R12A_TXDW3_MAX_AGG_S	17

	uint32_t	txdw4;
#define R12A_TXDW4_DATARATE_M		0x0000007f
#define R12A_TXDW4_DATARATE_S		0
#define R12A_TXDW4_DATARATE_FB_LMT_M	0x00001f00
#define R12A_TXDW4_DATARATE_FB_LMT_S	8
#define R12A_TXDW4_RTSRATE_FB_LMT_M	0x0001e000
#define R12A_TXDW4_RTSRATE_FB_LMT_S	13
#define R12A_TXDW4_RETRY_LMT_ENA	0x00020000
#define R12A_TXDW4_RETRY_LMT_M		0x00fc0000
#define R12A_TXDW4_RETRY_LMT_S		18
#define R12A_TXDW4_RTSRATE_M		0x1f000000
#define R12A_TXDW4_RTSRATE_S		24

	uint32_t	txdw5;
#define R12A_TXDW5_SHORT	0x00000010
#define R12A_TXDW5_DATA_LDPC	0x00000080

	uint32_t	txdw6;
#define R21A_TXDW6_MBSSID_M	0x0000f000
#define R21A_TXDW6_MBSSID_S	12

	uint16_t	txdsum;
	uint16_t	flags7;
#define R12A_FLAGS7_AGGNUM_M	0xff00
#define R12A_FLAGS7_AGGNUM_S	8

	uint32_t	txdw8;
#define R12A_TXDW8_HWSEQ_EN	0x00008000

	uint32_t	txdw9;
#define R12A_TXDW9_SEQ_M	0x00fff000
#define R12A_TXDW9_SEQ_S	12
} __packed __attribute__((aligned(4)));


/* Rate adaptation modes. */
#define R12A_RAID_11BGN_2_40	0
#define R12A_RAID_11BGN_1_40	1
#define R12A_RAID_11BGN_2	2
#define R12A_RAID_11BGN_1	3
#define R12A_RAID_11GN_2	4
#define R12A_RAID_11GN_1	5
#define R12A_RAID_11BG		6
#define R12A_RAID_11G		7	/* "pure" 11g */
#define R12A_RAID_11B		8
#define R12A_RAID_11AC_2_80	9
#define R12A_RAID_11AC_1_80	10
#define R12A_RAID_11AC_1	11
#define R12A_RAID_11AC_2	12

#endif	/* R12A_TX_DESC_H */

