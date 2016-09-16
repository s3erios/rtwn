/*	$OpenBSD: if_urtwn.c,v 1.16 2011/02/10 17:26:40 jakemsr Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2014 Kevin Lo <kevlo@FreeBSD.org>
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

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/rtwn/if_rtwnreg.h>
#include <dev/rtwn/if_rtwnvar.h>
#include <dev/rtwn/if_rtwn_debug.h>

#include <dev/rtwn/pci/rtwn_pci_var.h>

#include <dev/rtwn/rtl8192c/pci/r92ce.h>
#include <dev/rtwn/rtl8192c/pci/r92ce_reg.h>


/* XXX 92C? */
static int
r92ce_iq_calib_chain(struct rtwn_softc *sc, int chain, uint16_t tx[2],
    uint16_t rx[2])
{
	uint32_t status;
	int offset = chain * 0x20;

	if (chain == 0) {	/* IQ calibration for chain 0. */
		/* IQ calibration settings for chain 0. */
		rtwn_bb_write(sc, 0xe30, 0x10008c1f);
		rtwn_bb_write(sc, 0xe34, 0x10008c1f);
		rtwn_bb_write(sc, 0xe38, 0x82140102);

		if (sc->ntxchains > 1) {
			rtwn_bb_write(sc, 0xe3c, 0x28160202);	/* 2T */
			/* IQ calibration settings for chain 1. */
			rtwn_bb_write(sc, 0xe50, 0x10008c22);
			rtwn_bb_write(sc, 0xe54, 0x10008c22);
			rtwn_bb_write(sc, 0xe58, 0x82140102);
			rtwn_bb_write(sc, 0xe5c, 0x28160202);
		} else
			rtwn_bb_write(sc, 0xe3c, 0x28160502);	/* 1T */

		/* LO calibration settings. */
		rtwn_bb_write(sc, 0xe4c, 0x001028d1);
		/* We're doing LO and IQ calibration in one shot. */
		rtwn_bb_write(sc, 0xe48, 0xf9000000);
		rtwn_bb_write(sc, 0xe48, 0xf8000000);

	} else {		/* IQ calibration for chain 1. */
		/* We're doing LO and IQ calibration in one shot. */
		rtwn_bb_write(sc, 0xe60, 0x00000002);
		rtwn_bb_write(sc, 0xe60, 0x00000000);
	}

	/* Give LO and IQ calibrations the time to complete. */
	rtwn_delay(sc, 1000);

	/* Read IQ calibration status. */
	status = rtwn_bb_read(sc, 0xeac);

	if (status & (1 << (28 + chain * 3)))
		return (0);	/* Tx failed. */
	/* Read Tx IQ calibration results. */
	tx[0] = (rtwn_bb_read(sc, 0xe94 + offset) >> 16) & 0x3ff;
	tx[1] = (rtwn_bb_read(sc, 0xe9c + offset) >> 16) & 0x3ff;
	if (tx[0] == 0x142 || tx[1] == 0x042)
		return (0);	/* Tx failed. */

	if (status & (1 << (27 + chain * 3)))
		return (1);	/* Rx failed. */
	/* Read Rx IQ calibration results. */
	rx[0] = (rtwn_bb_read(sc, 0xea4 + offset) >> 16) & 0x3ff;
	rx[1] = (rtwn_bb_read(sc, 0xeac + offset) >> 16) & 0x3ff;
	if (rx[0] == 0x132 || rx[1] == 0x036)
		return (1);	/* Rx failed. */

	return (3);	/* Both Tx and Rx succeeded. */
}

