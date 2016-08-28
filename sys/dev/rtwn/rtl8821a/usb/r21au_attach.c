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
 *
 * $FreeBSD$
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
#include <dev/rtwn/if_rtwn_nop.h>

#include <dev/rtwn/usb/rtwn_usb_var.h>

#include <dev/rtwn/rtl8192c/r92c.h>

#include <dev/rtwn/rtl8188e/r88e.h>

#include <dev/rtwn/rtl8812a/r12a_var.h>
#include <dev/rtwn/rtl8812a/r12a_rom_defs.h>

#include <dev/rtwn/rtl8812a/usb/r12au.h>
#include <dev/rtwn/rtl8812a/usb/r12au_tx_desc.h>

#include <dev/rtwn/rtl8821a/r21a_reg.h>
#include <dev/rtwn/rtl8821a/r21a_priv.h>

#include <dev/rtwn/rtl8821a/usb/r21au.h>


void	r21au_attach(struct rtwn_usb_softc *);

static void
r21a_postattach(struct rtwn_softc *sc)
{
	struct r12a_softc *rs = sc->sc_priv;

	if (rs->board_type == R92C_BOARD_TYPE_MINICARD ||
	    rs->board_type == R92C_BOARD_TYPE_SOLO ||
	    rs->board_type == R92C_BOARD_TYPE_COMBO)
		sc->sc_set_led = r12a_set_led_mini;
	else
		sc->sc_set_led = r21a_set_led;

	sc->sc_ic.ic_ioctl = r12a_ioctl_net;
}

static void
r21a_attach_private(struct rtwn_softc *sc)
{
	struct r12a_softc *rs;

	rs = malloc(sizeof(struct r12a_softc), M_TEMP, M_WAITOK | M_ZERO);

	rs->rs_flags			= R12A_RXCKSUM_EN | R12A_RXCKSUM6_EN;

	rs->rs_fix_spur			= rtwn_nop_softc_chan;
	rs->rs_set_band_2ghz		= r21a_set_band_2ghz;
	rs->rs_set_band_5ghz		= r21a_set_band_5ghz;
	rs->rs_init_burstlen		= r21au_init_burstlen;
	rs->rs_init_ampdu_fwhw		= r21a_init_ampdu_fwhw;
	rs->rs_crystalcap_write		= r21a_crystalcap_write;
#ifndef RTWN_WITHOUT_UCODE
	rs->rs_iq_calib_fw_supported	= r21a_iq_calib_fw_supported;
#endif
	rs->rs_iq_calib_sw		= r21a_iq_calib_sw;

	rs->ampdu_max_time		= 0x5e;

	rs->ac_usb_dma_size		= 0x01;
	rs->ac_usb_dma_time		= 0x10;

	sc->sc_priv			= rs;
}

static void
r21au_adj_devcaps(struct rtwn_softc *sc)
{
	/* TODO: DFS, LDPC etc */
}

