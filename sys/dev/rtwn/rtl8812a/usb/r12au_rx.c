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
#include <net80211/ieee80211_ratectl.h>

#include <dev/rtwn/if_rtwnvar.h>

#include <dev/rtwn/rtl8812a/r12a_fw_cmd.h>
#include <dev/rtwn/rtl8812a/r12a_rx_desc.h>

#include <dev/rtwn/rtl8812a/usb/r12au.h>


int
r12au_classify_intr(struct rtwn_softc *sc, void *buf, int len)
{
	struct r92c_rx_stat *stat = buf;
	uint32_t rxdw2 = le32toh(stat->rxdw2);

	if (rxdw2 & R12A_RXDW2_RPT_C2H) {
		int pos = sizeof(struct r92c_rx_stat);
		/* Check if Rx descriptor + command id/sequence fits. */
		if (len < pos + 2)	/* unknown, skip */
			return (RTWN_RX_DATA);

		if (((uint8_t *)buf)[pos] == R12A_C2H_TX_REPORT)
			return (RTWN_RX_TX_REPORT);
		else
			return (RTWN_RX_OTHER);
	} else
		return (RTWN_RX_DATA);
}

int
r12au_align_rx(int totlen, int len)
{
	if (totlen < len)
		return (roundup2(totlen, 8));

	return (totlen);
}
