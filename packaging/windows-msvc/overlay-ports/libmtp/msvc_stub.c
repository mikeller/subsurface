#include <stdlib.h>
#include "libmtp.h"

/* MSVC-compatible stub implementation of the libmtp symbols used by
 * Subsurface / libdivecomputer on Windows.  All functions are no-ops that
 * signal "no device attached" so the code compiles and links cleanly;
 * actual MTP access is handled at a higher level (e.g. via WinUSB /
 * Windows Portable Devices) on this platform.
 */

void LIBMTP_Init(void) {}

LIBMTP_error_number_t LIBMTP_Detect_Raw_Devices(LIBMTP_raw_device_t **devices, int *numdevs)
{
    if (devices) *devices = NULL;
    if (numdevs) *numdevs = 0;
    return LIBMTP_ERROR_NO_DEVICE_ATTACHED;
}

LIBMTP_mtpdevice_t *LIBMTP_Open_Raw_Device_Uncached(LIBMTP_raw_device_t *device)
{
    (void)device;
    return NULL;
}

LIBMTP_mtpdevice_t *LIBMTP_Get_First_Device(void)
{
    return NULL;
}

void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device)
{
    (void)device;
}

int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device, uint16_t **filetypes, uint16_t *len)
{
    (void)device;
    if (filetypes) *filetypes = NULL;
    if (len) *len = 0;
    return 0;
}

LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device)
{
    (void)device;
    return NULL;
}

LIBMTP_file_t *LIBMTP_Get_Files_And_Folders(LIBMTP_mtpdevice_t *device,
                                              uint32_t storage_id,
                                              uint32_t parent_id)
{
    (void)device;
    (void)storage_id;
    (void)parent_id;
    return NULL;
}

void LIBMTP_destroy_file_t(LIBMTP_file_t *file)
{
    (void)file;
}

int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device, uint32_t id,
                                MTPDataPutFunc put_func, void *priv,
                                LIBMTP_progressfunc_t cb, const void *data)
{
    (void)device; (void)id; (void)put_func; (void)priv; (void)cb; (void)data;
    return -1;
}

int LIBMTP_Get_File_To_File(LIBMTP_mtpdevice_t *device, uint32_t id,
                             const char *path,
                             LIBMTP_progressfunc_t cb, const void *data)
{
    (void)device; (void)id; (void)path; (void)cb; (void)data;
    return -1;
}

void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *device)
{
    (void)device;
}

int LIBMTP_Update_File_Metadata(LIBMTP_mtpdevice_t *device, LIBMTP_file_t const *file)
{
    (void)device; (void)file;
    return -1;
}
