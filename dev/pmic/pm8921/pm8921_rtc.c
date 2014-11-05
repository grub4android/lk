/*
 * Copyright (c) 2011-2013, Linux Foundation. All rights reserved.
 * Copyright (c) 2011-2014, Xiaomi Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of  Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <err.h>
#include <dev/pm8921.h>
#include <dev/pm8921_rtc.h>
#include <platform/timer.h>
#include <platform/msm_shared/timer.h>
#include "pm8921_hw.h"

/* RTC Register offsets from RTC CTRL REG */
#define PM8XXX_ALARM_CTRL_OFFSET	0x01
#define PM8XXX_RTC_WRITE_OFFSET		0x02
#define PM8XXX_RTC_READ_OFFSET		0x06
#define PM8XXX_ALARM_RW_OFFSET		0x0A

/* RTC_CTRL register bit fields */
#define PM8xxx_RTC_ENABLE		BIT(7)
#define PM8xxx_RTC_ALARM_ENABLE		BIT(1)
#define PM8xxx_RTC_ALARM_CLEAR		BIT(0)
#define PM8xxx_RTC_ABORT_ENABLE		BIT(0)

#define NUM_8_BIT_RTC_REGS		0x4

#define PM8XXX_READ_BASE (PM8921_RTC_CTRL + PM8XXX_RTC_READ_OFFSET)
#define PM8XXX_WRITE_BASE (PM8921_RTC_CTRL + PM8XXX_RTC_WRITE_OFFSET)

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

static pm8921_dev_t *dev;

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* Intialize the pmic driver */
void pm8921_rtc_init(pm8921_dev_t *pmic)
{
	dev = pmic;
}

/*
 * The RTC registers need to be read/written one byte at a time. This is a
 * hardware limitation.
 */
static int pm8xxx_read_wrapper(uint8_t *rtc_val,
		int base, int count)
{
	int i, rc;

	for (i = 0; i < count; i++) {
		rc = dev->read(&rtc_val[i], 1, base + i);
		if (rc) {
			dprintf(CRITICAL,"Failed to read RTC_CTRL reg = %d\n", rc);
			return rc;
		}
	}

	return 0;
}

static int pm8xxx_write_wrapper(uint8_t *rtc_val,
		int base, int count)
{
	int i, rc;

	for (i = 0; i < count; i++) {
		rc = dev->write(&rtc_val[i], 1, base + i);
		if (rc) {
			dprintf(CRITICAL,"Failed to write RTC_CTRL reg = %d\n",rc);
			return rc;
		}
	}

	return 0;
}

/*
 * Steps to write the RTC registers.
 * 1. Disable alarm if enabled.
 * 2. Write 0x00 to LSB.
 * 3. Write Byte[1], Byte[2], Byte[3] then Byte[0].
 * 4. Enable alarm if disabled in step 1.
 */
