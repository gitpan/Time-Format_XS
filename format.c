/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

The GPG signature in this file may be checked with 'gpg --verify format.c'. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <config.h>

#ifdef I_LANGINFO
#include <langinfo.h>
#endif

static char _VERSION[] = "0.13";

/* format.c, version 0.13

This is part of the Time::Format_XS module.  See the .pm file for documentation.

This code is copyright (c) 2003 by Eric J. Roode -- all rights reserved.

See the Changes file for change history.

*/

typedef struct state_struct
{
    int year, month, day, hour, min, sec, dow;
    int micro, milli;
    char am;
    int h12;
    size_t length;
    const char *start, *fmt;
    char *out, *outptr;
    int modifying;
    int upper, lower, ucnext, lcnext;
    int quoting;
    int checked_locale;
    struct tm *tm;
} *state;


/* Month and weekday names, and their abbreviations.  Populated by setup_locale. */
static char *Month_Name[12];
static char *Mon_Name[12];
static char *Weekday_Name[7];
static char *Day_Name[7];

/* setup_locale
Populate day/month names based on current locale.
Alas, I don't know how portable this code is.
*/
static void setup_locale(void)
    {
    static int checked_locale = 0;

    char *cur_locale;
    static char prev_locale[40];

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

#ifdef HAS_NL_LANGINFO
    if (NULL != cur_locale)
        {
        strcpy(prev_locale, cur_locale);
        Month_Name[0]   = nl_langinfo(MON_1);
        Month_Name[1]   = nl_langinfo(MON_2);
        Month_Name[2]   = nl_langinfo(MON_3);
        Month_Name[3]   = nl_langinfo(MON_4);
        Month_Name[4]   = nl_langinfo(MON_5);
        Month_Name[5]   = nl_langinfo(MON_6);
        Month_Name[6]   = nl_langinfo(MON_7);
        Month_Name[7]   = nl_langinfo(MON_8);
        Month_Name[8]   = nl_langinfo(MON_9);
        Month_Name[9]   = nl_langinfo(MON_10);
        Month_Name[10]  = nl_langinfo(MON_11);
        Month_Name[11]  = nl_langinfo(MON_12);
        Mon_Name[0]     = nl_langinfo(ABMON_1);
        Mon_Name[1]     = nl_langinfo(ABMON_2);
        Mon_Name[2]     = nl_langinfo(ABMON_3);
        Mon_Name[3]     = nl_langinfo(ABMON_4);
        Mon_Name[4]     = nl_langinfo(ABMON_5);
        Mon_Name[5]     = nl_langinfo(ABMON_6);
        Mon_Name[6]     = nl_langinfo(ABMON_7);
        Mon_Name[7]     = nl_langinfo(ABMON_8);
        Mon_Name[8]     = nl_langinfo(ABMON_9);
        Mon_Name[9]     = nl_langinfo(ABMON_10);
        Mon_Name[10]    = nl_langinfo(ABMON_11);
        Mon_Name[11]    = nl_langinfo(ABMON_12);
        Weekday_Name[0] = nl_langinfo(DAY_1);
        Weekday_Name[1] = nl_langinfo(DAY_2);
        Weekday_Name[2] = nl_langinfo(DAY_3);
        Weekday_Name[3] = nl_langinfo(DAY_4);
        Weekday_Name[4] = nl_langinfo(DAY_5);
        Weekday_Name[5] = nl_langinfo(DAY_6);
        Weekday_Name[6] = nl_langinfo(DAY_7);
        Day_Name[0]     = nl_langinfo(ABDAY_1);
        Day_Name[1]     = nl_langinfo(ABDAY_2);
        Day_Name[2]     = nl_langinfo(ABDAY_3);
        Day_Name[3]     = nl_langinfo(ABDAY_4);
        Day_Name[4]     = nl_langinfo(ABDAY_5);
        Day_Name[5]     = nl_langinfo(ABDAY_6);
        Day_Name[6]     = nl_langinfo(ABDAY_7);
        return;
        }
#endif

    /* Couldn't set up locale for some reason.  Use English. */
    Month_Name[0]   = "January";
    Month_Name[1]   = "February";
    Month_Name[2]   = "March";
    Month_Name[3]   = "April";
    Month_Name[4]   = "May";
    Month_Name[5]   = "June";
    Month_Name[6]   = "July";
    Month_Name[7]   = "August";
    Month_Name[8]   = "September";
    Month_Name[9]   = "October";
    Month_Name[10]  = "November";
    Month_Name[11]  = "December";
    Mon_Name[0]     = "Jan";
    Mon_Name[1]     = "Feb";
    Mon_Name[2]     = "Mar";
    Mon_Name[3]     = "Apr";
    Mon_Name[4]     = "May";
    Mon_Name[5]     = "Jun";
    Mon_Name[6]     = "Jul";
    Mon_Name[7]     = "Aug";
    Mon_Name[8]     = "Sep";
    Mon_Name[9]     = "Oct";
    Mon_Name[10]    = "Nov";
    Mon_Name[11]    = "Dec";
    Weekday_Name[0] = "Sunday";
    Weekday_Name[1] = "Monday";
    Weekday_Name[2] = "Tuesday";
    Weekday_Name[3] = "Wednesday";
    Weekday_Name[4] = "Thursday";
    Weekday_Name[5] = "Friday";
    Weekday_Name[6] = "Saturday";
    Day_Name[0]     = "Sun";
    Day_Name[1]     = "Mon";
    Day_Name[2]     = "Tue";
    Day_Name[3]     = "Wed";
    Day_Name[4]     = "Thu";
    Day_Name[5]     = "Fri";
    Day_Name[6]     = "Sat";
    return;
    }

