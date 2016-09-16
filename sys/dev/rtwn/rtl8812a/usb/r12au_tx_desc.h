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

#ifndef R12AU_TX_DESC_H
#define R12AU_TX_DESC_H

#include <dev/rtwn/rtl8812a/r12a_tx_desc.h>

/* Tx MAC descriptor (USB). */
struct r12au_tx_desc {
	uint16_t	pktlen;
	uint8_t		offset;
	uint8_t		flags0;

	uint32_t	txdw1;
	uint32_t	txdw2;
	uint32_t	txdw3;
	uint32_t	txdw4;
	uint32_t	txdw5;
	uint32_t	txdw6;

	uint16_t	txdsum;
	uint16_t	flags7;
#define R12AU_FLAGS7_AGGNUM_M	0xff00
#define R12AU_FLAGS7_AGGNUM_S	8

	uint32_t	txdw8;
	uint32_t	txdw9;
} __packed __attribute__((aligned(4)));

#endif	/* R12AU_TX_DESC_H */
