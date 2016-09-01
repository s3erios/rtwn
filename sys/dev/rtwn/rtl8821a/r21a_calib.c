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

#include <dev/rtwn/if_rtwn_debug.h>

#include <dev/rtwn/rtl8812a/r12a.h>

#include <dev/rtwn/rtl8821a/r21a.h>
#include <dev/rtwn/rtl8821a/r21a_reg.h>
#include <dev/rtwn/rtl8821a/r21a_priv.h>


#ifndef RTWN_WITHOUT_UCODE
int
r21a_iq_calib_fw_supported(struct rtwn_softc *sc)
{
	if (sc->fwver == 0x16)
		return (1);

	return (0);
}
#endif

void
r21a_iq_calib_sw(struct rtwn_softc *sc)
{
#define R21A_MAX_NRXCHAINS	2
	uint32_t saved_bb_vals[nitems(r21a_iq_bb_regs)];
	uint32_t saved_afe_vals[nitems(r21a_iq_afe_regs)];
	uint32_t saved_rf_vals[nitems(r21a_iq_rf_regs) * R21A_MAX_NRXCHAINS];

	KASSERT(sc->nrxchains <= R21A_MAX_NRXCHAINS,
	    ("nrxchains > %d (%d)\n", R21A_MAX_NRXCHAINS, sc->nrxchains));

	RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB, "%s: SW IQ calibration: TODO\n",
	    __func__);

	/* Save registers. */
	r12a_save_bb_afe_vals(sc, saved_bb_vals, r21a_iq_bb_regs,
	    nitems(r21a_iq_bb_regs));
	r12a_save_bb_afe_vals(sc, saved_afe_vals, r21a_iq_afe_regs,
	    nitems(r21a_iq_afe_regs));
	r12a_save_rf_vals(sc, saved_rf_vals, r21a_iq_rf_regs,
	    nitems(r21a_iq_rf_regs));

#ifdef RTWN_TODO
	/* Configure MAC. */
	r12a_iq_config_mac(sc);
	r21a_iq_tx(sc);
#endif

	/* Restore registers. */
	r12a_restore_rf_vals(sc, saved_rf_vals, r21a_iq_rf_regs,
	    nitems(r21a_iq_rf_regs));
	r12a_restore_bb_afe_vals(sc, saved_afe_vals, r21a_iq_afe_regs,
	    nitems(r21a_iq_afe_regs));

	/* Select page C1. */
	rtwn_bb_setbits(sc, R12A_TXAGC_TABLE_SELECT, 0, 0x80000000);

	rtwn_bb_write(sc, R12A_SLEEP_NAV(0), 0);
	rtwn_bb_write(sc, R12A_PMPD(0), 0);
	rtwn_bb_write(sc, 0xc88, 0);
	rtwn_bb_write(sc, 0xc8c, 0x3c000000);
	rtwn_bb_write(sc, 0xc90, 0x80);
	rtwn_bb_write(sc, 0xc94, 0);
	rtwn_bb_write(sc, 0xcc4, 0x20040000);
	rtwn_bb_write(sc, 0xcc8, 0x20000000);
	rtwn_bb_write(sc, 0xcb8, 0);

	r12a_restore_bb_afe_vals(sc, saved_bb_vals, r21a_iq_bb_regs,
	    nitems(r21a_iq_bb_regs));
#undef R21A_MAX_NRXCHAINS
}