#define RESET_UCLC       self->ucnext = self->lcnext = 0
#define OUTPUSH(c)       *(self->outptr)++ = (c)
#define OUT_FIRST_UPPER(c)     *(self->outptr)++ = (self->lcnext||(self->lower&&!self->ucnext))? tolower(c) : toupper(c)
#define OUT_FIRST_LOWER(c)     *(self->outptr)++ = (self->ucnext||(self->upper&&!self->lcnext))? toupper(c) : tolower(c)
#define OUT_FIRST_MIXED(c)     *(self->outptr)++ = self->ucnext? toupper(c) : self->lcnext? tolower(c) : self->upper? toupper(c) : self->lower? tolower(c) : (c)
#define OUT_REST_UPPER(c)      *(self->outptr)++ = self->lower? tolower(c) : toupper(c)
#define OUT_REST_LOWER(c)      *(self->outptr)++ = self->upper? toupper(c) : tolower(c)
#define OUT_REST_MIXED(c)      *(self->outptr)++ = self->upper? toupper(c) : self->lower? tolower(c) : (c)

static int pack_02d (char *out, int num)
    {
    int t = num / 10;    /* tens position */
    *out++ = t + '0';
    num -= t*10;
    *out = num + '0';
    return 2;
    }
static int pack_2d (char *out, int num)
    {
    int t = num/10;
    if (t)
        {
        *out++ = t + '0';
        num -= t*10;
        }
    else
        *out++ = ' ';
    *out = num + '0';
    return 2;
    }
static int pack_d (char *out, int num)
    {
    int t = num/10;
    int rv = 1;
    if (t)
        {
        rv++;
        *out++ = t + '0';
        num -= t*10;
        }
    *out = num + '0';
    return rv;
    }

static void standard_x (state self, int num)
    {
    if (self->modifying)
        self->outptr += pack_d (self->outptr, num);
    else
        self->length += num>9? 2 : 1;
    self->fmt += 1;
    RESET_UCLC;
    }
static void standard_xx (state self, int num)
    {
    if (self->modifying)
        self->outptr += pack_02d (self->outptr, num);
    else
        self->length += 2;
    self->fmt += 2;
    RESET_UCLC;
    }
static void standard__x (state self, int num)   /* ?x */
    {
    if (self->modifying)
        self->outptr += pack_2d (self->outptr, num);
    else
        self->length += 2;
    self->fmt += 2;
    RESET_UCLC;
    }


static void yyyy (state self)
    {
    if (self->modifying)
        {
        int c = self->year/100;
        int y = self->year%100;
        self->outptr += pack_02d(self->outptr, c);
        self->outptr += pack_02d(self->outptr, y);
        }
    else
        self->length += 4;
    self->fmt += 4;
    RESET_UCLC;
    }
