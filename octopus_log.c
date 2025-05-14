/*******************************************************************************
 * @file     octopus_log.c
 * @brief    Implements UART logging functions to provide logging capabilities
 *           with various log levels (DEBUG, INFO, WARNING, ERROR). Supports
 *           formatted log messages and buffer outputs.
 *
 * This file contains functions that support logging in the system via UART.
 * It allows logging of messages with variable arguments, formatted output,
 * and buffer printing. The log levels are configurable to control the verbosity
 * of logs, making it useful for debugging and tracking system status.
 *
 * The `dbg_log_printf` functions allow formatted messages to be printed to
 * UART, while the `dbg_log_printf_level` function includes the log level and
 * function name in the log output.
 *
 * The logging level can be dynamically set using `dbg_log_set_level`, and
 * specific buffer data can be logged using `dbg_log_printf_buffer` or
 * `dbg_log_printf_buffer_level`.
 *
 * @version  1.0.0
 * @date     2024-12-09
 * @author   Octopus Team
 *******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "octopus_platform.h" // Include platform-specific header for hardware platform details
#include "octopus_log.h"      // Include logging functions for debugging

#define ZEROPAD 1  // Pad with zero
#define SIGN 2     // Unsigned/signed long
#define PLUS 4     // Show plus
#define SPACE 8    // Space if plus
#define LEFT 16    // Left justified
#define SPECIAL 32 // 0x
#define LARGE 64   // Use 'ABCDEF' instead of 'abcdef'
#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define LOG_DEFAULT_MAX_WIDTH 28

DBG_LOG_LEVEL current_log_level = LOG_LEVEL_DEBUG;

#ifdef PLATFORM_CST_OSAL_RTOS
static const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char *upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/**
 * @brief Calculates the length of a string up to a specified limit.
 * @param s The string to be measured.
 * @param count The maximum number of characters to check.
 * @return The length of the string, or `count` if the string exceeds `count`.
 */
static size_t strnlen__(const char *s, size_t count)
{
    const char *sc;
    for (sc = s; *sc != '\0' && count--; ++sc)
        ;
    return sc - s;
}

/**
 * @brief Converts a string to an integer (atoi), skipping non-digit characters.
 * @param s Pointer to the string to process.
 * @return The converted integer.
 */
static int skip_atoi__(const char **s)
{
    int i = 0;
    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

/**
 * @brief Formats and outputs a number in various bases (decimal, hexadecimal, etc.).
 * @param putc Callback function for output.
 * @param num The number to be printed.
 * @param base The base to use (e.g., 10 for decimal, 16 for hexadecimal).
 * @param size The width of the field (for padding).
 * @param precision The minimum number of digits to display.
 * @param type Format flags (e.g., left padding, sign, zero padding).
 */
static void number__(std_putc putc, long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[66];
    const char *dig = digits;
    int i;
    char tmpch;

    if (type & LARGE)
        dig = upper_digits;
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if (type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL)
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
    i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
    {
        while (num != 0)
        {
            tmp[i++] = dig[((unsigned long)num) % (unsigned)base];
            num = ((unsigned long)num) / (unsigned)base;
        }
    }
    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type & (ZEROPAD | LEFT)))
    {
        while (size-- > 0)
        {
            tmpch = ' ';
            putc(&tmpch, 1);
        }
    }
    if (sign)
    {
        putc(&sign, 1);
    }

    if (type & SPECIAL)
    {
        if (base == 8)
        {
            tmpch = '0';
            putc(&tmpch, 1);
        }
        else if (base == 16)
        {
            tmpch = '0';
            putc(&tmpch, 1);
            tmpch = digits[33];
            putc(&tmpch, 1);
        }
    }
    if (!(type & LEFT))
    {
        while (size-- > 0)
        {
            putc(&c, 1);
        }
    }
    while (i < precision--)
    {
        tmpch = '0';
        putc(&tmpch, 1);
    }
    while (i-- > 0)
    {
        tmpch = tmp[i];
        putc(&tmpch, 1);
    }
    while (size-- > 0)
    {
        tmpch = ' ';
        putc(&tmpch, 1);
    }
}

