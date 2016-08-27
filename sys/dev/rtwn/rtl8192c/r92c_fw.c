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
#include <dev/rtwn/if_rtwn_task.h>
#include <dev/rtwn/if_rtwn_tx.h>

#include <dev/rtwn/rtl8192c/r92c.h>
#include <dev/rtwn/rtl8192c/r92c_reg.h>
#include <dev/rtwn/rtl8192c/r92c_var.h>
#include <dev/rtwn/rtl8192c/r92c_fw_cmd.h>


#ifndef RTWN_WITHOUT_UCODE
static int
r92c_fw_cmd(struct rtwn_softc *sc, uint8_t id, const void *buf, int len)
{
	struct r92c_fw_cmd cmd;
	int ntries, error;

	KASSERT(len <= sizeof(cmd.msg),
	    ("%s: firmware command too long (%d > %d)\n",
	    __func__, len, sizeof(cmd.msg)));

	if (!(sc->sc_flags & RTWN_FW_LOADED)) {
		RTWN_DPRINTF(sc, RTWN_DEBUG_FIRMWARE, "%s: firmware "
		    "was not loaded; command (id %d) will be discarded\n",
		    __func__, id);
		return (0);
	}

	/* Wait for current FW box to be empty. */
	for (ntries = 0; ntries < 50; ntries++) {
		if (!(rtwn_read_1(sc, R92C_HMETFR) & (1 << sc->fwcur)))
			break;
		rtwn_delay(sc, 2000);
	}
	if (ntries == 100) {
		device_printf(sc->sc_dev,
		    "could not send firmware command\n");
		return (ETIMEDOUT);
	}
	memset(&cmd, 0, sizeof(cmd));
	cmd.id = id;
	if (len > 3) {
		/* Ext command: [id : byte2 : byte3 : byte4 : byte0 : byte1] */
		cmd.id |= R92C_CMD_FLAG_EXT;
		memcpy(cmd.msg, (const uint8_t *)buf + 2, len - 2);
		memcpy(cmd.msg + 3, buf, 2);
	} else
		memcpy(cmd.msg, buf, len);

	/* Write the first word last since that will trigger the FW. */
	if (len > 3) {
		error = rtwn_write_2(sc, R92C_HMEBOX_EXT(sc->fwcur),
		    *(uint16_t *)((uint8_t *)&cmd + 4));
		if (error != 0)
			return (error);
	}
	error = rtwn_write_4(sc, R92C_HMEBOX(sc->fwcur),
	    *(uint32_t *)&cmd);
	if (error != 0)
		return (error);

	sc->fwcur = (sc->fwcur + 1) % R92C_H2C_NBOX;

	return (0);
}

void
r92c_fw_reset(struct rtwn_softc *sc, int reason)
{
	int ntries;

	if (reason == RTWN_FW_RESET_CHECKSUM)
		return;

	/* Tell 8051 to reset itself. */
	rtwn_write_1(sc, R92C_HMETFR + 3, 0x20);

	/* Wait until 8051 resets by itself. */
	for (ntries = 0; ntries < 100; ntries++) {
		if ((rtwn_read_2(sc, R92C_SYS_FUNC_EN) &
		    R92C_SYS_FUNC_EN_CPUEN) == 0)
			return;
		rtwn_delay(sc, 50);
	}
	/* Force 8051 reset. */
	rtwn_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    R92C_SYS_FUNC_EN_CPUEN, 0, 1);
}
#endif