static void yy (state self)
    {
    standard_xx(self, self->year%100);
    }

static void mm_on_ (state self)    /* mm{on} */
    {
    standard_xx(self, self->month);
    self->fmt += 4;
    }
static void m_on_ (state self)     /* m{on} */
    {
    standard_x(self, self->month);
    self->fmt += 4;
    }
static void _m_on_ (state self)    /* ?m{on} */
    {
    standard__x(self, self->month);
    self->fmt += 4;
    }

static void dd (state self)
    {
    standard_xx (self, self->day);
    }
static void d (state self)
    {
    standard_x (self, self->day);
    }
static void _d (state self)
    {
    standard__x (self, self->day);
    }

static void hh (state self)
    {
    standard_xx (self, self->hour);
    }
static void h (state self)
    {
    standard_x (self, self->hour);
    }
static void _h (state self)
    {
    standard__x (self, self->hour);
    }

static void get_h12(state self)
    {
    if (self->h12) return;
    self->h12 = self->hour % 12;
    if (self->h12 == 0) self->h12 = 12;
    self->am = self->hour<12? 'a' : 'p';
    }
static void HH (state self)
    {
    get_h12(self);
    standard_xx (self, self->h12);
    }
static void H (state self)
    {
    get_h12(self);
    standard_x (self, self->h12);
    }
static void _H (state self)
    {
    get_h12(self);
    standard__x (self, self->h12);
    }

static void mm_in_ (state self)    /* mm{in} */
    {
    standard_xx(self, self->min);
    self->fmt += 4;
    }
static void m_in_ (state self)     /* m{in} */
    {
    standard_x(self, self->min);
    self->fmt += 4;
    }
static void _m_in_ (state self)    /* ?m{in} */
    {
    standard__x(self, self->min);
    self->fmt += 4;
    }

static void ss (state self)
    {
    standard_xx (self, self->sec);
    }
static void s (state self)
    {
    standard_x (self, self->sec);
    }
static void _s (state self)
    {
    standard__x (self, self->sec);
    }

static void mmm (state self)
    {
    self->fmt += 3;
    if (!self->modifying)
        {
        self->length += 3;
        return;
        }
    RESET_UCLC;
    if (self->milli == 0)
        {
        OUTPUSH('0');
        OUTPUSH('0');
        OUTPUSH('0');
        }
    else
        {
        int h  = self->milli / 100;
        int to = self->milli % 100;
        OUTPUSH(h + '0');
        self->outptr += pack_02d (self->outptr, to);
        }
    }

static void uuuuuu (state self)
    {
    self->fmt += 6;
    if (!self->modifying)
        {
        self->length += 6;
        return;
        }
    RESET_UCLC;
    if (self->micro == 0)
        {
        OUTPUSH('0');
        OUTPUSH('0');
        OUTPUSH('0');
        OUTPUSH('0');
        OUTPUSH('0');
        OUTPUSH('0');
        }
    else
        {
        int u  = self->micro/100;
        int u3 = self->micro % 100;
        int u2 = u % 100;
        int u1 = u / 100;
        self->outptr += pack_02d (self->outptr, u1);
        self->outptr += pack_02d (self->outptr, u2);
        self->outptr += pack_02d (self->outptr, u3);
        }
    }

/* Ambiguous mm, ?m, m codes */
static void mm (state self)
    {
    if (!self->modifying)
        {
        self->length += 2;
        self->fmt    += 2;
        return;
        }

    if (month_context (self, 2))
        return standard_xx(self, self->month);

    if (minute_context(self, 2))
        return standard_xx(self, self->min);

    OUT_FIRST_LOWER('m');
    OUT_REST_LOWER('m');
    self->fmt += 2;
    RESET_UCLC;
    }
static void m (state self)
    {
    if (month_context (self, 2))
        {
        standard_x(self, self->month);
        return;
        }

    if (minute_context(self, 2))
        {
        standard_x(self, self->min);
        return;
        }

    if (!self->modifying)
        {
        self->length += 1;
        self->fmt    += 1;
        return;
        }

    OUT_FIRST_LOWER('m');
    self->fmt += 1;
    RESET_UCLC;
    }
