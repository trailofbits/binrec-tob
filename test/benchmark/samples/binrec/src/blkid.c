#include <stdio.h>
#include <blkid/blkid.h>

// This sample exercises using a library dependency, libblkid, and calls
// several functions in the library.

int main(int argc, char **argv) {
    const char *ver = NULL;
    const char *date = NULL;

    int status = blkid_get_library_version(&ver, &date);

    printf("blkid_get_library_version: %d\n", status);
    printf(">> version: %s\n", ver);
    printf(">> date:    %s\n", date);

    blkid_cache cache;

    status = blkid_get_cache(&cache, NULL);
    if(status) {
        printf("failed to initialize blkid cache: %d\n", status);
        return 1;
    }

    printf("\nDevices:\n");

    status = blkid_probe_all(cache);
    if(status < 0) {
        printf("failed to probe devices: %d\n", status);
        blkid_gc_cache(cache);
    }

    blkid_dev device;
    blkid_dev_iterate iter;
    
    iter = blkid_dev_iterate_begin(cache);

    while(!blkid_dev_next(iter, &device)) {
        printf(">> %s\n", blkid_dev_devname(device));
    }

    blkid_dev_iterate_end(iter);

    blkid_gc_cache(cache);

    return 0;
}
