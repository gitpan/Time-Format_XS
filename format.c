/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

The GPG signature in this file may be checked with 'gpg --verify format.c'. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <ctype.h>
#include "format.h"

static char _VERSION[] = "0.12";

/* format.c, version 0.12

This is part of the Time::Format_XS module.  See the .pm file for documentation.

This code is copyright (c) 2003 by Eric J. Roode -- all rights reserved.

See the Changes file for change history.

*/


/* packnum
Append a number to the end of the output, or compute how many bytes it would add.
*/
static size_t packnum(char **out, int num, int len, char pad)
    {
    int outlen;
    char buf[12];

    /* Shortcut -- if padding, output length will always be requested length */
    if (NULL == out  &&  (pad == ' ' || pad == '0'))
        return len;

    if (pad == ' ')
        outlen = sprintf(buf, "%*d", len, num);
    else if (pad == '0')
        outlen = sprintf(buf, "%0*d", len, num);
    else
        outlen = sprintf(buf, "%d", num);

    if (NULL != out)
        {
        strcpy(*out, buf);
        *out += outlen;
        }
    return outlen;
    }

/* String cases: Uppercase, lowercase, mixed case, no case conversion, one-shot LC, one-shot UC */
enum cases { UC, LC, NC };

/* packstr
Append a string to the end of the output, performing case conversion.
*/
static size_t packstr(char **out, const char *str, size_t len, enum cases cfirst, enum cases crest)
    {
    char *target = *out;
    int ch;
    enum cases c = cfirst;
    if (!*str || len--==0)  return 0;  /* degenerate case */

    switch (cfirst)
        {
            case UC: *target++ = toupper(*str++); break;
            case LC: *target++ = tolower(*str++); break;
            case NC: *target++ =         *str++;  break;
        }

    while ((ch = *str++)  &&  len--)
        {
        switch (crest)
            {
                case UC: *target++ = toupper(ch); break;
                case LC: *target++ = tolower(ch); break;
                case NC: *target++ =         ch;  break;
            }
        }
    *out = target;
    return strlen(str);
    }

/* forward
Returns true if the beginning of fmt matches the whole of pat.
*/
static int forward(const char *fmt, const char *pat)
    {
    return !strncmp(fmt, pat, strlen(pat));
    }

/* backward
Returns true if fmt ends with pat, and is not preceded by an odd backslash.
<start> is a pointer to the beginning of fmt, so we know not to go back too far.
*/
static int backward(const char *start, const char *fmt, const char *pat)
    {
    size_t patlen = strlen(pat);
    int bs = 1;
    if (fmt-start < patlen) return 0;
    fmt -= patlen;
    if (strncmp(fmt, pat, patlen))  return 0;

    /* we have a match; check that it's not preceded by an odd number of backslashes */
    while (fmt >= start  &&  *fmt-- == '\\')
        bs = !bs;
    return bs;
    }

/* bool = month_context(start, fmt, patlen);
   Returns TRUE if the current format (delimited by fmt and patlen)
   is in a "month" context; that is, it is followed or preceeded by
   a year or a day.
   We check immediately following and immediately preceeding, and
   also we check one character away in either direction.  So mm/dd
   will work (because there's one character in between).
 */
static int month_context(const char *start, const char *fmt, size_t patlen)
    {
    const char *backskip = fmt-2;
    const char *fwdskip = fmt+patlen+1;
    if (*backskip != '\\') backskip++;
    if (*fwdskip == '\\') fwdskip++;

    return  forward(fmt+patlen, "?d")
        ||  forward(fmt+patlen , "d")
        ||  forward(fwdskip,    "?d")
        ||  forward(fwdskip,     "d")
        ||  forward(fmt+patlen, "yy")
        ||  forward(fwdskip,    "yy")
        ||  backward(start, fmt, "yy")
        ||  backward(start, backskip, "yy")
        ||  backward(start, fmt, "d")
        ||  backward(start, backskip, "d");
    }

/* bool = minute_context(start, fmt, patlen);
   Returns TRUE if the current format is in a "minute" context.
   That is, if it's preceeded by an hour and/or followed by a second.
 */
