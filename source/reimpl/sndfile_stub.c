/*
 * OpenSLES' Vita backend includes URI-player support through libsndfile.
 * Soda Dungeon feeds the backend with Android simple-buffer queues, so no
 * URI decoding is required.  The SDK's libsndfile package is hard-float and
 * must not be linked into this softfp loader; retain the symbols needed by
 * OpenSLES and fail URI-player requests cleanly instead.
 */

#include <stdint.h>

typedef struct SNDFILE_tag SNDFILE;
typedef struct SF_INFO SF_INFO;
typedef int64_t sf_count_t;

SNDFILE *sf_open(const char *path, int mode, SF_INFO *info)
{
    (void)path;
    (void)mode;
    (void)info;
    return 0;
}

int sf_close(SNDFILE *sndfile)
{
    (void)sndfile;
    return 0;
}

sf_count_t sf_read_short(SNDFILE *sndfile, short *ptr, sf_count_t items)
{
    (void)sndfile;
    (void)ptr;
    (void)items;
    return 0;
}

sf_count_t sf_seek(SNDFILE *sndfile, sf_count_t frames, int whence)
{
    (void)sndfile;
    (void)frames;
    (void)whence;
    return -1;
}
