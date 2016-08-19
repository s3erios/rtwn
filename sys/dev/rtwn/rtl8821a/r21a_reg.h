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

#ifndef R21A_REG_H
#define R21A_REG_H

#include <dev/rtwn/rtl8812a/r12a_reg.h>

/*
 * MAC registers.
 */
/* Tx DMA Configuration. */
#define R21A_DWBCN0_CTRL		R92C_TDECTRL
#define R21A_DWBCN1_CTRL		0x228


/* Bits for R92C_MAC_PHY_CTRL. */
#define R21A_MAC_PHY_CRYSTALCAP_M	0x00fff000
#define R21A_MAC_PHY_CRYSTALCAP_S	12

/* Bits for R21A_DWBCN1_CTRL. */
#define R21A_DWBCN1_CTRL_SEL_EN		0x00020000
#define R21A_DWBCN1_CTRL_SEL_BCN1	0x00100000

#endif	/* R21A_REG_H */