static int minute_context(const char *start, const char *fmt, size_t patlen)
    {
    const char *backskip = fmt-1;
    const char *fwdskip = fmt+patlen+1;
    if (*backskip == '\\') backskip--;
    if (*fwdskip == '\\') fwdskip++;

    return  forward(fmt+patlen, "?s")
        ||  forward(fmt+patlen,  "s")
        ||  forward(fwdskip,    "?s")
        ||  forward(fwdskip,     "s")
        ||  backward(start, fmt, "h")
        ||  backward(start, backskip, "h")
        ||  backward(start, fmt, "H")
        ||  backward(start, backskip, "H");
    }

/* Month and weekday names, and their abbreviations.  Populated by setup_locale. */
static char *Month[12];
static char *Mon[12];
static char *Weekday[7];
static char *Day[7];

/* setup_locale
Populate day/month names based on current locale.
Alas, I don't know how portable this code is.
*/
static void setup_locale(void)
    {
    char *cur_locale;
    static char prev_locale[40];
    static int checked_locale = 0;

    /* have we checked the locale yet? */
    if (checked_locale)
        {
        /* Yes.  Has it changed? */
        cur_locale = setlocale(LC_TIME, NULL);
        if (NULL != cur_locale  &&  !strcmp(cur_locale, prev_locale))
            /* No, it's the same */
            return;
        }
    else
        {
        cur_locale = setlocale(LC_TIME, "");
        checked_locale = 1;
        }

    if (NULL == cur_locale)
        {
        /* Couldn't set up locale for some reason.  Use English. */
        Month[0]   = "January";
        Month[1]   = "February";
        Month[2]   = "March";
        Month[3]   = "April";
        Month[4]   = "May";
        Month[5]   = "June";
        Month[6]   = "July";
        Month[7]   = "August";
        Month[8]   = "September";
        Month[9]   = "October";
        Month[10]  = "November";
        Month[11]  = "December";
        Mon[0]     = "Jan";
        Mon[1]     = "Feb";
        Mon[2]     = "Mar";
        Mon[3]     = "Apr";
        Mon[4]     = "May";
        Mon[5]     = "Jun";
        Mon[6]     = "Jul";
        Mon[7]     = "Aug";
        Mon[8]     = "Sep";
        Mon[9]     = "Oct";
        Mon[10]    = "Nov";
        Mon[11]    = "Dec";
        Weekday[0] = "Sunday";
        Weekday[1] = "Monday";
        Weekday[2] = "Tuesday";
        Weekday[3] = "Wednesday";
        Weekday[4] = "Thursday";
        Weekday[5] = "Friday";
        Weekday[6] = "Saturday";
        Day[0]     = "Sun";
        Day[1]     = "Mon";
        Day[2]     = "Tue";
        Day[3]     = "Wed";
        Day[4]     = "Thu";
        Day[5]     = "Fri";
        Day[6]     = "Sat";
        return;
        }
    strcpy(prev_locale, cur_locale);
    Month[0]   = nl_langinfo(MON_1);
    Month[1]   = nl_langinfo(MON_2);
    Month[2]   = nl_langinfo(MON_3);
    Month[3]   = nl_langinfo(MON_4);
    Month[4]   = nl_langinfo(MON_5);
    Month[5]   = nl_langinfo(MON_6);
    Month[6]   = nl_langinfo(MON_7);
    Month[7]   = nl_langinfo(MON_8);
    Month[8]   = nl_langinfo(MON_9);
    Month[9]   = nl_langinfo(MON_10);
    Month[10]  = nl_langinfo(MON_11);
    Month[11]  = nl_langinfo(MON_12);
    Mon[0]     = nl_langinfo(ABMON_1);
    Mon[1]     = nl_langinfo(ABMON_2);
    Mon[2]     = nl_langinfo(ABMON_3);
    Mon[3]     = nl_langinfo(ABMON_4);
    Mon[4]     = nl_langinfo(ABMON_5);
    Mon[5]     = nl_langinfo(ABMON_6);
    Mon[6]     = nl_langinfo(ABMON_7);
    Mon[7]     = nl_langinfo(ABMON_8);
    Mon[8]     = nl_langinfo(ABMON_9);
    Mon[9]     = nl_langinfo(ABMON_10);
    Mon[10]    = nl_langinfo(ABMON_11);
    Mon[11]    = nl_langinfo(ABMON_12);
    Weekday[0] = nl_langinfo(DAY_1);
    Weekday[1] = nl_langinfo(DAY_2);
    Weekday[2] = nl_langinfo(DAY_3);
    Weekday[3] = nl_langinfo(DAY_4);
    Weekday[4] = nl_langinfo(DAY_5);
    Weekday[5] = nl_langinfo(DAY_6);
    Weekday[6] = nl_langinfo(DAY_7);
    Day[0]     = nl_langinfo(ABDAY_1);
    Day[1]     = nl_langinfo(ABDAY_2);
    Day[2]     = nl_langinfo(ABDAY_3);
    Day[3]     = nl_langinfo(ABDAY_4);
    Day[4]     = nl_langinfo(ABDAY_5);
    Day[5]     = nl_langinfo(ABDAY_6);
    Day[6]     = nl_langinfo(ABDAY_7);
    return;
    }

