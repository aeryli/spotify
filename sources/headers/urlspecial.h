#ifndef QUERY_ENCODER_H
#define QUERY_ENCODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Encodes a source string into a URL-safe query string format.
 * Non-alphanumeric characters (including spaces) are converted to %XX hex values.
 *
 * @param src     The raw string to be encoded.
 * @param dest    The output buffer where the encoded string will be stored.
 * @param dest_sz The total allocation size of the destination buffer.
 * @return        0 on success, or -1 if the destination buffer is too small.
 */
static inline int url_encode_string(const char *src, char *dest, size_t dest_sz) {
    if (!src || !dest || dest_sz == 0) return -1;
    
    size_t written = 0;
    while (*src) {
        // Safe characters according to RFC 3986 section 2.3
        if (isalnum((unsigned char)*src) || *src == '-' || *src == '_' || *src == '.' || *src == '~') {
            if (written + 1 >= dest_sz) return -1;
            dest[written++] = *src;
        } else {
            // Hex escape sequence (%XX) requires 3 bytes.
            // Spaces fall through here and automatically convert to "%20"
            if (written + 3 >= dest_sz) return -1;
            snprintf(&dest[written], 4, "%%%02X", (unsigned char)*src);
            written += 3;
        }
        src++;
    }
    
    dest[written] = '\0';
    return 0;
}

#endif // QUERY_ENCODER_H
