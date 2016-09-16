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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_wlan.h"

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/linker.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/rtwn/if_rtwnreg.h>
#include <dev/rtwn/if_rtwnvar.h>

#include <dev/rtwn/rtl8812a/r12a.h>
#include <dev/rtwn/rtl8812a/r12a_tx_desc.h>

#include <dev/rtwn/rtl8821a/r21a.h>
#include <dev/rtwn/rtl8821a/r21a_reg.h>


void
r21a_beacon_init(struct rtwn_softc *sc, void *buf, int id)
{
	struct r12a_tx_desc *txd = (struct r12a_tx_desc *)buf;

	r12a_beacon_init(sc, buf, id);

	/* XXX sequence number for beacon 1 is not stable. */
	txd->txdw3 &= ~htole32(R12A_TXDW3_SEQ_SEL_M);
	txd->txdw3 |= htole32(SM(R12A_TXDW3_SEQ_SEL, id * 2));
}

void
r21a_beacon_select(struct rtwn_softc *sc, int id)
{
	switch (id) {
	case 0:
		/* Switch to port 0 beacon. */
		rtwn_setbits_1_shift(sc, R21A_DWBCN1_CTRL,
		    R21A_DWBCN1_CTRL_SEL_BCN1, 0, 2);
		break;
	case 1:
		/* Switch to port 1 beacon. */
		rtwn_setbits_1_shift(sc, R21A_DWBCN1_CTRL,
		    0, R21A_DWBCN1_CTRL_SEL_BCN1, 2);
		break;
	default:
		KASSERT(0, ("wrong port id %d\n", id));
		break;
	}
}