int pm8xxx_rtc_set_time(unsigned long secs)
{
	int rc, i;
	u8 value[NUM_8_BIT_RTC_REGS], reg = 0, alarm_enabled = 0, ctrl_reg;

	for (i = 0; i < NUM_8_BIT_RTC_REGS; i++) {
		value[i] = secs & 0xFF;
		secs >>= 8;
	}

	dprintf(CRITICAL, "Seconds value to be written to RTC = %lu\n", secs);

	rc = pm8xxx_read_wrapper(&ctrl_reg, PM8921_RTC_CTRL, 1);
	if (rc < 0) {
		dprintf(CRITICAL, "RTC read data register failed\n");
		return rc;
	}

	if (ctrl_reg & PM8xxx_RTC_ALARM_ENABLE) {
		alarm_enabled = 1;
		ctrl_reg &= ~PM8xxx_RTC_ALARM_ENABLE;
		rc = pm8xxx_write_wrapper(&ctrl_reg, PM8921_RTC_CTRL,
				1);
		if (rc < 0) {
			dprintf(CRITICAL, "Write to RTC control register "
								"failed\n");
			goto rtc_rw_fail;
		}
	}

	/* Write 0 to Byte[0] */
	reg = 0;
	rc = pm8xxx_write_wrapper(&reg, PM8XXX_WRITE_BASE, 1);
	if (rc < 0) {
		dprintf(CRITICAL, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	/* Write Byte[1], Byte[2], Byte[3] */
	rc = pm8xxx_write_wrapper(value + 1,
					PM8XXX_WRITE_BASE + 1, 3);
	if (rc < 0) {
		dprintf(CRITICAL, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	/* Write Byte[0] */
	rc = pm8xxx_write_wrapper(value, PM8XXX_WRITE_BASE, 1);
	if (rc < 0) {
		dprintf(CRITICAL, "Write to RTC write data register failed\n");
		goto rtc_rw_fail;
	}

	if (alarm_enabled) {
		ctrl_reg |= PM8xxx_RTC_ALARM_ENABLE;
		rc = pm8xxx_write_wrapper(&ctrl_reg, PM8921_RTC_CTRL,
									1);
		if (rc < 0) {
			dprintf(CRITICAL, "Write to RTC control register "
								"failed\n");
			goto rtc_rw_fail;
		}
	}

rtc_rw_fail:

	return rc;
}

int pm8xxx_rtc_read_time(unsigned long* secs)
{
	int rc;
	uint8_t value[NUM_8_BIT_RTC_REGS], reg;

	rc = pm8xxx_read_wrapper(value, PM8XXX_READ_BASE,
							NUM_8_BIT_RTC_REGS);
	if (rc < 0) {
		dprintf(CRITICAL, "RTC read data register failed\n");
		return rc;
	}

	/*
	 * Read the LSB again and check if there has been a carry over.
	 * If there is, redo the read operation.
	 */
	rc = pm8xxx_read_wrapper(&reg, PM8XXX_READ_BASE, 1);
	if (rc < 0) {
		dprintf(CRITICAL, "RTC read data register failed\n");
		return rc;
	}

	if (unlikely(reg < value[0])) {
		rc = pm8xxx_read_wrapper(value,
				PM8XXX_READ_BASE, NUM_8_BIT_RTC_REGS);
		if (rc < 0) {
			dprintf(CRITICAL, "RTC read data register failed\n");
			return rc;
		}
	}

	*secs = value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);

	return 0;
}

int pm8921_rtc_alarm_disable(void)
{
	int rc;
	uint8_t reg;

	rc = dev->read(&reg, 1, PM8921_RTC_CTRL);
	if (rc) {
		dprintf(CRITICAL,"Failed to read RTC_CTRL reg = %d\n",rc);
		return rc;
	}
	reg = (reg & ~PM8921_RTC_ALARM_ENABLE);

	rc = dev->write(&reg, 1, PM8921_RTC_CTRL);
	if (rc) {
		dprintf(CRITICAL,"Failed to write RTC_CTRL reg = %d\n",rc);
		return rc;
	}

	return rc;
}

/*
 * The number of days in the month.
 */
int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}

void rtc_time_to_tm(unsigned long time, struct rtc_time *tm)
{
	unsigned int month, year;
	int days;

	days = time / 86400;
	time -= (unsigned int) days * 86400;

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	year = 1970 + days / 365;
	days -= (year - 1970) * 365
		+ LEAPS_THRU_END_OF(year - 1)
		- LEAPS_THRU_END_OF(1970 - 1);
	if (days < 0) {
		year -= 1;
		days += 365 + is_leap_year(year);
	}
	tm->tm_year = year - 1900;
	tm->tm_yday = days + 1;

	for (month = 0; month < 11; month++) {
		int newdays;

		newdays = days - rtc_month_days(month, year);
		if (newdays < 0)
			break;
		days = newdays;
	}
	tm->tm_mon = month;
	tm->tm_mday = days + 1;

	tm->tm_hour = time / 3600;
	time -= tm->tm_hour * 3600;
	tm->tm_min = time / 60;
	tm->tm_sec = time - tm->tm_min * 60;

	tm->tm_isdst = 0;
}