static void _m (state self)
    {
    if (!self->modifying)
        {
        self->length += 2;
        self->fmt    += 2;
        return;
        }

    if (month_context (self, 2))
        {
        standard__x(self, self->month);
        return;
        }

    if (minute_context(self, 2))
        {
        standard__x(self, self->min);
        return;
        }

    OUTPUSH('?');
    OUT_REST_LOWER('m');
    self->fmt += 2;
    RESET_UCLC;
    }

static char *suffix[] = {"th", "st", "nd", "rd"};
static void th (state self)
    {
    int ones, tens;
    self->fmt += 2;
    if (!self->modifying)
        {
        self->length += 2;
        return;
        }
    ones = self->day % 10;
    tens = self->day / 10;
    if (tens == 1  ||  ones > 3) ones = 0;

    OUT_FIRST_LOWER(suffix[ones][0]);
    OUT_REST_LOWER (suffix[ones][1]);
    RESET_UCLC;
    return;
    }
static void TH (state self)
    {
    int ones, tens;
    self->fmt += 2;
    if (!self->modifying)
        {
        self->length += 2;
        return;
        }
    ones = self->day % 10;
    tens = self->day / 10;
    if (tens == 1  ||  ones > 3) ones = 0;

    OUT_FIRST_UPPER(suffix[ones][0]);
    OUT_REST_UPPER (suffix[ones][1]);
    RESET_UCLC;
    }

static void am (state self)
    {
    self->fmt += 2;
    if (!self->modifying)
        {
        self->length += 2;
        return;
        }
    get_h12(self);
    OUT_FIRST_LOWER(self->am);
    OUT_REST_LOWER('m');
    RESET_UCLC;
    }
static void pm (state self)
    {
    am(self);
    }
static void AM (state self)
    {
    self->fmt += 2;
    if (!self->modifying)
        {
        self->length += 2;
        return;
        }
    get_h12(self);
    OUT_FIRST_UPPER(self->am);
    OUT_REST_UPPER('M');
    RESET_UCLC;
    }
static void PM (state self)
    {
    AM(self);
    }
static void a_m_ (state self)
    {
    self->fmt += 4;
    if (!self->modifying)
        {
        self->length += 4;
        return;
        }
    get_h12(self);
    OUT_FIRST_LOWER(self->am);
    OUTPUSH('.');
    OUT_REST_LOWER('m');
    OUTPUSH('.');
    RESET_UCLC;
    }
static void p_m_ (state self)
    {
    a_m_(self);
    }
static void A_M_ (state self)
    {
    self->fmt += 4;
    if (!self->modifying)
        {
        self->length += 4;
        return;
        }
    get_h12(self);
    OUT_FIRST_UPPER(self->am);
    OUTPUSH('.');
    OUT_REST_UPPER('M');
    OUTPUSH('.');
    RESET_UCLC;
    }
static void P_M_ (state self)
    {
    A_M_(self);
    }

static void packstr_mc(state self, int fmtlen, const char *name)
    {
    int ch;
    self->fmt += fmtlen;
    if (!self->modifying)
        {
        self->length += strlen(name);
        return;
        }

    OUT_FIRST_MIXED(*name);
    while (*++name)
        OUT_REST_MIXED(*name);
    RESET_UCLC;
    }
static void packstr_uc(state self, int fmtlen, const char *name)
    {
    int ch;
    self->fmt += fmtlen;
    if (!self->modifying)
        {
        self->length += strlen(name);
        return;
        }

    OUT_FIRST_UPPER(*name);
    while (*++name)
        OUT_REST_UPPER(*name);
    RESET_UCLC;
    }
static void packstr_lc(state self, int fmtlen, const char *name)
    {
    int ch;
    self->fmt += fmtlen;
    if (!self->modifying)
        {
        self->length += strlen(name);
        return;
        }

    OUT_FIRST_LOWER(*name);
    while (*++name)
        OUT_REST_LOWER(*name);
    RESET_UCLC;
    }