static void
r92c_get_rates(struct ieee80211_node *ni, uint32_t *rates, int *maxrate,
    uint32_t *basicrates, int *maxbasicrate)
{
	struct ieee80211_rateset *rs, *rs_ht;
	uint8_t ridx;
	int i;

	rs = &ni->ni_rates;
	rs_ht = (struct ieee80211_rateset *) &ni->ni_htrates;

	/* Get normal and basic rates mask. */
	*rates = *basicrates = 0;
	*maxrate = *maxbasicrate = 0;

	/* This is for 11bg */
	for (i = 0; i < rs->rs_nrates; i++) {
		/* Convert 802.11 rate to HW rate index. */
		ridx = rate2ridx(IEEE80211_RV(rs->rs_rates[i]));
		if (ridx == RTWN_RIDX_UNKNOWN)	/* Unknown rate, skip. */
			continue;
		*rates |= 1 << ridx;
		if (ridx > *maxrate)
			*maxrate = ridx;
		if (rs->rs_rates[i] & IEEE80211_RATE_BASIC) {
			*basicrates |= 1 << ridx;
			if (ridx > *maxbasicrate)
				*maxbasicrate = ridx;
		}
	}

	/* If we're doing 11n, enable 11n rates */
	if (ni->ni_flags & IEEE80211_NODE_HT) {
		for (i = 0; i < rs_ht->rs_nrates; i++) {
			if ((rs_ht->rs_rates[i] & 0x7f) > 0xf)
				continue;
			/* 11n rates start at index 12 */
			ridx = ((rs_ht->rs_rates[i]) & 0xf) + RTWN_RIDX_MCS(0);
			*rates |= (1 << ridx);

			/* Guard against the rate table being oddly ordered */
			if (ridx > *maxrate)
				*maxrate = ridx;
		}
	}
}

static void
r92c_init_mrr_rate(struct rtwn_softc *sc, int macid)
{
	struct ieee80211_node *ni;
	uint32_t rates, basicrates;
	int maxrate, maxbasicrate;

	RTWN_NT_LOCK(sc);
	if (sc->node_list[macid] == NULL) {
		RTWN_DPRINTF(sc, RTWN_DEBUG_RA, "%s: macid %d, ni is NULL\n",
		    __func__, macid);
		RTWN_NT_UNLOCK(sc);
		return;
	}

	ni = ieee80211_ref_node(sc->node_list[macid]);
	r92c_get_rates(ni, &rates, &maxrate, &basicrates, &maxbasicrate);
	RTWN_NT_UNLOCK(sc);

	/* TODO: f/w rate adaptation */

	rtwn_write_1(sc, R92C_INIDATA_RATE_SEL(macid), maxrate);

	ieee80211_free_node(ni);
}

void
r92c_joinbss_rpt(struct rtwn_softc *sc, int macid)
{
#ifndef RTWN_WITHOUT_UCODE
	struct r92c_softc *rs = sc->sc_priv;
	struct ieee80211vap *vap;
	struct r92c_fw_cmd_joinbss_rpt cmd;
#endif

	/* TODO: init rates for RTWN_MACID_BC. */
	if (macid & RTWN_MACID_VALID)
		r92c_init_mrr_rate(sc, macid & ~RTWN_MACID_VALID);

#ifndef RTWN_WITHOUT_UCODE
	if (sc->vaps[0] == NULL)	/* XXX fix */
		return;

	vap = &sc->vaps[0]->vap;
	if ((vap->iv_state == IEEE80211_S_RUN) ^
	    !(rs->rs_flags & R92C_FLAG_ASSOCIATED))
		return;

	if (rs->rs_flags & R92C_FLAG_ASSOCIATED) {
		cmd.mstatus = R92C_MSTATUS_DISASSOC;
		rs->rs_flags &= ~R92C_FLAG_ASSOCIATED;
	} else {
		cmd.mstatus = R92C_MSTATUS_ASSOC;
		rs->rs_flags |= R92C_FLAG_ASSOCIATED;
	}

	if (r92c_fw_cmd(sc, R92C_CMD_JOINBSS_RPT, &cmd, sizeof(cmd)) != 0) {
		device_printf(sc->sc_dev, "%s: cannot change media status!\n",
		    __func__);
	}
#endif
}

#ifndef RTWN_WITHOUT_UCODE
int
r92c_set_rsvd_page(struct rtwn_softc *sc, int probe_resp, int null,
    int qos_null)
{
	struct r92c_fw_cmd_rsvdpage rsvd;

	rsvd.probe_resp = probe_resp;
	rsvd.ps_poll = 0;
	rsvd.null_data = null;

	return (r92c_fw_cmd(sc, R92C_CMD_RSVD_PAGE, &rsvd, sizeof(rsvd)));
}

