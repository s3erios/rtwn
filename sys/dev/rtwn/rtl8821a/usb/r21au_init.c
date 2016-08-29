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

#include <dev/rtwn/usb/rtwn_usb_var.h>

#include <dev/rtwn/rtl8192c/usb/r92cu.h>

#include <dev/rtwn/rtl8821a/usb/r21au.h>
#include <dev/rtwn/rtl8821a/usb/r21au_reg.h>


void
r21au_init_tx_agg(struct rtwn_softc *sc)
{
	struct rtwn_usb_softc *uc = RTWN_USB_SOFTC(sc);

	r92cu_init_tx_agg(sc);

	rtwn_write_1(sc, R21A_DWBCN1_CTRL, uc->tx_agg_desc_num << 1);
}

void
r21au_init_burstlen(struct rtwn_softc *sc)
{
	if ((rtwn_read_1(sc, R92C_USB_INFO) & 0x30) == 0) {
		/* Set burst packet length to 512 B. */
		rtwn_setbits_1(sc, R12A_RXDMA_PRO, 0x20, 0x1e);
	} else {
		/* Set burst packet length to 64 B. */
		rtwn_setbits_1(sc, R12A_RXDMA_PRO, 0x10, 0x2e);
	}
}
