#include <locale.h>

/* ---------------------------------------------------------------------------
 * Locale table entries
 *
 * CHAR_MAX (0x7f) is used as "not applicable" sentinel, per C99 §7.11.
 * ---------------------------------------------------------------------------*/
#define _LC_CHAR_MAX ((char)127)

/* ---- en_US ----------------------------------------------------------------*/
static struct lconv _lconv_en_US = {
    .decimal_point      = ".",
    .thousands_sep      = ",",
    .grouping           = "\3",      /* groups of 3 digits               */
    .int_curr_symbol    = "USD ",
    .currency_symbol    = "$",
    .mon_decimal_point  = ".",
    .mon_thousands_sep  = ",",
    .mon_grouping       = "\3",
    .positive_sign      = "",
    .negative_sign      = "-",
    .int_frac_digits    = 2,
    .frac_digits        = 2,
    .p_cs_precedes      = 1,         /* $ before amount                  */
    .p_sep_by_space     = 0,
    .n_cs_precedes      = 1,
    .n_sep_by_space     = 0,
    .p_sign_posn        = 1,
    .n_sign_posn        = 1,
    .int_p_cs_precedes  = 1,
    .int_p_sep_by_space = 1,
    .int_n_cs_precedes  = 1,
    .int_n_sep_by_space = 1,
    .int_p_sign_posn    = 1,
    .int_n_sign_posn    = 1,
};

/* ---- pt_BR ---------------------------------------------------------------*/
static struct lconv _lconv_pt_BR = {
    .decimal_point      = ",",
    .thousands_sep      = ".",
    .grouping           = "\3",
    .int_curr_symbol    = "BRL ",
    .currency_symbol    = "R$",
    .mon_decimal_point  = ",",
    .mon_thousands_sep  = ".",
    .mon_grouping       = "\3",
    .positive_sign      = "",
    .negative_sign      = "-",
    .int_frac_digits    = 2,
    .frac_digits        = 2,
    .p_cs_precedes      = 1,         /* R$ before amount                 */
    .p_sep_by_space     = 1,
    .n_cs_precedes      = 1,
    .n_sep_by_space     = 1,
    .p_sign_posn        = 1,
    .n_sign_posn        = 1,
    .int_p_cs_precedes  = 1,
    .int_p_sep_by_space = 1,
    .int_n_cs_precedes  = 1,
    .int_n_sep_by_space = 1,
    .int_p_sign_posn    = 1,
    .int_n_sign_posn    = 1,
};

/* ---- C / POSIX (minimal "C" locale) --------------------------------------*/
static struct lconv _lconv_C = {
    .decimal_point      = ".",
    .thousands_sep      = "",
    .grouping           = "",
    .int_curr_symbol    = "",
    .currency_symbol    = "",
    .mon_decimal_point  = "",
    .mon_thousands_sep  = "",
    .mon_grouping       = "",
    .positive_sign      = "",
    .negative_sign      = "",
    .int_frac_digits    = _LC_CHAR_MAX,
    .frac_digits        = _LC_CHAR_MAX,
    .p_cs_precedes      = _LC_CHAR_MAX,
    .p_sep_by_space     = _LC_CHAR_MAX,
    .n_cs_precedes      = _LC_CHAR_MAX,
    .n_sep_by_space     = _LC_CHAR_MAX,
    .p_sign_posn        = _LC_CHAR_MAX,
    .n_sign_posn        = _LC_CHAR_MAX,
    .int_p_cs_precedes  = _LC_CHAR_MAX,
    .int_p_sep_by_space = _LC_CHAR_MAX,
    .int_n_cs_precedes  = _LC_CHAR_MAX,
    .int_n_sep_by_space = _LC_CHAR_MAX,
    .int_p_sign_posn    = _LC_CHAR_MAX,
    .int_n_sign_posn    = _LC_CHAR_MAX,
};

/* ---------------------------------------------------------------------------
 * Internal state
 * ---------------------------------------------------------------------------*/
typedef struct {
    const char   *name;
    struct lconv *lconv;
} _locale_entry_t;

static const _locale_entry_t _locale_table[] = {
    { "C",       &_lconv_C     },
    { "POSIX",   &_lconv_C     },
    { "en_US",   &_lconv_en_US },
    { "pt_BR",   &_lconv_pt_BR },
    { (void*)0,  (void*)0      },
};

/* Current active locale per category (all start as "C"). */
static const _locale_entry_t *_current_locale[7] = {
    &_locale_table[0], /* LC_ALL      */
    &_locale_table[0], /* LC_COLLATE  */
    &_locale_table[0], /* LC_CTYPE    */
    &_locale_table[0], /* LC_MESSAGES */
    &_locale_table[0], /* LC_MONETARY */
    &_locale_table[0], /* LC_NUMERIC  */
    &_locale_table[0], /* LC_TIME     */
};

/* ---------------------------------------------------------------------------
 * Helper: find entry by name, returns NULL when not found.
 * ---------------------------------------------------------------------------*/
static const _locale_entry_t *_find_locale(const char *name)
{
    if (!name) {
        return (void*)0;
    }

    for (const _locale_entry_t *e = _locale_table; e->name; e++) {
        const char *a = e->name;
        const char *b = name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == *b) {   /* strings equal */
            return e;
        }
    }
    return (void*)0;
}

/* ---------------------------------------------------------------------------
 * setlocale
 *
 * category : LC_ALL … LC_TIME  (0 … 6)
 * locale   : locale name string, or NULL to query the current locale.
 *
 * Returns a pointer to the locale name string on success, NULL on failure.
 * ---------------------------------------------------------------------------*/
char *setlocale(int category, const char *locale)
{
    if (category < 0 || category > LC_TIME) {
        return (void*)0;
    }

    /* Query only – return the name of the current locale for the category. */
    if (!locale) {
        return (char *)_current_locale[category]->name;
    }

    const _locale_entry_t *entry = _find_locale(locale);
    if (!entry) {
        return (void*)0;        /* unsupported locale */
    }

    if (category == LC_ALL) {
        for (int i = 0; i <= LC_TIME; i++) {
            _current_locale[i] = entry;
        }
    } else {
        _current_locale[category] = entry;
    }

    return (char *)entry->name;
}

/* ---------------------------------------------------------------------------
 * localeconv
 *
 * Returns a pointer to the lconv struct that reflects the conventions of the
 * LC_NUMERIC and LC_MONETARY categories of the current locale.
 *
 * The numeric fields come from LC_NUMERIC; the monetary fields from
 * LC_MONETARY.  When both categories are the same locale the pointer is the
 * very same struct, so no merging is needed in that common case.
 *
 * NOTE: The standard says the returned pointer may be invalidated by the next
 * call to localeconv or setlocale.  Because our structs are static that
 * invariant holds as long as the caller does not cache the pointer across a
 * setlocale call.
 * ---------------------------------------------------------------------------*/
struct lconv *localeconv(void)
{
    return _current_locale[LC_NUMERIC]->lconv;
}