/**
 * @brief Internal function for formatted string processing.
 * @param putc Callback function to handle character output.
 * @param fmt The format string.
 * @param args Variable arguments to process the format string.
 */
static void vsprintf__(std_putc putc, const char *fmt, va_list args)
{
    int len;
    unsigned long num;
    int base;
    char *s;
    int flags;       // Flags to number()
    int field_width; // Width of output field
    int precision;   // Min. # of digits for integers; max number of chars for strings
    int qualifier;   // 'h', 'l', or 'L' for integer fields
    char *tmpstr = NULL;
    int tmpstr_size = 0;
    char tmpch;

    for (; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            if (tmpstr == NULL)
            {
                tmpstr = (char *)fmt;
                tmpstr_size = 0;
            }
            tmpstr_size++;
            continue;
        }
        else if (tmpstr_size)
        {
            putc(tmpstr, tmpstr_size);
            tmpstr = NULL;
            tmpstr_size = 0;
        }

        // Process flags
        flags = 0;
    repeat:
        fmt++; // Skip the first '%'
        switch (*fmt)
        {
        case '-':
            flags |= LEFT;
            goto repeat;
        case '+':
            flags |= PLUS;
            goto repeat;
        case ' ':
            flags |= SPACE;
            goto repeat;
        case '#':
            flags |= SPECIAL;
            goto repeat;
        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        // Get field width
        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi__(&fmt);
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // Get precision
        precision = -1;
        if (*fmt == '.')
        {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi__(&fmt);
            else if (*fmt == '*')
            {
                ++fmt;
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        // Get the conversion qualifier
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            fmt++;
        }

        // Default base
        base = 10;
        switch (*fmt)
        {
        case 'c':
            if (!(flags & LEFT))
            {
                while (--field_width > 0)
                {
                    tmpch = ' ';
                    putc(&tmpch, 1);
                }
            }
            tmpch = (unsigned char)va_arg(args, int);
            putc(&tmpch, 1);

            while (--field_width > 0)
            {
                tmpch = ' ';
                putc(&tmpch, 1);
            }
            continue;
        case 's':
            s = va_arg(args, char *);
            if (!s)
                s = "<NULL>";
            len = strnlen__(s, precision);
            if (!(flags & LEFT))
            {
                while (len < field_width--)
                {
                    tmpch = ' ';
                    putc(&tmpch, 1);
                }
            }
            putc(s, len);
            while (len < field_width--)
            {
                tmpch = ' ';
                putc(&tmpch, 1);
            }
            continue;
        case 'p':
            if (field_width == -1)
            {
                field_width = 2 * sizeof(void *);
                flags |= ZEROPAD;
            }
            number__(putc, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
            continue;
        case 'o':
            base = 8;
            break;
        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;
        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;
        default:
            if (*fmt != '%')
            {
                tmpch = '%';
                putc(&tmpch, 1);
            }
            if (*fmt)
            {
                tmpch = *fmt;
                putc(&tmpch, 1);
            }
            else
            {
                --fmt;
            }
            continue;
        }

        if (qualifier == 'l')
            num = va_arg(args, unsigned long);
        else if (qualifier == 'h')
        {
            if (flags & SIGN)
                num = va_arg(args, int);
            else
                num = va_arg(args, unsigned int);
        }
        else if (flags & SIGN)
            num = va_arg(args, int);
        else
            num = va_arg(args, unsigned int);

        number__(putc, num, base, field_width, precision, flags);
    }
    if (tmpstr_size)
    {
        putc(tmpstr, tmpstr_size);
        tmpstr = NULL;
        tmpstr_size = 0;
    }
}

/**
 * @brief Native UART function to send a buffer of data.
 * @param data The data buffer to send.
 * @param size The size of the data buffer.
 */
static void native_uart_putc(char *data, uint16_t size)
{
    HalUartSendBuf(UART0, (uint8_t *)data, size);
}
#endif

/**
 * @brief Print a formatted log message.
 * @param format The format string.
 * @param ... The variable arguments.
 */
void dbg_log_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format); // Initialize the va_list to process the variable arguments
#ifdef PLATFORM_CST_OSAL_RTOS
    VSPRINTF(native_uart_putc, format, args); // Call formatted print function
#else
    vprintf(format, args);
#endif
    va_end(args); // Clean up the va_list after use
}

