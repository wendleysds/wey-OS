#ifndef _CLIB_CTYPE_H
#define _CLIB_CTYPE_H

/**
 * @brief Checks whether a character is an alphabetic letter.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a letter (A-Z or a-z), zero otherwise.
 */
int isalpha(int c);

/**
 * @brief Checks whether a character is an uppercase letter.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is an uppercase letter, zero otherwise.
 */
int isupper(int c);

/**
 * @brief Checks whether a character is a lowercase letter.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a lowercase letter, zero otherwise.
 */
int islower(int c);

/**
 * @brief Checks whether a character is a digit.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a digit (0-9), zero otherwise.
 */
int isdigit(int c);

/**
 * @brief Checks whether a character is alphanumeric.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a letter or a digit, zero otherwise.
 */
int isalnum(int c);

/**
 * @brief Checks whether a character is a hexadecimal digit.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a hexadecimal digit (0-9, a-f, A-F), zero otherwise.
 */
int isxdigit(int c);

/**
 * @brief Checks whether a character is a control character.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a control character, zero otherwise.
 */
int iscntrl(int c);

/**
 * @brief Checks whether a character is a graphical character (has a visual representation).
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a graphical character, zero otherwise.
 */
int isgraph(int c);

/**
 * @brief Checks whether a character is a whitespace character.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a whitespace character (space, form feed, newline, 
 *         carriage return, horizontal tab, vertical tab), zero otherwise.
 */
int isspace(int c);

/**
 * @brief Checks whether a character is a printable character.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a printable character, zero otherwise.
 */
int isprint(int c);

/**
 * @brief Checks whether a character is a punctuation character.
 *
 * @param c The character to check (int).
 * @return Non-zero if c is a punctuation character, zero otherwise.
 */
int ispunct(int c);

/**
 * @brief Converts an uppercase letter to lowercase.
 *
 * @param c The character to convert (int).
 * @return If c is an uppercase letter, returns the lowercase equivalent. 
 *         Otherwise, returns c unchanged.
 */
int tolower(int c);

/**
 * @brief Converts a lowercase letter to uppercase.
 *
 * @param c The character to convert (int).
 * @return If c is a lowercase letter, returns the uppercase equivalent. 
 *         Otherwise, returns c unchanged.
 */
int toupper(int c);

#define _toupper(c) toupper((int)(c))
#define _tolower(c) tolower((int)(c))

#endif