static void packstr_mc_limit(state self, int fmtlen, const char *name, size_t limit)
    {
    int ch;
    self->fmt += fmtlen;
    if (!limit)  return;   /* output length zero */

    if (!self->modifying)
        {
        self->length += limit;
        return;
        }

    OUT_FIRST_MIXED(*name);
    while (*++name  &&  --limit)
        OUT_REST_MIXED(*name);
    RESET_UCLC;
    }

static void Month (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_mc(self, 5, Month_Name[self->month-1]);
    }
static void MONTH (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_uc(self, 5, Month_Name[self->month-1]);
    }
static void month (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_lc(self, 5, Month_Name[self->month-1]);
    }

static void Mon (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_mc(self, 3, Mon_Name[self->month-1]);
    }
static void MON (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_uc(self, 3, Mon_Name[self->month-1]);
    }
static void mon (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_lc(self, 3, Mon_Name[self->month-1]);
    }

static void Weekday (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_mc(self, 7, Weekday_Name[self->dow]);
    }
static void WEEKDAY (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_uc(self, 7, Weekday_Name[self->dow]);
    }
static void weekday (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_lc(self, 7, Weekday_Name[self->dow]);
    }

static void Day (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_mc(self, 3, Day_Name[self->dow]);
    }
static void DAY (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_uc(self, 3, Day_Name[self->dow]);
    }
static void day (state self)
    {
    if (!self->checked_locale++)  setup_locale();
    packstr_lc(self, 3, Day_Name[self->dow]);
    }

static void tz (state self)
    {
#ifdef HAVE_TM_ZONE
    packstr_mc(self, 2, self->tm->tm_zone);
#else
    char zone[32];   /* I'm assuming 32 is enough... */
    (void) strftime(zone, 32, "%Z", self->tm);
    packstr_mc(self, 2, zone);
#endif
    }

static void literal (state self)
    {
    if (!self->modifying)
        {
        self->length++;
        self->fmt++;
        return;
        }
    *(self->outptr)++ = *(self->fmt++);
    }
static void bs_literal (state self)    /* literal preceded by a backslash */
    {
    self->fmt++;
    literal(self);
    }

static void bs_Q (state self)
    {
    self->fmt += 2;
    self->quoting = 1;
    }
static void bs_E (state self)
    {
    self->fmt += 2;
    self->quoting = self->upper = self->lower = self->lcnext = self->ucnext = 0;
    }
static void bs_U (state self)
    {
    self->fmt += 2;
    self->upper = 1;
    }
static void bs_L (state self)
    {
    self->fmt += 2;
    self->lower = 1;
    }
static void bs_u (state self)
    {
    self->fmt += 2;
    self->lcnext = 0;
    self->ucnext = 1;
    }