/**
 * @brief Print a log message with level, function name, file, and line number.
 * @param function_name The name of the function calling the log.
 * @param format The format string.
 * @param ... The variable arguments.
 */
void dbg_log_printf_level(const char *function_name, const char *format, ...)
{
    // Skip logging if log level is set to NONE or format is NULL
    if (current_log_level == LOG_LEVEL_NONE || !format)
    {
        return;
    }

    // Map log level to string representation
    const char *level_str;
    switch (current_log_level)
    {
    case LOG_LEVEL_DEBUG:
        level_str = "DEBUG";
        break;
    case LOG_LEVEL_INFO:
        level_str = "INFO";
        break;
    case LOG_LEVEL_WARNING:
        level_str = "WARNING";
        break;
    case LOG_LEVEL_ERROR:
        level_str = "ERROR";
        break;
    default:
        level_str = "";
        break;
    }

// Print log header with timestamp, level, function name
#ifdef PLATFORM_CST_OSAL_RTOS
    dbg_log_printf("[%s][%28s] ", level_str, function_name);
#else
    printf("[%s][%28s] ", level_str, function_name);
#endif

    if (format == NULL || format[0] == '\0')
    {
        return;
    }
    
    va_list args;
    va_start(args, format);
// Print formatted log message
#ifdef PLATFORM_CST_OSAL_RTOS
    VSPRINTF(native_uart_putc, format, args);
#else
    vprintf(format, args);
#endif
    va_end(args);
}

/**
 * @brief Print a buffer of bytes in hexadecimal format.
 * @param buff The buffer to print.
 * @param length The length of the buffer.
 */
void dbg_log_printf_buffer(uint8_t *buff, uint16_t length)
{
    for (int i = 0; i < length; i++)
    {
#ifdef PLATFORM_CST_OSAL_RTOS
        dbg_log_printf("%02x ", buff[i]);
#else
        printf("%02x ", buff[i]);
#endif
    }
#ifdef PLATFORM_CST_OSAL_RTOS
    dbg_log_printf("\r\n");
#else
    printf("\r\n");
#endif
}

/**
 * @brief Print a buffer of bytes in hexadecimal format with log level and function name.
 * @param function_name The name of the function calling the log.
 * @param buff The buffer to print.
 * @param length The length of the buffer.
 */
void dbg_log_printf_buffer_level(const char *function_name, const uint8_t *buff, uint16_t length)
{
    if (buff == NULL || length == 0)
    {
        /// dbg_log_printf_level(function_name, "Invalid buffer or length");
        return;
    }
    dbg_log_printf_level(function_name, "");
    for (int i = 0; i < length; i++)
    {
#ifdef PLATFORM_CST_OSAL_RTOS
        dbg_log_printf("%02x ", buff[i]);
#else
        printf("%02x ", buff[i]);
#endif
    }
#ifdef PLATFORM_CST_OSAL_RTOS
    dbg_log_printf("\r\n");
#else
    printf("\r\n");
#endif
}

/**
 * @brief Set the current log level.
 * @param level The log level to set (DEBUG, INFO, WARNING, ERROR, or NONE).
 */
void dbg_log_set_level(DBG_LOG_LEVEL level)
{
    current_log_level = level; // Update the log level
}
