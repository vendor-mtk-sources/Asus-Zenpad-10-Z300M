/*
 * MUSB OTG driver debugfs support
 *
 * Copyright 2010 Nokia Corporation
 * Contact: Felipe Balbi <felipe.balbi@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>


#include "musbfsh_core.h"
#include <linux/usb/ch9.h>

#define MUSBFSH_OTG_CSR0 0x102

static struct dentry *musbfsh_debugfs_root;

struct musbfsh_register_map {
	char *name;
	unsigned offset;
	unsigned size;
};

static const struct musbfsh_register_map musbfsh_regmap[] = {
	{"FAddr", 0x00, 8},
	{"Power", 0x01, 8},
	{"Frame", 0x0c, 16},
	{"Index", 0x0e, 8},
	{"Testmode", 0x0f, 8},
	{"TxMaxPp", 0x10, 16},
	{"TxCSRp", 0x12, 16},
	{"RxMaxPp", 0x14, 16},
	{"RxCSR", 0x16, 16},
	{"RxCount", 0x18, 16},
	{"ConfigData", 0x1f, 8},
	{"DevCtl", 0x60, 8},
	{"MISC", 0x61, 8},
	{"TxFIFOsz", 0x62, 8},
	{"RxFIFOsz", 0x63, 8},
	{"TxFIFOadd", 0x64, 16},
	{"RxFIFOadd", 0x66, 16},
	{"VControl", 0x68, 32},
	{"HWVers", 0x6C, 16},
	{"EPInfo", 0x78, 8},
	{"RAMInfo", 0x79, 8},
	{"LinkInfo", 0x7A, 8},
	{"VPLen", 0x7B, 8},
	{"HS_EOF1", 0x7C, 8},
	{"FS_EOF1", 0x7D, 8},
	{"LS_EOF1", 0x7E, 8},
	{"SOFT_RST", 0x7F, 8},
	{"DMA_CNTLch0", 0x204, 16},
	{"DMA_ADDRch0", 0x208, 32},
	{"DMA_COUNTch0", 0x20C, 32},
	{"DMA_CNTLch1", 0x214, 16},
	{"DMA_ADDRch1", 0x218, 32},
	{"DMA_COUNTch1", 0x21C, 32},
	{"DMA_CNTLch2", 0x224, 16},
	{"DMA_ADDRch2", 0x228, 32},
	{"DMA_COUNTch2", 0x22C, 32},
	{"DMA_CNTLch3", 0x234, 16},
	{"DMA_ADDRch3", 0x238, 32},
	{"DMA_COUNTch3", 0x23C, 32},
	{"DMA_CNTLch4", 0x244, 16},
	{"DMA_ADDRch4", 0x248, 32},
	{"DMA_COUNTch4", 0x24C, 32},
	{"DMA_CNTLch5", 0x254, 16},
	{"DMA_ADDRch5", 0x258, 32},
	{"DMA_COUNTch5", 0x25C, 32},
	{"DMA_CNTLch6", 0x264, 16},
	{"DMA_ADDRch6", 0x268, 32},
	{"DMA_COUNTch6", 0x26C, 32},
	{"DMA_CNTLch7", 0x274, 16},
	{"DMA_ADDRch7", 0x278, 32},
	{"DMA_COUNTch7", 0x27C, 32},
	{}			/* Terminating Entry */
};



static int musbfsh_regdump_show(struct seq_file *s, void *unused)
{
	struct musbfsh *musbfsh = s->private;
	unsigned i;

	seq_puts(s, "MUSB (M)HDRC Register Dump\n");

	for (i = 0; i < ARRAY_SIZE(musbfsh_regmap); i++) {
		switch (musbfsh_regmap[i].size) {
		case 8:
			seq_printf(s, "%-12s: %02x\n", musbfsh_regmap[i].name,
			musbfsh_readb(musbfsh->mregs, musbfsh_regmap[i].offset));
			break;
		case 16:
			seq_printf(s, "%-12s: %04x\n", musbfsh_regmap[i].name,
			musbfsh_readw(musbfsh->mregs, musbfsh_regmap[i].offset));
			break;
		case 32:
			seq_printf(s, "%-12s: %08x\n", musbfsh_regmap[i].name,
			musbfsh_readl(musbfsh->mregs, musbfsh_regmap[i].offset));
			break;
		}
	}

	return 0;
}

static int musbfsh_regdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, musbfsh_regdump_show, inode->i_private);
}



static int musbfsh_test_mode_show(struct seq_file *s, void *unused)
{
	struct musbfsh *musbfsh = s->private;
	unsigned test;

	test = musbfsh_readb(musbfsh->mregs, MUSBFSH_TESTMODE);

	if (test & MUSBFSH_TEST_FORCE_HOST)
		seq_puts(s, "force host\n");

	if (test & MUSBFSH_TEST_FIFO_ACCESS)
		seq_puts(s, "fifo access\n");

	if (test & MUSBFSH_TEST_FORCE_FS)
		seq_puts(s, "force full-speed\n");

	if (test & MUSBFSH_TEST_FORCE_HS)
		seq_puts(s, "force high-speed\n");

	if (test & MUSBFSH_TEST_PACKET)
		seq_puts(s, "test packet\n");

	if (test & MUSBFSH_TEST_K)
		seq_puts(s, "test K\n");

	if (test & MUSBFSH_TEST_J)
		seq_puts(s, "test J\n");

	if (test & MUSBFSH_TEST_SE0_NAK)
		seq_puts(s, "test SE0 NAK\n");

	return 0;
}