static void
r92ce_iq_calib_run(struct rtwn_softc *sc, int n, uint16_t tx[2][2],
    uint16_t rx[2][2])
{
	/* Registers to save and restore during IQ calibration. */
	struct iq_cal_regs {
		uint32_t	adda[16];
		uint8_t		txpause;
		uint8_t		bcn_ctrl;
		uint8_t		ustime_tsf;
		uint32_t	gpio_muxcfg;
		uint32_t	ofdm0_trxpathena;
		uint32_t	ofdm0_trmuxpar;
		uint32_t	fpga0_rfifacesw1;
	} iq_cal_regs;
	static const uint16_t reg_adda[16] = {
		0x85c, 0xe6c, 0xe70, 0xe74,
		0xe78, 0xe7c, 0xe80, 0xe84,
		0xe88, 0xe8c, 0xed0, 0xed4,
		0xed8, 0xedc, 0xee0, 0xeec
	};
	int i, chain;
	uint32_t hssi_param1;

	if (n == 0) {
		for (i = 0; i < nitems(reg_adda); i++)
			iq_cal_regs.adda[i] = rtwn_bb_read(sc, reg_adda[i]);

		iq_cal_regs.txpause = rtwn_read_1(sc, R92C_TXPAUSE);
		iq_cal_regs.bcn_ctrl = rtwn_read_1(sc, R92C_BCN_CTRL(0));
		iq_cal_regs.ustime_tsf = rtwn_read_1(sc, R92C_USTIME_TSF);
		iq_cal_regs.gpio_muxcfg = rtwn_read_4(sc, R92C_GPIO_MUXCFG);
	}

	if (sc->ntxchains == 1) {
		rtwn_bb_write(sc, reg_adda[0], 0x0b1b25a0);
		for (i = 1; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], 0x0bdb25a0);
	} else {
		for (i = 0; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], 0x04db25a4);
	}

	hssi_param1 = rtwn_bb_read(sc, R92C_HSSI_PARAM1(0));
	if (!(hssi_param1 & R92C_HSSI_PARAM1_PI)) {
		rtwn_bb_write(sc, R92C_HSSI_PARAM1(0),
		    hssi_param1 | R92C_HSSI_PARAM1_PI);
		rtwn_bb_write(sc, R92C_HSSI_PARAM1(1),
		    hssi_param1 | R92C_HSSI_PARAM1_PI);
	}

	if (n == 0) {
		iq_cal_regs.ofdm0_trxpathena =
		    rtwn_bb_read(sc, R92C_OFDM0_TRXPATHENA);
		iq_cal_regs.ofdm0_trmuxpar =
		    rtwn_bb_read(sc, R92C_OFDM0_TRMUXPAR);
		iq_cal_regs.fpga0_rfifacesw1 =
		    rtwn_bb_read(sc, R92C_FPGA0_RFIFACESW(1));
	}

	rtwn_bb_write(sc, R92C_OFDM0_TRXPATHENA, 0x03a05600);
	rtwn_bb_write(sc, R92C_OFDM0_TRMUXPAR, 0x000800e4);
	rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(1), 0x22204000);
	if (sc->ntxchains > 1) {
		rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00010000);
		rtwn_bb_write(sc, R92C_LSSI_PARAM(1), 0x00010000);
	}

	rtwn_write_1(sc, R92C_TXPAUSE,
	    R92C_TX_QUEUE_AC | R92C_TX_QUEUE_MGT | R92C_TX_QUEUE_HIGH);
	rtwn_write_1(sc, R92C_BCN_CTRL(0),
	    iq_cal_regs.bcn_ctrl & ~R92C_BCN_CTRL_EN_BCN);
	rtwn_write_1(sc, R92C_USTIME_TSF, iq_cal_regs.ustime_tsf & ~(0x08));
	rtwn_write_1(sc, R92C_GPIO_MUXCFG,
	    iq_cal_regs.gpio_muxcfg & ~R92C_GPIO_MUXCFG_ENBT);

	rtwn_bb_write(sc, 0x0b68, 0x00080000);
	if (sc->ntxchains > 1)
		rtwn_bb_write(sc, 0x0b6c, 0x00080000);

	rtwn_bb_write(sc, 0x0e28, 0x80800000);
	rtwn_bb_write(sc, 0x0e40, 0x01007c00);
	rtwn_bb_write(sc, 0x0e44, 0x01004800);

	rtwn_bb_write(sc, 0x0b68, 0x00080000);

	for (chain = 0; chain < sc->ntxchains; chain++) {
		if (chain > 0) {
			/* Put chain 0 on standby. */
			rtwn_bb_write(sc, 0x0e28, 0x00);
			rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00010000);
			rtwn_bb_write(sc, 0x0e28, 0x80800000);

			/* Enable chain 1. */
			for (i = 0; i < nitems(reg_adda); i++)
				rtwn_bb_write(sc, reg_adda[i], 0x0b1b25a4);
		}

		/* Run IQ calibration twice. */
		for (i = 0; i < 2; i++) {
			int ret;

			ret = r92ce_iq_calib_chain(sc, chain,
			    tx[chain], rx[chain]);
			if (ret == 0) {
				RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB,
				    "%s: chain %d: Tx failed.\n",
				    __func__, chain);
				tx[chain][0] = 0xff;
				tx[chain][1] = 0xff;
				rx[chain][0] = 0xff;
				rx[chain][1] = 0xff;
			} else if (ret == 1) {
				RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB,
				    "%s: chain %d: Rx failed.\n",
				    __func__, chain);
				rx[chain][0] = 0xff;
				rx[chain][1] = 0xff;
			} else if (ret == 3) {
				RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB,
				    "%s: chain %d: Both Tx and Rx "
				    "succeeded.\n", __func__, chain);
			}
		}

		RTWN_DPRINTF(sc, RTWN_DEBUG_CALIB,
		    "%s: results for run %d chain %d: tx[0] 0x%x, "
		    "tx[1] 0x%x, rx[0] 0x%x, rx[1] 0x%x\n", __func__, n, chain,
		    tx[chain][0], tx[chain][1], rx[chain][0], rx[chain][1]);
	}

	rtwn_bb_write(sc, R92C_OFDM0_TRXPATHENA,
	    iq_cal_regs.ofdm0_trxpathena);
	rtwn_bb_write(sc, R92C_FPGA0_RFIFACESW(1),
	    iq_cal_regs.fpga0_rfifacesw1);
	rtwn_bb_write(sc, R92C_OFDM0_TRMUXPAR, iq_cal_regs.ofdm0_trmuxpar);

	rtwn_bb_write(sc, 0x0e28, 0x00);
	rtwn_bb_write(sc, R92C_LSSI_PARAM(0), 0x00032ed3);
	if (sc->ntxchains > 1)
		rtwn_bb_write(sc, R92C_LSSI_PARAM(1), 0x00032ed3);

	if (n != 0) {
		if (!(hssi_param1 & R92C_HSSI_PARAM1_PI)) {
			rtwn_bb_write(sc, R92C_HSSI_PARAM1(0), hssi_param1);
			rtwn_bb_write(sc, R92C_HSSI_PARAM1(1), hssi_param1);
		}

		for (i = 0; i < nitems(reg_adda); i++)
			rtwn_bb_write(sc, reg_adda[i], iq_cal_regs.adda[i]);

		rtwn_write_1(sc, R92C_TXPAUSE, iq_cal_regs.txpause);
		rtwn_write_1(sc, R92C_BCN_CTRL(0), iq_cal_regs.bcn_ctrl);
		rtwn_write_1(sc, R92C_USTIME_TSF, iq_cal_regs.ustime_tsf);
		rtwn_write_4(sc, R92C_GPIO_MUXCFG, iq_cal_regs.gpio_muxcfg);
	}
}

