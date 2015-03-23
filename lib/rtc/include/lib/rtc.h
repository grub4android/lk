/*
 * Generic RTC interface.
 * This version contains the part of the user interface to the Real Time Clock
 * service. It is used with both the legacy mc146818 and also  EFI
 * Struct rtc_time and first 12 ioctl by Paul Gortmaker, 1996 - separated out
 * from <linux/mc146818rtc.h> to this file for 2.4 kernels.
 *
 * Copyright (C) 1999 Hewlett-Packard Co.
 * Copyright (C) 1999 Stephane Eranian <eranian@hpl.hp.com>
 */
#ifndef _LINUX_RTC_H_
#define _LINUX_RTC_H_


#include <sys/types.h>
#include <lib/rtc_uapi.h>
#include <stdbool.h>

extern int rtc_month_days(unsigned int month, unsigned int year);
extern int rtc_year_days(unsigned int day, unsigned int month, unsigned int year);
extern int rtc_valid_tm(struct rtc_time *tm);
extern lk_bigtime_t rtc_tm_to_time64(struct rtc_time *tm);
extern void rtc_time64_to_tm(lk_bigtime_t time, struct rtc_time *tm);

/**
 * Deprecated. Use rtc_time64_to_tm().
 */
static inline void rtc_time_to_tm(unsigned long time, struct rtc_time *tm)
{
	rtc_time64_to_tm(time, tm);
}

/**
 * Deprecated. Use rtc_tm_to_time64().
 */
static inline int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time)
{
	*time = rtc_tm_to_time64(tm);

	return 0;
}


extern struct class *rtc_class;

static inline bool is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

#endif /* _LINUX_RTC_H_ */