int
r92c_set_pwrmode(struct rtwn_softc *sc, struct ieee80211vap *vap,
    int off)
{
	struct r92c_fw_cmd_pwrmode mode;
	int error;

	/* XXX dm_RF_saving */

	if (off && vap->iv_state == IEEE80211_S_RUN &&
	    (vap->iv_flags & IEEE80211_F_PMGTON))
		mode.mode = R92C_PWRMODE_MIN;
	else
		mode.mode = R92C_PWRMODE_CAM;
	mode.smart_ps = R92C_PWRMODE_SMARTPS_NULLDATA;
	mode.bcn_pass = 1;	/* XXX */
	error = r92c_fw_cmd(sc, R92C_CMD_SET_PWRMODE, &mode, sizeof(mode));
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "%s: CMD_SET_PWRMODE was not sent, error %d\n",
		    __func__, error);
	}

	return (error);
}

#ifdef RTWN_TODO
/* XXX should be called from joinbss_rpt() */
/*
 * Initialize firmware rate adaptation.
 */
static int
rtwn_ra_init(struct rtwn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	struct ieee80211_node *ni;
	struct r92c_fw_cmd_macid_cfg cmd;
	uint32_t rates, basicrates;
	uint8_t mode;
	int maxrate, maxbasicrate, error = 0, i;

	ni = ieee80211_ref_node(vap->iv_bss);

	/* Get normal and basic rates mask. */
	r92c_get_rates(ni, &rates, &maxrate, &basicrates, &maxbasicrate);

	/* NB: group addressed frames are done at 11bg rates for now */
	if (ic->ic_curmode == IEEE80211_MODE_11B)
		mode = R92C_RAID_11B;
	else
		mode = R92C_RAID_11BG;
	/* XXX misleading 'mode' value here for unicast frames */
	RTWN_DPRINTF(sc, RTWN_DEBUG_RA,
	    "%s: mode 0x%x, rates 0x%08x, basicrates 0x%08x\n", __func__,
	    mode, rates, basicrates);

	/* Set rates mask for group addressed frames. */
	cmd.macid = RTWN_MACID_BC | RTWN_MACID_VALID;
	cmd.mask = htole32(mode << 28 | basicrates);
	error = rtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, &cmd, sizeof(cmd));
	if (error != 0) {
		ieee80211_free_node(ni);
		device_printf(sc->sc_dev,
		    "could not add broadcast station\n");
		return (error);
	}

	/* Set rates mask for unicast frames. */
	if (ni->ni_flags & IEEE80211_NODE_HT)
		mode = R92C_RAID_11GN;
	else if (ic->ic_curmode == IEEE80211_MODE_11B)
		mode = R92C_RAID_11B;
	else
		mode = R92C_RAID_11BG;
	cmd.macid = RTWN_MACID_BSS | RTWN_MACID_VALID;
	cmd.mask = htole32(mode << 28 | rates);
	error = rtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, &cmd, sizeof(cmd));
	if (error != 0) {
		ieee80211_free_node(ni);
		device_printf(sc->sc_dev, "could not add BSS station\n");
		return (error);
	}

	/* Indicate highest supported rate. */
	if (ni->ni_flags & IEEE80211_NODE_HT)
		ni->ni_txrate = rs_ht->rs_rates[rs_ht->rs_nrates - 1]
		    | IEEE80211_RATE_MCS;
	else
		ni->ni_txrate = rs->rs_rates[rs->rs_nrates - 1];
	ieee80211_free_node(ni);

	return (0);
}
#endif

