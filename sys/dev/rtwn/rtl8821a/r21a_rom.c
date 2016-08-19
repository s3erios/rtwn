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

#include <dev/rtwn/rtl8812a/r12a.h>
#include <dev/rtwn/rtl8812a/r12a_var.h>
#include <dev/rtwn/rtl8812a/r12a_rom_image.h>

#include <dev/rtwn/rtl8821a/r21a.h>
#include <dev/rtwn/rtl8821a/r21a_reg.h>


void
r21a_parse_rom(struct rtwn_softc *sc, uint8_t *buf)
{
	struct r12a_softc *rs = sc->sc_priv;
	struct r12a_rom *rom = (struct r12a_rom *)buf;
	uint8_t pa_type, lna_type_2g, lna_type_5g;

	/* Read PA/LNA types. */
	pa_type = RTWN_GET_ROM_VAR(rom->pa_type, 0);
	lna_type_2g = RTWN_GET_ROM_VAR(rom->lna_type_2g, 0);
	lna_type_5g = RTWN_GET_ROM_VAR(rom->lna_type_5g, 0);

	rs->ext_pa_2g = R21A_ROM_IS_PA_EXT_2GHZ(pa_type);
	rs->ext_pa_5g = R21A_ROM_IS_PA_EXT_5GHZ(pa_type);
	rs->ext_lna_2g = R21A_ROM_IS_LNA_EXT(lna_type_2g);
	rs->ext_lna_5g = R21A_ROM_IS_LNA_EXT(lna_type_5g);

	RTWN_LOCK(sc);
	rs->bt_coex =
	    !!(rtwn_read_4(sc, R92C_MULTI_FUNC_CTRL) & R92C_MULTI_BT_FUNC_EN);
	RTWN_UNLOCK(sc);
	rs->bt_ant_num = (rom->rf_bt_opt & R12A_RF_BT_OPT_ANT_NUM);

	/* Read MAC address. */
	IEEE80211_ADDR_COPY(sc->sc_ic.ic_macaddr, rom->macaddr_21a);

	/* Execute common part of initialization. */
	r12a_parse_rom_common(sc, buf);
}