static const struct file_operations musbfsh_regdump_fops = {
	.open = musbfsh_regdump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static int musbfsh_test_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, musbfsh_test_mode_show, inode->i_private);
}

void musbfshdebugfs_otg_write_fifo(u16 len, u8 *buf, struct musbfsh *mtk_musb)
{
	int i;

	INFO("musb_otg_write_fifo,len=%d\n", len);
	for (i = 0; i < len; i++)
		musbfsh_writeb(mtk_musb->mregs, 0x20, *(buf + i));
}

void musbfshdebugfs_h_setup(struct usb_ctrlrequest *setup, struct musbfsh *mtk_musb)
{
	unsigned short csr0;

	INFO("musbfsh_h_setup++\n");
	musbfshdebugfs_otg_write_fifo(sizeof(struct usb_ctrlrequest), (u8 *) setup, mtk_musb);
	csr0 = musbfsh_readw(mtk_musb->mregs, MUSBFSH_OTG_CSR0);
	INFO("musbfsh_h_setup,csr0=0x%x\n", csr0);
	csr0 |= MUSBFSH_CSR0_H_SETUPPKT | MUSBFSH_CSR0_TXPKTRDY;
	musbfsh_writew(mtk_musb->mregs, MUSBFSH_OTG_CSR0, csr0);

	INFO("musbfsh_h_setup--\n");
}

static ssize_t musbfsh_test_mode_write(struct file *file,
				       const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct musbfsh *musbfsh = s->private;
	u8 test = 0;
	char buf[20];
	unsigned char power;
	struct usb_ctrlrequest setup_packet;

	setup_packet.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
	setup_packet.bRequest = USB_REQ_GET_DESCRIPTOR;
	setup_packet.wIndex = 0;
	setup_packet.wValue = 0x0100;
	setup_packet.wLength = 0x40;

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "force host", 9))
		test = MUSBFSH_TEST_FORCE_HOST;

	if (!strncmp(buf, "fifo access", 11))
		test = MUSBFSH_TEST_FIFO_ACCESS;

	if (!strncmp(buf, "force full-speed", 15))
		test = MUSBFSH_TEST_FORCE_FS;

	if (!strncmp(buf, "force high-speed", 15))
		test = MUSBFSH_TEST_FORCE_HS;

	if (!strncmp(buf, "test packet", 10)) {
		test = MUSBFSH_TEST_PACKET;
		musbfsh_load_testpacket(musbfsh);
	}

	if (!strncmp(buf, "test suspend_resume", 18)) {
		INFO("HS_HOST_PORT_SUSPEND_RESUME\n");
		msleep(5000);	/*the host must continue sending SOFs for 15s*/
		INFO("please begin to trigger suspend!\n");
		msleep(10000);
		power = musbfsh_readb(musbfsh->mregs, MUSBFSH_POWER);
		power |= MUSBFSH_POWER_SUSPENDM | MUSBFSH_POWER_ENSUSPEND;
		musbfsh_writeb(musbfsh->mregs, MUSBFSH_POWER, power);
		msleep(5000);
		INFO("please begin to trigger resume!\n");
		msleep(10000);
		power &= ~MUSBFSH_POWER_SUSPENDM;
		power |= MUSBFSH_POWER_RESUME;
		musbfsh_writeb(musbfsh->mregs, MUSBFSH_POWER, power);
		mdelay(25);
		power &= ~MUSBFSH_POWER_RESUME;
		musbfsh_writeb(musbfsh->mregs, MUSBFSH_POWER, power);
		/*SOF continue*/
		musbfshdebugfs_h_setup(&setup_packet, musbfsh);
		return count;
	}

	if (!strncmp(buf, "test get_descripter", 18)) {
		INFO("SINGLE_STEP_GET_DEVICE_DESCRIPTOR\n");
/*the host issues SOFs for 15s allowing the test engineer to raise the scope trigger just above the SOF voltage level.*/
		msleep(15000);
		musbfshdebugfs_h_setup(&setup_packet, musbfsh);
		return count;
	}


	if (!strncmp(buf, "test K", 6))
		test = MUSBFSH_TEST_K;

	if (!strncmp(buf, "test J", 6))
		test = MUSBFSH_TEST_J;

	if (!strncmp(buf, "test SE0 NAK", 12))
		test = MUSBFSH_TEST_SE0_NAK;

	musbfsh_writeb(musbfsh->mregs, MUSBFSH_TESTMODE, test);

	return count;
}

static const struct file_operations musbfsh_test_mode_fops = {
	  .open = musbfsh_test_mode_open,
	  .write = musbfsh_test_mode_write,
	  .read = seq_read,
	  .llseek = seq_lseek,
	  .release = single_release,
};

int musbfsh_init_debugfs(struct musbfsh *musbfsh)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	INFO("musbfsh_init_debugfs\n");
	INFO("++\n");
	root = debugfs_create_dir("musbfsh", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}
	file = debugfs_create_file("regdump", S_IRUGO, root, musbfsh, &musbfsh_regdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	INFO("musbfsh_init_debugfs 1\n");
	file = debugfs_create_file("testmode", S_IRUGO | S_IWUSR,
								root, musbfsh, &musbfsh_test_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}
	INFO("musbfsh_init_debugfs 2\n");

	musbfsh_debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void /* __init_or_exit */ musbfsh_exit_debugfs(struct musbfsh *musbfsh)
{
	  debugfs_remove_recursive(musbfsh_debugfs_root);
}
