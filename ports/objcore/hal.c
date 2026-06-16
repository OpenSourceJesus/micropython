#include <stdio.h>
#include <unistd.h>

#include "py/mpconfig.h"
#include "py/mphal.h"

int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    (void)read(STDIN_FILENO, &c, 1);
    return c;
}

mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    ssize_t n = write(STDOUT_FILENO, str, len);
    return (n < 0) ? 0 : (mp_uint_t)n;
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    mp_hal_stdout_tx_strn(str, len);
}
