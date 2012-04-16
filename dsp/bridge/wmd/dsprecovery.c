/*
 * dsprecovery: under some conditions, the dsp bridge implementation in Milestone's
 * kernel may end up in an endless loop ("Mailbox full" msgs repeated forever in dmesg).
 * In such case, this simple module will set hw.dspbridge.needs_recovery property that
 * can be used to trigger dsp bridge reinitialization by init.
 *
 * hooking taken from "n - for testing kernel function hooking" by Nothize.
 * 
 * Copyright (C) 2012 Nadlabak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include "hook.h"
#include "symsearch.h"

#include <dspbridge/dbdefs.h>
#include <dspbridge/errbase.h>

#include <dspbridge/cfg.h>
#include <dspbridge/drv.h>
#include <dspbridge/dev.h>

#include <dspbridge/dbg.h>

#include "_tiomap.h"
#include "_tiomap_pwr.h"

#define MAILBOX_FIFOSTATUS(m) (0x80 + 4 * (m))

SYMSEARCH_DECLARE_FUNCTION_STATIC(DSP_STATUS, ss_tiomap3430_bump_dsp_opp_level);
SYMSEARCH_DECLARE_FUNCTION_STATIC(DSP_STATUS, ss_DSP_PeripheralClocks_Enable, struct WMD_DEV_CONTEXT *, IN void *);
SYMSEARCH_DECLARE_FUNCTION_STATIC(HW_STATUS, ss_HW_MBOX_restoreSettings, void __iomem *);
SYMSEARCH_DECLARE_FUNCTION_STATIC(HW_STATUS, ss_HW_MBOX_MsgWrite, const void __iomem *, const HW_MBOX_Id_t, const u32);

static inline unsigned int fifo_full(void __iomem *mbox_base, int mbox_id)
{
	return __raw_readl(mbox_base + MAILBOX_FIFOSTATUS(mbox_id)) & 0x1;
}

static int dsp_recovery( void )
{
  char *argv[] = { "/system/bin/setprop", "hw.dspbridge.needs_recovery", "1", NULL };
  static char *envp[] = {
        "HOME=/",
        "TERM=linux",
        "PATH=/sbin:/system/bin:/system/xbin", NULL };

  return call_usermodehelper( argv[0], argv, envp, UMH_WAIT_EXEC );
}

DSP_STATUS CHNLSM_InterruptDSP2(struct WMD_DEV_CONTEXT *pDevContext,
				u16 wMbVal)
{
	DSP_STATUS status = DSP_SOK;
	unsigned long timeout;
	u32 temp;
	u32 cnt = 0;
	bool time_expired = true;


	if (DSP_FAILED(status))
		return DSP_EFAIL;
#ifdef CONFIG_BRIDGE_DVFS
	if (pDevContext->dwBrdState == BRD_DSP_HIBERNATION ||
	    pDevContext->dwBrdState == BRD_HIBERNATION) {
		if (!in_atomic())
			ss_tiomap3430_bump_dsp_opp_level();
	}
#endif

	if (pDevContext->dwBrdState == BRD_DSP_HIBERNATION ||
	    pDevContext->dwBrdState == BRD_HIBERNATION) {
		/* Restart the peripheral clocks */
		ss_DSP_PeripheralClocks_Enable(pDevContext, NULL);

		/* Restore mailbox settings */
		/* Enabling Dpll in lock mode*/
		temp = (u32) *((REG_UWORD32 *)
				((u32) (pDevContext->cmbase) + 0x34));
		temp = (temp & 0xFFFFFFFE) | 0x1;
		*((REG_UWORD32 *) ((u32) (pDevContext->cmbase) + 0x34)) =
			(u32) temp;
		temp = (u32) *((REG_UWORD32 *)
				((u32) (pDevContext->cmbase) + 0x4));
		temp = (temp & 0xFFFFFC8) | 0x37;

		*((REG_UWORD32 *) ((u32) (pDevContext->cmbase) + 0x4)) =
			(u32) temp;
		ss_HW_MBOX_restoreSettings(pDevContext->dwMailBoxBase);

		/*  Access MMU SYS CONFIG register to generate a short wakeup */
		temp = (u32) *((REG_UWORD32 *) ((u32)
						(pDevContext->dwDSPMmuBase) + 0x10));

		pDevContext->dwBrdState = BRD_RUNNING;
	} else if (pDevContext->dwBrdState == BRD_RETENTION)
		/* Restart the peripheral clocks */
		ss_DSP_PeripheralClocks_Enable(pDevContext, NULL);

	/* Check the mailbox every 1 msec 10 times before giving up */
	for (cnt = 0; (cnt < 10) && (time_expired); cnt++) {
		timeout = jiffies + msecs_to_jiffies(1);
		time_expired = false;
		while (fifo_full((void __iomem *)pDevContext->dwMailBoxBase, 0)) {
			if (time_after(jiffies, timeout)) {
				time_expired = true;
				printk(KERN_WARNING "Mailbox full trying again count %d \n", cnt);
				break;
			}
		}
	}
	if ((cnt == 10) && (time_expired)) {
		printk(KERN_ERR "dspbridge: Mailbox was not empty after %d trials , no message written !!!\n", cnt);
		dsp_recovery();
		return WMD_E_TIMEOUT;
	}
	DBG_Trace(DBG_LEVEL3, "writing %x to Mailbox\n",
		  wMbVal);

	ss_HW_MBOX_MsgWrite(pDevContext->dwMailBoxBase, MBOX_ARM2DSP,
			 wMbVal);
	if (0) HOOK_INVOKE(CHNLSM_InterruptDSP2, pDevContext, wMbVal);
	return DSP_SOK;
}

struct hook_info g_hi[] = {
	HOOK_INIT(CHNLSM_InterruptDSP2),
	HOOK_INIT_END
};

static int __init dsprecovery_init( void )
{
	printk(KERN_INFO "DSP Bridge recovery helper: init\n");
	SYMSEARCH_BIND_FUNCTION_TO(dsprecovery, tiomap3430_bump_dsp_opp_level, ss_tiomap3430_bump_dsp_opp_level);
	SYMSEARCH_BIND_FUNCTION_TO(dsprecovery, DSP_PeripheralClocks_Enable, ss_DSP_PeripheralClocks_Enable);
	SYMSEARCH_BIND_FUNCTION_TO(dsprecovery, HW_MBOX_restoreSettings, ss_HW_MBOX_restoreSettings);
	SYMSEARCH_BIND_FUNCTION_TO(dsprecovery, HW_MBOX_MsgWrite, ss_HW_MBOX_MsgWrite);
	hook_init();
	return 0;
}

static void __exit dsprecovery_exit( void )
{
	hook_exit();
}

module_init( dsprecovery_init );
module_exit( dsprecovery_exit );

MODULE_ALIAS("DSP Bridge recovery helper");
MODULE_DESCRIPTION("Milestone's DSP Bridge failure signalization via android property");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nadlabak");