#define RTWN_IQ_CAL_MAX_TOLERANCE 5
static int
r92ce_iq_calib_compare_results(struct rtwn_softc *sc, uint16_t tx1[2][2],
    uint16_t rx1[2][2], uint16_t tx2[2][2], uint16_t rx2[2][2])
{
	int chain, i, tx_ok[2], rx_ok[2];

	tx_ok[0] = tx_ok[1] = rx_ok[0] = rx_ok[1] = 0;
	for (chain = 0; chain < sc->ntxchains; chain++) {
		for (i = 0; i < 2; i++)	{
			if (tx1[chain][i] == 0xff || tx2[chain][i] == 0xff ||
			    rx1[chain][i] == 0xff || rx2[chain][i] == 0xff)
				continue;

			tx_ok[chain] = (abs(tx1[chain][i] - tx2[chain][i]) <=
			    RTWN_IQ_CAL_MAX_TOLERANCE);

			rx_ok[chain] = (abs(rx1[chain][i] - rx2[chain][i]) <=
			    RTWN_IQ_CAL_MAX_TOLERANCE);
		}
	}

	if (sc->ntxchains > 1)
		return (tx_ok[0] && tx_ok[1] && rx_ok[0] && rx_ok[1]);
	else
		return (tx_ok[0] && rx_ok[0]);
}
#undef RTWN_IQ_CAL_MAX_TOLERANCE

