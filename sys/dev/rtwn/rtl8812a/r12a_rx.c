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

#include <dev/rtwn/if_rtwnreg.h>
#include <dev/rtwn/if_rtwnvar.h>

#include <dev/rtwn/if_rtwn_debug.h>
#include <dev/rtwn/if_rtwn_ridx.h>

#include <dev/rtwn/rtl8812a/r12a.h>
#include <dev/rtwn/rtl8812a/r12a_var.h>
#include <dev/rtwn/rtl8812a/r12a_fw_cmd.h>
#include <dev/rtwn/rtl8812a/r12a_rx_desc.h>


#ifndef RTWN_WITHOUT_UCODE
void
r12a_ratectl_tx_complete(struct rtwn_softc *sc, uint8_t *buf, int len)
{
	struct r12a_c2h_tx_rpt *rpt;
	struct ieee80211vap *vap;
	struct ieee80211_node *ni;
	int ntries;

	/* Skip Rx descriptor / report id / sequence fields. */
	buf += sizeof(struct r92c_rx_stat) + 2;
	len -= sizeof(struct r92c_rx_stat) + 2;

	rpt = (struct r12a_c2h_tx_rpt *)buf;
	if (len != sizeof(*rpt)) {
		device_printf(sc->sc_dev,
		    "%s: wrong report size (%d, must be %zu)\n",
		    __func__, len, sizeof(*rpt));
		return;
	}

	if (rpt->macid > sc->macid_limit) {
		device_printf(sc->sc_dev,
		    "macid %u is too big; increase MACID_MAX limit\n",
		    rpt->macid);
		return;
	}

	ntries = MS(rpt->txrptb2, R12A_TXRPTB2_RETRY_CNT);

	ni = sc->node_list[rpt->macid];
	if (ni != NULL) {
		vap = ni->ni_vap;
		RTWN_DPRINTF(sc, RTWN_DEBUG_INTR, "%s: frame for macid %u was"
		    "%s sent (%d retries)\n", __func__, rpt->macid,
		    (rpt->txrptb0 & (R12A_TXRPTB0_RETRY_OVER |
		    R12A_TXRPTB0_LIFE_EXPIRE)) ? " not" : "", ntries);

		if (rpt->txrptb0 & R12A_TXRPTB0_RETRY_OVER) {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_FAILURE, &ntries, NULL);
		} else {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_SUCCESS, &ntries, NULL);
		}
	} else {
		RTWN_DPRINTF(sc, RTWN_DEBUG_INTR,
		    "%s: macid %u, ni is NULL\n", __func__, rpt->macid);
	}
}

void
r12a_handle_c2h_report(struct rtwn_softc *sc, uint8_t *buf, int len)
{
	struct r12a_softc *rs = sc->sc_priv;

	/* Skip Rx descriptor. */
	buf += sizeof(struct r92c_rx_stat);
	len -= sizeof(struct r92c_rx_stat);

	if (len < 2) {
		device_printf(sc->sc_dev, "C2H report too short (len %d)\n",
		    len);
		return;
	}
	len -= 2;

	switch (buf[0]) {	/* command id */
	case R12A_C2H_TX_REPORT:
		/* NOTREACHED */
		KASSERT(0, ("use handle_tx_report() instead of %s\n",
		    __func__));
		break;
	case R12A_C2H_IQK_FINISHED:
		RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB,
		    "FW IQ calibration finished\n");
		rs->rs_flags &= ~R12A_IQK_RUNNING;
		break;
	default:
		device_printf(sc->sc_dev,
		    "%s: C2H report %u was not handled\n",
		    __func__, buf[0]);
	}
}
#else
void
r12a_ratectl_tx_complete(struct rtwn_softc *sc, uint8_t *buf, int len)
{
	/* Should not happen. */
	device_printf(sc->sc_dev, "%s: called\n", __func__);
}

void
r12a_handle_c2h_report(struct rtwn_softc *sc, uint8_t *buf, int len)
{
	/* Should not happen. */
	device_printf(sc->sc_dev, "%s: called\n", __func__);
}
#endif

int
r12a_check_frame_checksum(struct rtwn_softc *sc, struct mbuf *m)
{
	struct r12a_softc *rs = sc->sc_priv;
	struct r92c_rx_stat *stat;
	uint32_t rxdw1;

	stat = mtod(m, struct r92c_rx_stat *);
	rxdw1 = le32toh(stat->rxdw1);
	if (rxdw1 & R12A_RXDW1_CKSUM) {
		RTWN_DPRINTF(sc, RTWN_DEBUG_RECV,
		    "%s: %s/%s checksum is %s\n", __func__,
		    (rxdw1 & R12A_RXDW1_UDP) ? "UDP" : "TCP",
		    (rxdw1 & R12A_RXDW1_IPV6) ? "IPv6" : "IP",
		    (rxdw1 & R12A_RXDW1_CKSUM_ERR) ? "invalid" : "valid");

		if (rxdw1 & R12A_RXDW1_CKSUM_ERR)
			return (-1);

		if ((rxdw1 & R12A_RXDW1_IPV6) ?
		    (rs->rs_flags & R12A_RXCKSUM6_EN) :
		    (rs->rs_flags & R12A_RXCKSUM_EN)) {
			m->m_pkthdr.csum_flags = CSUM_IP_CHECKED |
			    CSUM_IP_VALID | CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
			m->m_pkthdr.csum_data = 0xffff;
		}
	}

	return (0);
}

uint8_t
r12a_rx_radiotap_flags(const void *buf)
{
	const struct r92c_rx_stat *stat = buf;
	uint8_t flags, rate;

	if (!(stat->rxdw4 & htole32(R12A_RXDW4_SPLCP)))
		return (0);
	rate = MS(le32toh(stat->rxdw3), R92C_RXDW3_RATE);
	if (RTWN_RATE_IS_CCK(rate))
		flags = IEEE80211_RADIOTAP_F_SHORTPRE;
	else
		flags = IEEE80211_RADIOTAP_F_SHORTGI;
	return (flags);
}
