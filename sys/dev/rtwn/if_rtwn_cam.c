/*	$OpenBSD: if_urtwn.c,v 1.16 2011/02/10 17:26:40 jakemsr Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2014 Kevin Lo <kevlo@FreeBSD.org>
 * Copyright (c) 2015-2016 Andriy Voskoboinyk <avos@FreeBSD.org>
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

#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/rtwn/if_rtwnreg.h>
#include <dev/rtwn/if_rtwnvar.h>

#include <dev/rtwn/if_rtwn_cam.h>
#include <dev/rtwn/if_rtwn_debug.h>
#include <dev/rtwn/if_rtwn_task.h>

#include <dev/rtwn/rtl8192c/r92c_reg.h>


void
rtwn_cam_init(struct rtwn_softc *sc)
{
	/* Invalidate all CAM entries. */
	rtwn_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_CLR);
}

static int
rtwn_cam_write(struct rtwn_softc *sc, uint32_t addr, uint32_t data)
{
	int error;

	error = rtwn_write_4(sc, R92C_CAMWRITE, data);
	if (error != 0)
		return (error);
	error = rtwn_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_WRITE |
	    SM(R92C_CAMCMD_ADDR, addr));

	return (error);
}

int
rtwn_key_alloc(struct ieee80211vap *vap, struct ieee80211_key *k,
    ieee80211_keyix *keyix, ieee80211_keyix *rxkeyix)
{
	struct rtwn_softc *sc = vap->iv_ic->ic_softc;
	uint8_t i;

	if (!(&vap->iv_nw_keys[0] <= k &&
	     k < &vap->iv_nw_keys[IEEE80211_WEP_NKID])) {
		if (!(k->wk_flags & IEEE80211_KEY_SWCRYPT)) {
			RTWN_LOCK(sc);
			/*
			 * Current slot usage:
			 * everything for pairwise keys.
			 */
			for (i = 0; i < sc->cam_entry_limit; i++) {
				if (isclr(sc->keys_bmap, i)) {
					setbit(sc->keys_bmap, i);
					*keyix = i;
					break;
				}
			}
			RTWN_UNLOCK(sc);
			if (i == sc->cam_entry_limit) {
				device_printf(sc->sc_dev,
				    "%s: no free space in the key table\n",
				    __func__);
				return 0;
			}
		} else
			*keyix = 0;
	} else {
		*keyix = k - vap->iv_nw_keys;
		/* XXX will be overridden during group key handshake */
		k->wk_flags |= IEEE80211_KEY_SWCRYPT;
	}
	*rxkeyix = *keyix;
	return 1;
}

static void
rtwn_key_set_cb(struct rtwn_softc *sc, union sec_param *data)
{
	struct ieee80211_key *k = &data->key;
	uint8_t algo, keyid;
	int i, error;

	keyid = 0;

	/* Map net80211 cipher to HW crypto algorithm. */
	switch (k->wk_cipher->ic_cipher) {
	case IEEE80211_CIPHER_WEP:
		if (k->wk_keylen < 8)
			algo = R92C_CAM_ALGO_WEP40;
		else
			algo = R92C_CAM_ALGO_WEP104;
		break;
	case IEEE80211_CIPHER_TKIP:
		algo = R92C_CAM_ALGO_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		algo = R92C_CAM_ALGO_AES;
		break;
	default:
		device_printf(sc->sc_dev, "%s: unknown cipher %d\n",
		    __func__, k->wk_cipher->ic_cipher);
		return;
	}

	RTWN_DPRINTF(sc, RTWN_DEBUG_KEY,
	    "%s: keyix %d, keyid %d, algo %d/%d, flags %04X, len %d, "
	    "macaddr %s\n", __func__, k->wk_keyix, keyid,
	    k->wk_cipher->ic_cipher, algo, k->wk_flags, k->wk_keylen,
	    ether_sprintf(k->wk_macaddr));

	/* Clear high bits. */
	rtwn_cam_write(sc, R92C_CAM_CTL6(k->wk_keyix), 0);
	rtwn_cam_write(sc, R92C_CAM_CTL7(k->wk_keyix), 0);

	/* Write key. */
	for (i = 0; i < 4; i++) {
		error = rtwn_cam_write(sc, R92C_CAM_KEY(k->wk_keyix, i),
		    le32dec(&k->wk_key[i * 4]));
		if (error != 0)
			goto fail;
	}

	/* Write CTL0 last since that will validate the CAM entry. */
	error = rtwn_cam_write(sc, R92C_CAM_CTL1(k->wk_keyix),
	    le32dec(&k->wk_macaddr[2]));
	if (error != 0)
		goto fail;
	error = rtwn_cam_write(sc, R92C_CAM_CTL0(k->wk_keyix),
	    SM(R92C_CAM_ALGO, algo) |
	    SM(R92C_CAM_KEYID, keyid) |
	    SM(R92C_CAM_MACLO, le16dec(&k->wk_macaddr[0])) |
	    R92C_CAM_VALID);
	if (error != 0)
		goto fail;

	return;

fail:
	device_printf(sc->sc_dev, "%s fails, error %d\n", __func__, error);
}

static void
rtwn_key_del_cb(struct rtwn_softc *sc, union sec_param *data)
{
	struct ieee80211_key *k = &data->key;
	int i;

	RTWN_DPRINTF(sc, RTWN_DEBUG_KEY,
	    "%s: keyix %d, flags %04X, macaddr %s\n", __func__,
	    k->wk_keyix, k->wk_flags, ether_sprintf(k->wk_macaddr));

	rtwn_cam_write(sc, R92C_CAM_CTL0(k->wk_keyix), 0);
	rtwn_cam_write(sc, R92C_CAM_CTL1(k->wk_keyix), 0);

	/* Clear key. */
	for (i = 0; i < 4; i++)
		rtwn_cam_write(sc, R92C_CAM_KEY(k->wk_keyix, i), 0);
	clrbit(sc->keys_bmap, k->wk_keyix);
}

static int
rtwn_process_key(struct ieee80211vap *vap, const struct ieee80211_key *k,
    int set)
{
	struct rtwn_softc *sc = vap->iv_ic->ic_softc;

	if (k->wk_flags & IEEE80211_KEY_SWCRYPT) {
		/* Not for us. */
		return (1);
	}

	if (&vap->iv_nw_keys[0] <= k &&
	    k < &vap->iv_nw_keys[IEEE80211_WEP_NKID]) {
		struct ieee80211_key *k1 = &vap->iv_nw_keys[k->wk_keyix];

		/*
		 * Do not offload group keys;
		 * multicast key search is too problematic with 2+ vaps.
		 */
		/* XXX fix net80211 instead */
		k1->wk_flags |= IEEE80211_KEY_SWCRYPT;
		return (k->wk_cipher->ic_setkey(k1));
	}

	return (!rtwn_cmd_sleepable(sc, k, sizeof(*k),
	    set ? rtwn_key_set_cb : rtwn_key_del_cb));
}

int
rtwn_key_set(struct ieee80211vap *vap, const struct ieee80211_key *k)
{
	return (rtwn_process_key(vap, k, 1));
}

int
rtwn_key_delete(struct ieee80211vap *vap, const struct ieee80211_key *k)
{
	return (rtwn_process_key(vap, k, 0));
}
