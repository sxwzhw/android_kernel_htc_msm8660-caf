/*
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2010 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* Control bluetooth power for shooter platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>

#include <linux/mfd/pmic8058.h>
#include "board-shooter.h"

#ifdef CONFIG_MACH_SHOOTER_U
#include <mach/htc_sleep_clk.h>
#endif

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4329";

/* bt on configuration */
static uint32_t shooter_bt_on_table[] = {

	/* BT_RTS */
	GPIO_CFG(shooter_GPIO_BT_UART1_RTS,
				1,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA),
	/* BT_CTS */
	GPIO_CFG(shooter_GPIO_BT_UART1_CTS,
				1,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA),
	/* BT_RX */
	GPIO_CFG(shooter_GPIO_BT_UART1_RX,
				1,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA),
	/* BT_TX */
	GPIO_CFG(shooter_GPIO_BT_UART1_TX,
				1,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA),

	/* BT_HOST_WAKE */
	GPIO_CFG(shooter_GPIO_BT_HOST_WAKE,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
	/* BT_CHIP_WAKE */
	GPIO_CFG(shooter_GPIO_BT_CHIP_WAKE,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),

	/* BT_RESET_N */
	GPIO_CFG(shooter_GPIO_BT_RESET_N,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
	/* BT_SHUTDOWN_N */
	GPIO_CFG(shooter_GPIO_BT_SHUTDOWN_N,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
};

/* bt off configuration */
static uint32_t shooter_bt_off_table[] = {

	/* BT_RTS */
	GPIO_CFG(shooter_GPIO_BT_UART1_RTS,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA),
	/* BT_CTS */
	GPIO_CFG(shooter_GPIO_BT_UART1_CTS,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA),
	/* BT_RX */
	GPIO_CFG(shooter_GPIO_BT_UART1_RX,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA),
	/* BT_TX */
	GPIO_CFG(shooter_GPIO_BT_UART1_TX,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA),

	/* BT_RESET_N */
	GPIO_CFG(shooter_GPIO_BT_RESET_N,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
	/* BT_SHUTDOWN_N */
	GPIO_CFG(shooter_GPIO_BT_SHUTDOWN_N,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),

	/* BT_HOST_WAKE */
	GPIO_CFG(shooter_GPIO_BT_HOST_WAKE,
				0,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP,
				GPIO_CFG_4MA),
	/* BT_CHIP_WAKE */
	GPIO_CFG(shooter_GPIO_BT_CHIP_WAKE,
				0,
				GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_4MA),
};

static void config_bt_table(uint32_t *table, int len)
{
	int n, rc;

	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[BT]%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void shooter_config_bt_on(void)
{
	printk(KERN_INFO "[BT]-- R ON --\n");

	/* set bt on configuration*/
	config_bt_table(shooter_bt_on_table,
				ARRAY_SIZE(shooter_bt_on_table));
	mdelay(5);

	/* BT_SHUTDOWN_N */
	gpio_set_value(shooter_GPIO_BT_SHUTDOWN_N, 1);
	/*mdelay(2);*/

	/* BT_RESET_N */
	gpio_set_value(shooter_GPIO_BT_RESET_N, 1);
	mdelay(2);
}

static void shooter_config_bt_off(void)
{
	printk(KERN_INFO "[BT]-- R OFF --\n");

	/* BT_RESET_N */
	gpio_set_value(shooter_GPIO_BT_RESET_N, 0);
	/*mdelay(2);*/

	/* BT_SHUTDOWN_N */
	gpio_set_value(shooter_GPIO_BT_SHUTDOWN_N, 0);
	mdelay(2);

	/* set bt off configuration*/
	config_bt_table(shooter_bt_off_table,
				ARRAY_SIZE(shooter_bt_off_table));
	mdelay(5);

	/* BT_RTS */
	gpio_set_value(shooter_GPIO_BT_UART1_RTS, 1);

	/* BT_CTS */

	/* BT_RX */

	/* BT_TX */
	gpio_set_value(shooter_GPIO_BT_UART1_TX, 0);

	/* BT_HOST_WAKE */

	/* BT_CHIP_WAKE */
	gpio_set_value(shooter_GPIO_BT_CHIP_WAKE, 0);
}

static int bluetooth_set_power(void *data, bool blocked)
{
	if (!blocked)
		shooter_config_bt_on();
	else
		shooter_config_bt_off();

	return 0;
}

static struct rfkill_ops shooter_rfkill_ops = {
	.set_block = bluetooth_set_power,
};

static int shooter_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	bool default_state = true; /* off */

#ifdef CONFIG_MACH_SHOOTER_U
	rc = gpio_request(shooter_GPIO_BT_RESET_N, "bt_reset");
	if (rc)
		goto err_gpio_reset;
	rc = gpio_request(shooter_GPIO_BT_SHUTDOWN_N, "bt_shutdown");
	if (rc)
		goto err_gpio_shutdown;

	htc_wifi_bt_sleep_clk_ctl(CLK_ON, ID_BT);
#endif

	mdelay(2);

	bluetooth_set_power(NULL, default_state);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
						 &shooter_rfkill_ops, NULL);
	if (!bt_rfk) {
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_set_states(bt_rfk, default_state, false);

	/* userspace cannot take exclusive control */
	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	return 0;

err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_rfkill_alloc:
#ifdef CONFIG_MACH_SHOOTER_U
	gpio_free(shooter_GPIO_BT_SHUTDOWN_N);
err_gpio_shutdown:
	gpio_free(shooter_GPIO_BT_RESET_N);
err_gpio_reset:
#endif
	return rc;
}

static int shooter_rfkill_remove(struct platform_device *dev)
{
	rfkill_unregister(bt_rfk);
	/*rfkill_free(bt_rfk);*/
	rfkill_destroy(bt_rfk);
#ifdef CONFIG_MACH_SHOOTER_U
	gpio_free(shooter_GPIO_BT_SHUTDOWN_N);
	gpio_free(shooter_GPIO_BT_RESET_N);
#endif

	return 0;
}

static struct platform_driver shooter_rfkill_driver = {
	.probe = shooter_rfkill_probe,
	.remove = shooter_rfkill_remove,
	.driver = {
		.name = "shooter_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init shooter_rfkill_init(void)
{
	return platform_driver_register(&shooter_rfkill_driver);
}

static void __exit shooter_rfkill_exit(void)
{
	platform_driver_unregister(&shooter_rfkill_driver);
}

module_init(shooter_rfkill_init);
module_exit(shooter_rfkill_exit);
MODULE_DESCRIPTION("shooter rfkill");
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");
