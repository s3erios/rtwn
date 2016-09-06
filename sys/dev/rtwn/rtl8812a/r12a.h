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

#ifndef RTL8812A_H
#define RTL8812A_H

/*
 * Global definitions.
 */
#define R12A_PUBQ_NPAGES	219
#define R12A_TXPKTBUF_COUNT	255
#define R12A_TX_PAGE_COUNT	248

#define R12A_TX_PAGE_SIZE	512
#define R12A_RX_DMA_BUFFER_SIZE	0x3e80

#define R12A_MAX_FW_SIZE	0x8000
#define R12A_MACID_MAX		127
#define R12A_CAM_ENTRY_COUNT	64

#define	R12A_INTR_MSG_LEN	60

static const uint8_t r12a_chan_5ghz[] =
	{ 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64,
	  100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124,
	  126, 128, 130, 132, 134, 136, 138, 140, 142, 144,
	  149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 168, 169, 171,
	  173, 175, 177 };


/*
 * Function declarations.
 */
/* r12a_attach.c */
void	r12a_vap_preattach(struct rtwn_softc *, struct ieee80211vap *);
void	r12a_detach_private(struct rtwn_softc *);

/* r12a_beacon.c */
void	r12a_beacon_init(struct rtwn_softc *, void *, int);
void	r12a_beacon_set_rate(void *, int);

/* r12a_calib.c */
void	r12a_save_bb_afe_vals(struct rtwn_softc *, uint32_t[],
	    const uint16_t[], int);
void	r12a_restore_bb_afe_vals(struct rtwn_softc *, uint32_t[],
	    const uint16_t[], int);
void	r12a_save_rf_vals(struct rtwn_softc *, uint32_t[],
	    const uint8_t[], int);
void	r12a_restore_rf_vals(struct rtwn_softc *, uint32_t[],
	    const uint8_t[], int);
void	r12a_lc_calib(struct rtwn_softc *);
#ifndef RTWN_WITHOUT_UCODE
int	r12a_iq_calib_fw_supported(struct rtwn_softc *);
#endif
void	r12a_iq_calib_sw(struct rtwn_softc *);
void	r12a_iq_calib(struct rtwn_softc *);

/* r12a_caps.c */
int	r12a_ioctl_net(struct ieee80211com *, u_long, void *);

/* r12a_chan.c */
void	r12a_fix_spur(struct rtwn_softc *, struct ieee80211_channel *);
void	r12a_set_chan(struct rtwn_softc *, struct ieee80211_channel *);
void	r12a_set_band_2ghz(struct rtwn_softc *, uint32_t);
void	r12a_set_band_5ghz(struct rtwn_softc *, uint32_t);

/* r12a_fw.c */
#ifndef RTWN_WITHOUT_UCODE
void	r12a_fw_reset(struct rtwn_softc *, int);
void	r12a_fw_download_enable(struct rtwn_softc *, int);
void	r12a_set_media_status(struct rtwn_softc *, int);
int	r12a_set_pwrmode(struct rtwn_softc *, struct ieee80211vap *,
	    int);
void	r12a_iq_calib_fw(struct rtwn_softc *);
#endif

/* r12a_init.c */
int	r12a_check_condition(struct rtwn_softc *, const uint8_t[]);
int	r12a_set_page_size(struct rtwn_softc *);
void	r12a_init_edca(struct rtwn_softc *);
void	r12a_init_bb(struct rtwn_softc *);
void	r12a_init_rf(struct rtwn_softc *);
void	r12a_crystalcap_write(struct rtwn_softc *);
int	r12a_power_on(struct rtwn_softc *);
void	r12a_power_off(struct rtwn_softc *);
void	r12a_init_intr(struct rtwn_softc *);
void	r12a_init_antsel(struct rtwn_softc *);

/* r12a_led.c */
void	r12a_set_led(struct rtwn_softc *, int, int);

/* r12a_rf.c */
uint32_t	r12a_rf_read(struct rtwn_softc *, int, uint8_t);
uint32_t	r12a_c_cut_rf_read(struct rtwn_softc *, int, uint8_t);
void		r12a_rf_write(struct rtwn_softc *, int, uint8_t, uint32_t);

/* r12a_rom.c */
void	r12a_parse_rom_common(struct rtwn_softc *, uint8_t *);
void	r12a_parse_rom(struct rtwn_softc *, uint8_t *);

/* r12a_rx.c */
void	r12a_ratectl_tx_complete(struct rtwn_softc *, uint8_t *, int);
void	r12a_handle_c2h_report(struct rtwn_softc *, uint8_t *, int);
int	r12a_check_frame_checksum(struct rtwn_softc *, struct mbuf *);
int	r12a_rx_sgi_isset(void *);

/* r12a_tx.c */
void	r12a_fill_tx_desc(struct rtwn_softc *, struct ieee80211_node *,
	    struct mbuf *, void *, uint8_t, int);
void	r12a_fill_tx_desc_raw(struct rtwn_softc *, struct ieee80211_node *,
	    struct mbuf *, void *, const struct ieee80211_bpf_params *);
void	r12a_fill_tx_desc_null(struct rtwn_softc *, void *, int, int, int);
int	r12a_tx_sgi_isset(void *);

#endif	/* RTL8812A_H */
