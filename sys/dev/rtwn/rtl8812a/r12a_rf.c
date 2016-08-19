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
#include <dev/rtwn/rtl8812a/r12a_reg.h>


uint32_t
r12a_rf_read(struct rtwn_softc *sc, int chain, uint8_t addr)
{
	uint32_t pi_mode, val;

	/* Turn off CCA (avoids reading the wrong value). */
	if (addr != R92C_RF_AC)
		rtwn_bb_setbits(sc, R12A_CCA_ON_SEC, 0, 0x08);

	val = rtwn_bb_read(sc, R12A_HSSI_PARAM1(chain));
	pi_mode = (val & R12A_HSSI_PARAM1_PI) ? 1 : 0;

	rtwn_bb_setbits(sc, R12A_HSSI_PARAM2,
	    R12A_HSSI_PARAM2_READ_ADDR_MASK, addr);

	val = rtwn_bb_read(sc, pi_mode ? R12A_HSPI_READBACK(chain) :
	    R12A_LSSI_READBACK(chain));

	/* Turn on CCA (when exiting). */
	if (addr != R92C_RF_AC)
		rtwn_bb_setbits(sc, R12A_CCA_ON_SEC, 0x08, 0);

	return (MS(val, R92C_LSSI_READBACK_DATA));
}

uint32_t
r12a_c_cut_rf_read(struct rtwn_softc *sc, int chain, uint8_t addr)
{
	uint32_t pi_mode, val;

	val = rtwn_bb_read(sc, R12A_HSSI_PARAM1(chain));
	pi_mode = (val & R12A_HSSI_PARAM1_PI) ? 1 : 0;

	rtwn_bb_setbits(sc, R12A_HSSI_PARAM2,
	    R12A_HSSI_PARAM2_READ_ADDR_MASK, addr);
	rtwn_delay(sc, 20);

	val = rtwn_bb_read(sc, pi_mode ? R12A_HSPI_READBACK(chain) :
	    R12A_LSSI_READBACK(chain));

	return (MS(val, R92C_LSSI_READBACK_DATA));
}

void
r12a_rf_write(struct rtwn_softc *sc, int chain, uint8_t addr,
    uint32_t val)
{
	rtwn_bb_write(sc, R12A_LSSI_PARAM(chain),
	    SM(R88E_LSSI_PARAM_ADDR, addr) |
	    SM(R92C_LSSI_PARAM_DATA, val));
}