void
r21au_attach(struct rtwn_usb_softc *uc)
{
	struct rtwn_softc *sc		= &uc->uc_sc;

	uc->uc_align_rx			= r12au_align_rx;
	uc->uc_tx_checksum		= r12au_tx_checksum;

	sc->sc_flags			= RTWN_FLAG_EXT_HDR;

	sc->sc_rf_read			= r12a_c_cut_rf_read;
	sc->sc_rf_write			= r12a_rf_write;
	sc->sc_check_condition		= r21a_check_condition;
	sc->sc_efuse_postread		= rtwn_nop_softc;
	sc->sc_parse_rom		= r21a_parse_rom;
	sc->sc_get_rssi_cck		= r21a_get_rssi_cck;
	sc->sc_get_rssi_ofdm		= r88e_get_rssi_ofdm;
	sc->sc_classify_intr		= r12au_classify_intr;
	sc->sc_handle_tx_report		= r12a_ratectl_tx_complete;
	sc->sc_handle_c2h_report	= r12a_handle_c2h_report;
	sc->sc_check_frame		= r12a_check_frame_checksum;
	sc->sc_power_on			= r21a_power_on;
	sc->sc_power_off		= r21a_power_off;
#ifndef RTWN_WITHOUT_UCODE
	sc->sc_fw_reset			= r21a_fw_reset;
#endif
	sc->sc_set_page_size 		= rtwn_nop_int_softc;
	sc->sc_lc_calib			= rtwn_nop_softc;	/* XXX not used */
	sc->sc_iq_calib			= r12a_iq_calib;
	sc->sc_read_chipid_vendor	= rtwn_nop_softc_uint32;
	sc->sc_adj_devcaps		= r21au_adj_devcaps;
	sc->sc_vap_preattach		= r12a_vap_preattach;
	sc->sc_postattach		= r21a_postattach;
	sc->sc_detach_private		= r12a_detach_private;
	sc->sc_fill_tx_desc		= r12a_fill_tx_desc;
	sc->sc_fill_tx_desc_raw		= r12a_fill_tx_desc_raw;
	sc->sc_fill_tx_desc_null	= r12au_fill_tx_desc_null;
	sc->sc_sgi_isset		= r12a_sgi_isset;
	sc->sc_set_chan			= r12a_set_chan;
#ifndef RTWN_WITHOUT_UCODE
	sc->sc_set_media_status		= r12a_set_media_status;
	sc->sc_set_rsvd_page		= r88e_set_rsvd_page;
	sc->sc_set_pwrmode		= r12a_set_pwrmode;
	sc->sc_set_rssi			= rtwn_nop_softc;	/* XXX TODO */
#else
	sc->sc_set_media_status		= rtwn_nop_softc_uint8;
#endif
	sc->sc_beacon_init		= r21a_beacon_init;
	sc->sc_beacon_enable		= r92c_beacon_enable;
	sc->sc_beacon_set_rate		= r12a_beacon_set_rate;
	sc->sc_beacon_select		= r21a_beacon_select;
	sc->sc_temp_measure		= r88e_temp_measure;
	sc->sc_temp_read		= r88e_temp_read;
	sc->sc_init_tx_agg		= r21au_init_tx_agg;
	sc->sc_init_rx_agg 		= r12au_init_rx_agg;
	sc->sc_init_ampdu		= r12au_init_ampdu;
	sc->sc_init_intr		= r12a_init_intr;
	sc->sc_init_rf_workaround	= rtwn_nop_softc;
	sc->sc_init_edca		= r12a_init_edca;
	sc->sc_init_bb			= r12a_init_bb;
	sc->sc_init_rf			= r12a_init_rf;
	sc->sc_init_antsel		= r12a_init_antsel;
	sc->sc_post_init		= r12au_post_init;
	sc->sc_init_bcnq1_boundary	= r21a_init_bcnq1_boundary;

	sc->chan_list_5ghz		= r12a_chan_5ghz;
	sc->chan_num_5ghz		= nitems(r12a_chan_5ghz);

	sc->mac_prog			= &rtl8821au_mac[0];
	sc->mac_size			= nitems(rtl8821au_mac);
	sc->bb_prog			= &rtl8821au_bb[0];
	sc->bb_size			= nitems(rtl8821au_bb);
	sc->agc_prog			= &rtl8821au_agc[0];
	sc->agc_size			= nitems(rtl8821au_agc);
	sc->rf_prog			= &rtl8821au_rf[0];

	sc->name			= "RTL8821AU";
	sc->fwname			= "rtwn-rtl8821aufw";
	sc->fwsig			= 0x210;

	sc->page_count			= R21A_TX_PAGE_COUNT;
	sc->pktbuf_count		= R12A_TXPKTBUF_COUNT;
	sc->tx_boundary			= R21A_TX_PAGE_BOUNDARY;
	sc->ackto			= 0x80;
	sc->npubqpages			= R12A_PUBQ_NPAGES;
	sc->page_size			= R21A_TX_PAGE_SIZE;

	sc->txdesc_len			= sizeof(struct r12au_tx_desc);
	sc->efuse_maxlen		= R12A_EFUSE_MAX_LEN;
	sc->efuse_maplen		= R12A_EFUSE_MAP_LEN;
	sc->rx_dma_size			= R12A_RX_DMA_BUFFER_SIZE;

	sc->macid_limit			= R12A_MACID_MAX + 1;
	sc->cam_entry_limit		= R12A_CAM_ENTRY_COUNT;
	sc->fwsize_limit		= R12A_MAX_FW_SIZE;

	sc->tx_agg_desc_num		= 6;

	sc->bcn_status_reg[0]		= R92C_TDECTRL;
	sc->bcn_status_reg[1]		= R21A_DWBCN1_CTRL;
	sc->rcr				= R12A_RCR_DIS_CHK_14 |
					  R12A_RCR_VHT_ACK |
					  R12A_RCR_TCP_OFFLD_EN;

	sc->ntxchains			= 1;
	sc->nrxchains			= 1;

	r21a_attach_private(sc);
}