static void
r92ce_iq_calib_write_results(struct rtwn_softc *sc, uint16_t tx[2],
    uint16_t rx[2], int chain)
{
	uint32_t reg, val, x;
	long y, tx_c;

	if (tx[0] == 0xff || tx[1] == 0xff)
		return;

	reg = rtwn_bb_read(sc, R92C_OFDM0_TXIQIMBALANCE(chain));
	val = ((reg >> 22) & 0x3ff);
	x = tx[0];
	if (x & 0x0200)
		x |= 0xfc00;
	reg = (((x * val) >> 8) & 0x3ff);
	rtwn_bb_write(sc, R92C_OFDM0_TXIQIMBALANCE(chain), reg);

	if (((x * val) >> 7) & 0x01)
		rtwn_bb_setbits(sc, R92C_OFDM0_ECCATHRESHOLD, 0, 0x80000000);
	else
		rtwn_bb_setbits(sc, R92C_OFDM0_ECCATHRESHOLD, 0x80000000, 0);

	y = tx[1];
	if (y & 0x00000200)
		y |= 0xfffffc00;
	tx_c = (y * val) >> 8;
	rtwn_bb_setbits(sc, R92C_OFDM0_TXAFE(chain), 0,
	    (((tx_c & 0x3c0) >> 6) << 24) & 0xf0000000);

	rtwn_bb_setbits(sc, R92C_OFDM0_TXIQIMBALANCE(chain), 0,
	    ((tx_c & 0x3f) << 16) & 0x003F0000);

	if (((y * val) >> 7) & 0x01)
		rtwn_bb_setbits(sc, R92C_OFDM0_ECCATHRESHOLD, 0, 0x20000000);
	else
		rtwn_bb_setbits(sc, R92C_OFDM0_ECCATHRESHOLD, 0x20000000, 0);

	if (rx[0] == 0xff || rx[1] == 0xff)
		return;

	reg = rtwn_bb_read(sc, R92C_OFDM0_RXIQIMBALANCE(chain));
	reg |= (rx[0] & 0x3ff);
	rtwn_bb_write(sc, R92C_OFDM0_RXIQIMBALANCE(chain), reg);
	reg |= (((rx[1] & 0x03f) << 8) & 0xFC00);
	rtwn_bb_write(sc, R92C_OFDM0_RXIQIMBALANCE(chain), reg);

	if (chain == 0) {
		rtwn_bb_setbits(sc, R92C_OFDM0_RXIQEXTANTA, 0,
		    ((rx[1] & 0xf) >> 6) & 0x000f);
	} else {
		rtwn_bb_setbits(sc, R92C_OFDM0_AGCRSSITABLE, 0,
		    (((rx[1] & 0xf) >> 6) << 12) & 0xf000);
	}
}

#define RTWN_IQ_CAL_NRUN	3
void
r92ce_iq_calib(struct rtwn_softc *sc)
{
	uint16_t tx[RTWN_IQ_CAL_NRUN][2][2], rx[RTWN_IQ_CAL_NRUN][2][2];
	int n, valid;

	valid = 0;
	for (n = 0; n < RTWN_IQ_CAL_NRUN; n++) {
		r92ce_iq_calib_run(sc, n, tx[n], rx[n]);

		if (n == 0)
			continue;

		/* Valid results remain stable after consecutive runs. */
		valid = r92ce_iq_calib_compare_results(sc, tx[n - 1],
		    rx[n - 1], tx[n], rx[n]);
		if (valid)
			break;
	}

	if (valid) {
		r92ce_iq_calib_write_results(sc, tx[n][0], rx[n][0], 0);
		if (sc->ntxchains > 1)
			r92ce_iq_calib_write_results(sc, tx[n][1], rx[n][1], 1);
	}
}
#undef RTWN_IQ_CAL_NRUN
