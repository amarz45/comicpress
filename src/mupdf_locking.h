#pragma once

#include <QMutex>
#include <mupdf/fitz.h>

static QMutex mupdf_mutexes[FZ_LOCK_MAX];

static void lock_mupdf(void* user, int lock) {
    if (lock >= 0 && lock < FZ_LOCK_MAX) {
        mupdf_mutexes[lock].lock();
    }
}

static void unlock_mupdf(void* user, int lock) {
    if (lock >= 0 && lock < FZ_LOCK_MAX) {
        mupdf_mutexes[lock].unlock();
    }
}

static fz_locks_context mupdf_locks = {
    .user = nullptr,
    .lock = lock_mupdf,
    .unlock = unlock_mupdf,
};