/* helper function -- tightly bound with time_format() */
static void translate_str(const char *pat, const char **fmt, char **out, const char *newval, enum cases cf, enum cases cr, size_t *len, int modifying)
    {
    *fmt += strlen(pat);
    if (modifying)
        packstr(out, newval, -1, cf, cr);
    else
        *len += strlen(newval);
    }

/* helper function -- tightly bound with time_format() */
static void translate_num(const char *pat, const char **fmt, char **out, int num, int width, char pack, size_t *len, int modifying)
    {
    *fmt += strlen(pat);
    if (modifying)
        packnum(out, num, width, pack);
    else
        *len += packnum(NULL, num, width, pack);
    }

/* time_format
Given a format, and a string that represents a time number, returns a malloc'd output string.
The time input is a string, because it could be ten digits plus six or more decimals, which
exceeds the precision of most double-precision floats.  So we parse it into a long and a double.
See the documentation for the Time::Format module on what formats are expanded.
*/
char *time_format(const char *fmt, const char *in_time)
    {
    const char *start = fmt;    /* remember where we started */
    char *out, *ret;            /* local holding place for output string */
    int modifying;
    int checked_locale=0;       /* Have we setup the locale this run? */

    /* do time computations here */
    long time = atol(in_time);
    struct tm *t = localtime(&time);
    int month = t->tm_mon+1;
    int year  = t->tm_year + 1900;
    int hour  = t->tm_hour == 0? 12 : t->tm_hour <= 12? t->tm_hour : t->tm_hour-12;

    /* First, compute length of result string.  Then actually populate it. */
    size_t length = 0;
    for (modifying=0; modifying<=1; modifying++)
        {
        int reset_uclc;
        int upper=0, lower=0, uppernext=0, lowernext=0, quoting=0;
        while (*fmt)
            {
            char *jump;
            enum cases first, rest, f, r;
            reset_uclc = 1;    /* reset uppernext and lowernext by default */

            if (quoting)
                jump = strstr(fmt, "\\E");    /* look for end of literal-quote */
            else
                jump = strpbrk(fmt, "\\dDy?hHsaApPMmWwutT");  /* jump to one of these */

            /* what sort of case conversions to do? */
            if (lower)
                rest = LC;
            else if (upper)
                rest = UC;
            else
                rest = NC;

            if (lowernext)
                first = LC;
            else if (uppernext)
                first = UC;
            else
                first = rest;

            if (NULL == jump)
                {
                if (modifying)
                    packstr(&out, fmt, -1, first, rest);    /* No further matches; done! */
                else
                    length += strlen(fmt);
                break;
                }
            else if (jump > fmt)    /* skip over the section that does not contain codes */
                {
                if (modifying)
                    packstr(&out, fmt, jump-fmt, first, rest);
                else
                    length += jump-fmt;
                fmt = jump;
                }

            switch (*fmt)
                {
                    case '\\':        /* escape character */
                          fmt++;
                          switch (*fmt)
                              {
                                  case 'Q':    /* Begin quote */
                                        fmt++;
                                        quoting = 1;
                                        break;
                                  case 'E':    /* end quote */
                                        fmt++;
                                        quoting = upper = lower = lowernext = uppernext = 0;
                                        break;
                                  case 'U':    /* begin uppercasing */
                                        fmt++;
                                        reset_uclc = 0;    /* don't reset uppernext/lowernext */
                                        upper = 1;
                                        break;
                                  case 'L':    /* begin lowercasing */
                                        fmt++;
                                        reset_uclc = 0;
                                        lower = 1;
                                        break;
                                  case 'u':    /* one-shot upper */
                                        fmt++;
                                        reset_uclc = 0;
                                        lowernext = 0;
                                        uppernext = 1;
                                        break;
                                  case 'l':    /* one-shot lower */
                                        fmt++;
                                        reset_uclc = 0;
                                        uppernext = 0;
                                        lowernext = 1;
                                        break;
                                  default:
                                        if (modifying) *out++ = *fmt++; else length++, fmt++;
                                        break;
                              }
                          break;

                    case 'd':        /* dd, day, d */

                          if (fmt[1] == 'd')
                              translate_num("dd", &fmt, &out, t->tm_mday, 2, '0', &length, modifying);
                          else if (forward(fmt, "day"))
                              {
                              f = first==NC? LC : first;
                              r = rest ==NC? LC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("day", &fmt, &out, Day[t->tm_wday], f,r, &length, modifying);
                              }
                          else
                              translate_num("d", &fmt, &out, t->tm_mday, 0, 'x', &length, modifying);
                          break;

                    case 'y':        /* yyyy, yy */

                          if (forward(fmt, "yyyy"))
                              translate_num("yyyy", &fmt, &out, year, 4, '0', &length, modifying);
                          else if (forward(fmt, "yy"))
                              translate_num("yy", &fmt, &out, year%100, 2, '0', &length, modifying);
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 'h':        /* hh, h */

                          if (fmt[1] == 'h')
                              translate_num("hh", &fmt, &out, t->tm_hour, 2, '0', &length, modifying);
                          else
                              translate_num("h", &fmt, &out, t->tm_hour, 0, 'x', &length, modifying);
                          break;

                    case 'H':        /* HH, H */

                          if (fmt[1] == 'H')
                              translate_num("HH", &fmt, &out, hour, 2, '0', &length, modifying);
                          else
                              translate_num("H", &fmt, &out, hour, 0, 'x', &length, modifying);
                          break;

                    case 's':        /* ss, s */

                          if (fmt[1] == 's')
                              translate_num("ss", &fmt, &out, t->tm_sec, 2, '0', &length, modifying);
                          else
                              translate_num("s", &fmt, &out, t->tm_sec, 0, 'x', &length, modifying);
                          break;

                    case 'm':        /* month, mon, mm{on}, m{on}, mm{in}, m{in}, mmm, mm, m */

                          if (forward(fmt, "month"))
                              {
                              f = first==NC? LC : first;
                              r = rest ==NC? LC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("month", &fmt, &out, Month[t->tm_mon], f,r, &length, modifying);
                              }
                          else if (forward(fmt, "mon"))
                              {
                              f = first==NC? LC : first;
                              r = rest ==NC? LC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("mon", &fmt, &out, Mon[t->tm_mon], f,r, &length, modifying);
                              }
                          else if (forward(fmt, "mm{on}"))
                              translate_num("mm{on}", &fmt, &out, month, 2, '0', &length, modifying);
                          else if (forward(fmt, "m{on}"))
                              translate_num("m{on}", &fmt, &out, month, 0, 'x', &length, modifying);
                          else if (forward(fmt, "mm{in}"))
                              translate_num("mm{in}", &fmt, &out, t->tm_min, 2, '0', &length, modifying);
                          else if (forward(fmt, "m{in}"))
                              translate_num("m{in}", &fmt, &out, t->tm_min, 0, 'x', &length, modifying);
                          else if (forward(fmt, "mmm"))
                              {
                              /* compute milliseconds */
                              char *msptr = strchr(in_time, '.');
                              int msec =  NULL == msptr?  0  :  1000 * atof(msptr);
                              translate_num("mmm", &fmt, &out, msec, 3, '0', &length, modifying);
                              }
                          else if (fmt[1] == 'm')    /* mm -- months or minutes? */
                              {
                              if (month_context(start,fmt,2))
                                  translate_num("mm", &fmt, &out, month, 2, '0', &length, modifying);
                              else if (minute_context(start,fmt,2))
                                  translate_num("mm", &fmt, &out, t->tm_min, 2, '0', &length, modifying);
                              else
                                  {    /* Can't tell -- just put the 'mm' in there unchanged. */
                                  translate_str("mm", &fmt, &out, "mm", first,rest, &length, modifying);
                                  }
                              }
                          else    /* m -- months or minutes? */
                              {
                              if (month_context(start,fmt,1))
                                  translate_num("m", &fmt, &out, month, 0, 'x', &length, modifying);
                              else if (minute_context(start,fmt,1))
                                  translate_num("m", &fmt, &out, t->tm_min, 0, 'x', &length, modifying);
                              else
                                  {    /* Can't tell -- just put the 'm' in there unchanged. */
                                  translate_str("m", &fmt, &out, "m", first,rest, &length, modifying);
                                  }
                              }
                          break;

                    case 'M':        /* Month, MONTH, Mon, MON */
                          
                          if (forward(fmt, "Month"))
                              {
                              if (!checked_locale++) setup_locale();
                              translate_str("Month", &fmt, &out, Month[t->tm_mon], first,rest, &length, modifying);
                              }
                          else if (forward(fmt, "MONTH"))
                              {
                              f = first==NC? UC : first;
                              r = rest ==NC? UC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("MONTH", &fmt, &out, Month[t->tm_mon], f,r, &length, modifying);
                              }
                          else if (forward(fmt, "Mon"))
                              {
                              if (!checked_locale++) setup_locale();
                              translate_str("Mon", &fmt, &out, Mon[t->tm_mon], first,rest, &length, modifying);
                              }
                          else if (forward(fmt, "MON"))
                              {
                              f = first==NC? UC : first;
                              r = rest ==NC? UC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("MON", &fmt, &out, Mon[t->tm_mon], f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 'W':        /* Weekday, WEEKDAY */

                          if (forward(fmt, "Weekday"))
                              {
                              if (!checked_locale++) setup_locale();
                              translate_str("Weekday", &fmt, &out, Weekday[t->tm_wday], first,rest, &length, modifying);
                              }
                          else if (forward(fmt, "WEEKDAY"))
                              {
                              f = first==NC? UC : first;
                              r = rest ==NC? UC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("WEEKDAY", &fmt, &out, Weekday[t->tm_wday], f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 'w':        /* weekday */

                          if (forward(fmt, "weekday"))
                              {
                              f = first==NC? LC : first;
                              r = rest ==NC? LC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("weekday", &fmt, &out, Weekday[t->tm_wday], f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 'D':        /* Day, DAY */

                          if (forward(fmt, "Day"))
                              {
                              if (!checked_locale++) setup_locale();
                              translate_str("Day", &fmt, &out, Day[t->tm_wday], first,rest, &length, modifying);
                              }
                          else if (forward(fmt, "DAY"))
                              {
                              f = first==NC? UC : first;
                              r = rest ==NC? UC : rest;
                              if (!checked_locale++) setup_locale();
                              translate_str("DAY", &fmt, &out, Day[t->tm_wday], f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 'a':        /* am, a.m. */
                    case 'p':        /* pm, p.m. */
                    case 'A':        /* AM, A.M. */
                    case 'P':        /* PM, P.M. */

                          if (forward(fmt, "am")  ||  forward(fmt, "pm")  ||  forward(fmt, "AM")  ||  forward(fmt, "PM"))
                              {
                              f = first==NC? (fmt[1]=='m'? LC : UC) : first;
                              r = rest ==NC? (fmt[1]=='m'? LC : UC) : rest;
                              char *ap  = t->tm_hour < 12? "am"   : "pm";
                              translate_str("am", &fmt, &out, ap, f,r, &length, modifying);
                              }
                          else if (forward(fmt, "a.m.")  ||  forward(fmt, "p.m.")  ||  forward(fmt, "A.M.")  ||  forward(fmt, "P.M."))
                              {
                              f = first==NC? (fmt[2]=='m'? LC : UC) : first;
                              r = rest ==NC? (fmt[2]=='m'? LC : UC) : rest;
                              char *a_p_= t->tm_hour < 12? "a.m." : "p.m.";
                              translate_str("a.m.", &fmt, &out, a_p_, f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case '?':        /* ?d, ?h, ?H, ?s, ?m{on}, ?m{in}, ?m */

                          switch (fmt[1])
                              {
                                  case 'd':
                                        translate_num("?d", &fmt, &out, t->tm_mday, 2, ' ', &length, modifying);
                                        break;
                                  case 'h':
                                        translate_num("?h", &fmt, &out, t->tm_hour, 2, ' ', &length, modifying);
                                        break;
                                  case 'H':
                                        translate_num("?H", &fmt, &out, hour, 2, ' ', &length, modifying);
                                        break;
                                  case 's':
                                        translate_num("?s", &fmt, &out, t->tm_sec, 2, ' ', &length, modifying);
                                        break;
                                  case 'm':
                                        if (forward(fmt, "?m{on}"))
                                            translate_num("?m{on}", &fmt, &out, month, 2, ' ', &length, modifying);
                                        else if (forward(fmt, "?m{in}"))
                                            translate_num("?m{in}", &fmt, &out, t->tm_min, 2, ' ', &length, modifying);
                                        else    /* ?m -- months or minutes? */
                                            {
                                            if (month_context(start,fmt,2))
                                                translate_num("?m", &fmt, &out, month, 2, ' ', &length, modifying);
                                            else if (minute_context(start,fmt,2))
                                                translate_num("?m", &fmt, &out, t->tm_min, 2, ' ', &length, modifying);
                                            else
                                                {    /* Can't tell -- just put the '?m' in there unchanged. */
                                                translate_str("?m", &fmt, &out, "?m", first,rest, &length, modifying);
                                                }
                                            }
                                        break;

                                  default:   /* just a question mark */
                                        if (modifying) *out++=*fmt++; else length++, fmt++;
                              }
                          break;

                    case 'u':        /* uuuuuu (microseconds) */

                          if (forward(fmt, "uuuuuu"))
                              {
                              /* compute microseconds */
                              char *msptr = strchr(in_time, '.');
                              int msec =  NULL == msptr?  0  :  1000000 * atof(msptr);
                              translate_num("uuuuuu", &fmt, &out, msec, 6, '0', &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    case 't':        /* th, tz */
                    case 'T':        /* TH */

                          if (fmt[1] == 'z')
                              {
                              char zone[32];   /* I'm assuming 32 is enough... */
                              (void) strftime(zone, 32, "%Z", t);
                              translate_str("tz", &fmt, &out, zone, first,rest, &length, modifying);
                              }
                          else if (forward(fmt, "th") || forward(fmt, "TH"))    /* day ordinal suffix */
                              {
                              f = first==NC? (*fmt=='t'? LC : UC) : first;
                              r = rest ==NC? (*fmt=='t'? LC : UC) : rest;
                              static char *suffix[] = {"th", "st", "nd", "rd"};
                              int ones = t->tm_mday % 10;
                              int tens = t->tm_mday % 100 / 10;
                              if (tens == 1  ||  ones > 3) ones = 0;

                              translate_str("th", &fmt, &out, suffix[ones], f,r, &length, modifying);
                              }
                          else if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;

                    default:
                          if (modifying) *out++=*fmt++; else length++, fmt++;
                          break;
                }

            if (reset_uclc)
                lowernext = uppernext = 0;
            }
        if (modifying)
            *out = '\0';
        else
            {
            ret = out = malloc(length+1);
            if (NULL == ret) return ret;  /* Yikes */
            fmt=start;    /* Start over! */
            }
        }

    return ret;
    }

/*
-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.2.2 (GNU/Linux)

iD8DBQE/G0taY96i4h5M0egRAu2JAKDlpcYj1R5opzKy2eJaTZUoMXZH6gCghUNk
0DJrwiK3xple1mAYwypwSs0=
=WLuA
-----END PGP SIGNATURE-----
*/
