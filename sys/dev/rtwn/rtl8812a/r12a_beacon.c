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

#include <dev/rtwn/if_rtwn_ridx.h>

#include <dev/rtwn/rtl8812a/r12a.h>
#include <dev/rtwn/rtl8812a/r12a_reg.h>
#include <dev/rtwn/rtl8812a/usb/r12au_tx_desc.h>	/* XXX */


void
r12a_beacon_init(struct rtwn_softc *sc, void *buf, int id)
{
	struct r12au_tx_desc *txd = (struct r12au_tx_desc *)buf; /* XXX */

	txd->flags0 = R12A_FLAGS0_LSG | R12A_FLAGS0_FSG | R12A_FLAGS0_BMCAST;

	/*
	 * NB: there is no need to setup HWSEQ_EN bit;
	 * QSEL_BEACON already implies it.
	 */
	txd->txdw1 = htole32(SM(R12A_TXDW1_QSEL, R12A_TXDW1_QSEL_BEACON));
	txd->txdw1 |= htole32(SM(R12A_TXDW1_MACID, RTWN_MACID_BC));

	txd->txdw3 = htole32(R12A_TXDW3_DRVRATE);
	txd->txdw3 |= htole32(SM(R12A_TXDW3_SEQ_SEL, id));

	txd->txdw6 = htole32(SM(R21A_TXDW6_MBSSID, id));
}

void
r12a_beacon_set_rate(void *buf, int is5ghz)
{
	struct r12au_tx_desc *txd = (struct r12au_tx_desc *)buf; /* XXX */

	txd->txdw4 &= ~htole32(R12A_TXDW4_DATARATE_M);
	if (is5ghz) {
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE,
		    RTWN_RIDX_OFDM6));
	} else
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE, RTWN_RIDX_CCK1));
}