static void bs_l (state self)
    {
    self->fmt += 2;
    self->ucnext = 0;
    self->lcnext = 1;
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
    if (fmt - start < patlen)  return 0;
    fmt -= patlen;
    if (strncmp(fmt, pat, patlen))   return 0;

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
int month_context(state self, size_t patlen)
    {
    const char *backskip = self->fmt-2;
    const char *fwdskip  = self->fmt + patlen + 1;
    if (*backskip != '\\') backskip++;
    if (*fwdskip == '\\') fwdskip++;

    return  forward(self->fmt+patlen, "?d")
        ||  forward(self->fmt+patlen , "d")
        ||  forward(fwdskip,          "?d")
        ||  forward(fwdskip,           "d")
        ||  forward(self->fmt+patlen, "yy")
        ||  forward(fwdskip,          "yy")
        ||  backward(self->start, self->fmt, "yy")
        ||  backward(self->start, backskip,  "yy")
        ||  backward(self->start, self->fmt,  "d")
        ||  backward(self->start, backskip,   "d");
    }

/* bool = minute_context(start, fmt, patlen);
   Returns TRUE if the current format is in a "minute" context.
   That is, if it's preceeded by an hour and/or followed by a second.
 */
int minute_context(state self, size_t patlen)
    {
    const char *backskip = self->fmt-1;
    const char *fwdskip  = self->fmt+patlen+1;
    if (*backskip == '\\') backskip--;
    if (*fwdskip == '\\')  fwdskip++;

    return  forward(self->fmt+patlen, "?s")
        ||  forward(self->fmt+patlen,  "s")
        ||  forward(fwdskip,    "?s")
        ||  forward(fwdskip,     "s")
        ||  backward(self->start, self->fmt, "h")
        ||  backward(self->start, backskip,  "h")
        ||  backward(self->start, self->fmt, "H")
        ||  backward(self->start, backskip,  "H");
    }

#define THISCHAR      (st->fmt[0])
#define NEXTCHAR      (st->fmt[1])
#define CHARPLUS2     (st->fmt[2])
#define FORMATCODE(f) (forward(st->fmt, (f)))

/* time_format
Given a format, and a string that represents a time number, returns a malloc'd output string.
The time input is a string, because it could be ten digits plus six or more decimals, which
exceeds the precision of most double-precision floats.  So we parse it into a long and a double.
See the documentation for the Time::Format module on what formats are expanded.
*/
char *time_format(const char *fmt, const char *in_time)
    {
    struct state_struct mystate;
    state st = &mystate;

    /* do time computations here */
    long time = atol(in_time);
    st->tm    =  localtime(&time);
    st->year  = st->tm->tm_year + 1900;
    st->month = st->tm->tm_mon + 1;
    st->day   = st->tm->tm_mday;
    st->hour  = st->tm->tm_hour;
    st->min   = st->tm->tm_min;
    st->sec   = st->tm->tm_sec;
    st->dow   = st->tm->tm_wday;
    st->h12   = 0;

    /* other intialization */
    st->length = 0;
    st->fmt    = st->start = fmt;
    st->checked_locale = 0;
    st->out = st->outptr = NULL;

    /* First, compute length of result string.  Then actually populate it. */
    for (st->modifying=0; st->modifying<=1; st->modifying++)
        {
        st->quoting = st->upper = st->lower = st->ucnext = st->lcnext = 0;

        while (THISCHAR)
            {
            char *jump;

            if (st->quoting)
                jump = strstr(st->fmt, "\\E");    /* look for end of literal-quote */
            else
                jump = strpbrk(st->fmt, "\\dDy?hHsaApPMmWwutT");  /* jump to one of these */

            if (NULL == jump)
                {
                packstr_mc (st, strlen(st->fmt), st->fmt);
                break;
                }
            else if (jump > st->fmt)    /* skip over the section that does not contain codes */
                {
                packstr_mc_limit (st, jump - st->fmt, st->fmt, jump - st->fmt);
                }

            switch (THISCHAR)
                {
                    case '\\':        /* escape character */
                          switch (NEXTCHAR)
                              {
                                  case 'Q':  bs_Q(st);    break;
                                  case 'E':  bs_E(st);    break;
                                  case 'U':  bs_U(st);    break;
                                  case 'L':  bs_L(st);    break;
                                  case 'u':  bs_u(st);    break;
                                  case 'l':  bs_l(st);    break;
                                  default :  bs_literal(st); break;
                              }
                          break;

                    case 'd':        /* dd, day, d */

                          if      (NEXTCHAR == 'd')    dd(st);
                          else if (FORMATCODE("day"))  day(st);
                          else                         d(st);
                          break;

                    case 'y':        /* yyyy, yy */

                          if      (FORMATCODE("yyyy"))  yyyy(st);
                          else if (FORMATCODE("yy"))    yy(st);
                          else                          literal(st);
                          break;

                    case 'h':        /* hh, h */

                          if (NEXTCHAR == 'h')  hh(st);
                          else                  h(st);
                          break;

                    case 'H':        /* HH, H */

                          if (NEXTCHAR == 'H')  HH(st);
                          else                   H(st);
                          break;

                    case 's':        /* ss, s */

                          if (NEXTCHAR == 's')  ss(st);
                          else                  s(st);
                          break;

                    case 'm':        /* month, mon, mm{on}, m{on}, mm{in}, m{in}, mmm, mm, m */

                          if      (FORMATCODE("month"))   month(st);
                          else if (FORMATCODE("mon"))     mon(st);
                          else if (FORMATCODE("mm{on}"))  mm_on_(st);
                          else if (FORMATCODE("m{on}"))   m_on_(st);
                          else if (FORMATCODE("mm{in}"))  mm_in_(st);
                          else if (FORMATCODE("m{in}"))   m_in_(st);
                          else if (FORMATCODE("mmm"))
                              {
                              /* compute milliseconds */
                              char *msptr = strchr(in_time, '.');
                              st->milli  =  NULL == msptr?  0  :  1000 * atof(msptr);
                              mmm(st);
                              }
                          else if (NEXTCHAR == 'm')       mm(st);
                          else                            m(st);
                          break;

                    case 'M':        /* Month, MONTH, Mon, MON */
                          
                          if      (FORMATCODE("Month"))  Month(st);
                          else if (FORMATCODE("MONTH"))  MONTH(st);
                          else if (FORMATCODE("Mon"))    Mon(st);
                          else if (FORMATCODE("MON"))    MON(st);
                          else                           literal(st);
                          break;

                    case 'W':        /* Weekday, WEEKDAY */

                          if      (FORMATCODE("Weekday"))  Weekday(st);
                          else if (FORMATCODE("WEEKDAY"))  WEEKDAY(st);
                          else                             literal(st);
                          break;

                    case 'w':        /* weekday */

                          if      (FORMATCODE("weekday"))   weekday(st);
                          else                              literal(st);
                          break;

                    case 'D':        /* Day, DAY */

                          if      (FORMATCODE("Day"))  Day(st);
                          else if (FORMATCODE("DAY"))  DAY(st);
                          else                         literal(st);
                          break;

                    case 'a':        /* am, a.m. */

                          if      (FORMATCODE("am"))    am(st);
                          else if (FORMATCODE("a.m."))  a_m_(st);
                          else                          literal(st);
                          break;

                    case 'p':        /* pm, p.m. */

                          if      (FORMATCODE("pm"))    pm(st);
                          else if (FORMATCODE("p.m."))  p_m_(st);
                          else                          literal(st);
                          break;

                    case 'A':        /* AM, A.M. */

                          if      (FORMATCODE("AM"))    AM(st);
                          else if (FORMATCODE("A.M."))  A_M_(st);
                          else                          literal(st);
                          break;

                    case 'P':        /* PM, P.M. */

                          if      (FORMATCODE("PM"))    PM(st);
                          else if (FORMATCODE("P.M."))  P_M_(st);
                          else                          literal(st);
                          break;

                    case '?':        /* ?d, ?h, ?H, ?s, ?m{on}, ?m{in}, ?m */

                          switch (NEXTCHAR)
                              {
                                  case 'd':  _d(st);  break;
                                  case 'h':  _h(st);  break;
                                  case 'H':  _H(st);  break;
                                  case 's':  _s(st);  break;
                                  case 'm':
                                        if      (FORMATCODE("?m{on}"))  _m_on_(st);
                                        else if (FORMATCODE("?m{in}"))  _m_in_(st);
                                        else                            _m(st);
                                        break;

                                  default:  literal(st);   /* just a question mark */
                              }
                          break;

                    case 'u':        /* uuuuuu (microseconds) */

                          if (FORMATCODE("uuuuuu"))
                              {
                              /* compute microseconds */
                              char *msptr = strchr(in_time, '.');
                              st->micro  =  NULL == msptr?  0  :  1000000 * atof(msptr);
                              uuuuuu(st);
                              }
                          else
                              literal(st);
                          break;

                    case 't':        /* th, tz */
                          if      (NEXTCHAR == 'h')  th(st);
                          else if (NEXTCHAR == 'z')  tz(st);
                          else                       literal(st);
                          break;

                    case 'T':        /* TH */

                          if      (NEXTCHAR == 'H')  TH(st);
                          else                       literal(st);
                          break;

                    default:
                          literal(st);
                          break;
                }

            }
        if (st->modifying)
            *(st->outptr) = '\0';
        else
            {
            st->out = st->outptr = malloc(st->length+1);
            if (NULL == st->out) return st->out;  /* Yikes */
            st->fmt = st->start;    /* Start over! */
            }
        }

    return st->out;
    }

/*
-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.2.2 (GNU/Linux)

iD8DBQE/JQ5EY96i4h5M0egRAr9HAKCbwTQDO3ihkEcxyS0sBlLtZ90LOACdExY7
GEQQlnd9geWxBIrPfZE+gjI=
=Pysx
-----END PGP SIGNATURE-----
*/
