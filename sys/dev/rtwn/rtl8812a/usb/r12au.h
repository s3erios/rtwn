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

#ifndef RTL8812AU_H
#define RTL8812AU_H

#include <dev/rtwn/rtl8812a/r12a.h>


/*
 * Function declarations.
 */
/* r12au_init.c */
void	r12au_init_rx_agg(struct rtwn_softc *);
void	r12au_init_burstlen(struct rtwn_softc *);
void	r12au_init_ampdu_fwhw(struct rtwn_softc *);
void	r12au_init_ampdu(struct rtwn_softc *);
void	r12au_post_init(struct rtwn_softc *);

/* r12au_rx.c */
int	r12au_classify_intr(struct rtwn_softc *, void *, int);
int	r12au_align_rx(int, int);

/* r12au_tx.c */
void	r12au_dump_tx_desc(struct rtwn_softc *, const void *);

#endif	/* RTL8812AU_H */
