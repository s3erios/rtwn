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

#include <dev/rtwn/if_rtwnvar.h>
#include <dev/rtwn/if_rtwn_debug.h>

#include <dev/rtwn/usb/rtwn_usb_reg.h>

#include <dev/rtwn/rtl8812a/usb/r12au.h>
#include <dev/rtwn/rtl8812a/usb/r12au_tx_desc.h>


void
r12au_fill_tx_desc_null(struct rtwn_softc *sc, void *buf, int is11b,
    int qos, int id)
{

	r12a_fill_tx_desc_null(sc, buf, is11b, qos, id);
	r12au_tx_checksum(buf);
}

void
r12au_dump_tx_desc(struct rtwn_softc *sc, const void *desc)
{
#ifdef RTWN_DEBUG
	const struct r12au_tx_desc *txd = desc;

	RTWN_DPRINTF(sc, RTWN_DEBUG_XMIT_DESC,
	    "%s: len %d, off %d, flags0 %02X, dw: 1 %08X, 2 %08X, 3 %08X, "
	    "4 %08X, 5 %08X, 6 %08X, sum %04X, flags7 %04X, 8 %08X, 9 %08X\n",
	    __func__, le16toh(txd->pktlen), txd->offset, txd->flags0,
	    le32toh(txd->txdw1), le32toh(txd->txdw2), le32toh(txd->txdw3),
	    le32toh(txd->txdw4), le32toh(txd->txdw5), le32toh(txd->txdw6),
	    le16toh(txd->txdsum), le16toh(txd->flags7), le32toh(txd->txdw8),
	    le32toh(txd->txdw9));
#endif
}

void
r12au_tx_checksum(void *buf)
{
	struct r12au_tx_desc *txd = (struct r12au_tx_desc *)buf;

	txd->txdsum = 0;
	txd->txdsum = rtwn_usb_calc_tx_checksum(txd);
}