static void
r92c_ratectl_tx_complete(struct rtwn_softc *sc, uint8_t *buf, int len)
{
	struct r92c_c2h_tx_rpt *rpt;
	struct ieee80211vap *vap;
	struct ieee80211_node *ni;
	uint8_t macid;
	int ntries;

	if (sc->sc_ratectl != RTWN_RATECTL_NET80211) {
		/* shouldn't happen */
		device_printf(sc->sc_dev, "%s called while ratectl = %d!\n",
		    __func__, sc->sc_ratectl);
		return;
	}

	rpt = (struct r92c_c2h_tx_rpt *)buf;
	if (len != sizeof(*rpt)) {
		device_printf(sc->sc_dev,
		    "%s: wrong report size (%d, must be %d)\n",
		    __func__, len, sizeof(*rpt));
		return;
	}

	macid = MS(rpt->rptb5, R92C_RPTB5_MACID);
        if (macid > sc->macid_limit) {
                device_printf(sc->sc_dev,
                    "macid %u is too big; increase MACID_MAX limit\n",
                    macid);
                return;
        }

	ntries = MS(rpt->rptb0, R92C_RPTB0_RETRY_CNT);

	RTWN_NT_LOCK(sc);
	ni = sc->node_list[macid];
	if (ni != NULL) {
		vap = ni->ni_vap;
		RTWN_DPRINTF(sc, RTWN_DEBUG_INTR, "%s: frame for macid %d was"
		    "%s sent (%d retries)\n", __func__, macid,
		    (rpt->rptb7 & R92C_RPTB7_PKT_OK) ? "" : " not",
		    ntries);

		if (rpt->rptb7 & R92C_RPTB7_PKT_OK) {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_SUCCESS, &ntries, NULL);
		} else {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_FAILURE, &ntries, NULL);
		}
	} else {
		RTWN_DPRINTF(sc, RTWN_DEBUG_INTR, "%s: macid %d, ni is NULL\n",
		    __func__, macid);
	}
	RTWN_NT_UNLOCK(sc);

#ifdef IEEE80211_SUPPORT_SUPERG
	if (sc->sc_tx_n_active > 0 && --sc->sc_tx_n_active <= 1)
		rtwn_cmd_sleepable(sc, NULL, 0, rtwn_ff_flush_all);
#endif
}

static void
r92c_handle_c2h_task(struct rtwn_softc *sc, union sec_param *data)
{
	const uint16_t off = R92C_C2H_EVT_MSG + sizeof(struct r92c_c2h_evt);
	struct r92c_softc *rs = sc->sc_priv;
	uint8_t buf[R92C_C2H_MSG_MAX_LEN];
	uint8_t id, len, status;
	int i;

	/* Do not reschedule the task if device is detached. */
	if (sc->sc_flags & RTWN_DETACHED)
		return;

	/* Read current status. */
	status = rtwn_read_1(sc, R92C_C2H_EVT_CLEAR);
	if (status == R92C_C2H_EVT_HOST_CLOSE)
		goto end;	/* nothing to do */
	else if (status == R92C_C2H_EVT_FW_CLOSE) {
		len = rtwn_read_1(sc, R92C_C2H_EVT_MSG);
		id = MS(len, R92C_C2H_EVTB0_ID);
		len = MS(len, R92C_C2H_EVTB0_LEN);

		memset(buf, 0, sizeof(buf));
		/* Try to optimize event reads. */
		for (i = 0; i < len; i += 4)
			*(uint32_t *)&buf[i] = rtwn_read_4(sc, off + i);
		KASSERT(i < sizeof(buf), ("%s: buffer overrun (%d >= %d)!",
		    __func__, i, sizeof(buf)));

		switch (id) {
		case R92C_C2H_EVT_TX_REPORT:
			r92c_ratectl_tx_complete(sc, buf, len);
			break;
		default:
			device_printf(sc->sc_dev,
			    "%s: C2H report %d (len %d) was not handled\n",
			    __func__, id, len);
			break;
		}
	}

	/* Prepare for next event. */
	rtwn_write_1(sc, R92C_C2H_EVT_CLEAR, R92C_C2H_EVT_HOST_CLOSE);

end:
	/* Adjust timeout for next call. */
	if (rs->rs_c2h_pending != 0) {
		rs->rs_c2h_pending = 0;
		rs->rs_c2h_paused = 0;
	} else
		rs->rs_c2h_paused++;

	if (rs->rs_c2h_paused > R92C_TX_PAUSED_THRESHOLD)
		rs->rs_c2h_timeout = hz;
	else
		rs->rs_c2h_timeout = MAX(hz / 100, 1);

	/* Reschedule the task. */
	callout_reset(&rs->rs_c2h_report, rs->rs_c2h_timeout,
	    r92c_handle_c2h_report, sc);
}

void
r92c_handle_c2h_report(void *arg)
{
	struct rtwn_softc *sc = arg;

	rtwn_cmd_sleepable(sc, NULL, 0, r92c_handle_c2h_task);
}

#endif	/* RTWN_WITHOUT_UCODE */
