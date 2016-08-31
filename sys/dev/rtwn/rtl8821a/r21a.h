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

#ifndef RTL8821A_H
#define RTL8821A_H

/*
 * Global definitions.
 */
#define R21A_TX_PAGE_COUNT	243
#define R21A_BCNQ0_PAGE_COUNT	8
#define R21A_BCNQ0_BOUNDARY	\
	(R21A_TX_PAGE_COUNT + R21A_BCNQ0_PAGE_COUNT + 1)

#define R21A_TX_PAGE_SIZE	256


/*
 * Function declarations.
 */
/* r21a_beacon.c */
void	r21a_beacon_init(struct rtwn_softc *, void *, int);
void	r21a_beacon_select(struct rtwn_softc *, int);

/* r21a_calib.c */
#ifndef RTWN_WITHOUT_UCODE
int	r21a_iq_calib_fw_supported(struct rtwn_softc *);
#endif
void	r21a_iq_calib_sw(struct rtwn_softc *);

/* r21a_chan.c */
void	r21a_set_band_2ghz(struct rtwn_softc *, uint32_t);
void	r21a_set_band_5ghz(struct rtwn_softc *, uint32_t);

/* r21a_fw.c */
void	r21a_fw_reset(struct rtwn_softc *, int);

/* r21a_init.c */
int	r21a_power_on(struct rtwn_softc *);
void	r21a_power_off(struct rtwn_softc *);
int	r21a_check_condition(struct rtwn_softc *, const uint8_t[]);
void	r21a_crystalcap_write(struct rtwn_softc *);
int	r21a_init_bcnq1_boundary(struct rtwn_softc *);
void	r21a_init_ampdu_fwhw(struct rtwn_softc *);

/* r21a_led.c */
void	r21a_set_led(struct rtwn_softc *, int, int);

/* r21a_rom.c */
void	r21a_parse_rom(struct rtwn_softc *, uint8_t *);

/* r21a_rx.c */
int8_t	r21a_get_rssi_cck(struct rtwn_softc *, void *);

#endif	/* RTL8821A_H */
