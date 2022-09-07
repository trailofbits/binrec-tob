#include <uuid/uuid.h>
#include <stdio.h>

// This sample exercises using a library dependency, libuuid, and calls
// several functions in the library.

int main() {
    uuid_t uuid;
    char uuid_str[37];

    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
    printf("uuid_generate:           %s\n", uuid_str);

    uuid_generate_random(uuid);
    uuid_unparse(uuid, uuid_str);
    printf("uuid_generate_random:    %s\n", uuid_str);

    uuid_generate_time(uuid);
    uuid_unparse(uuid, uuid_str);
    printf("uuid_generate_time:      %s\n", uuid_str);

    uuid_generate_time_safe(uuid);
    uuid_unparse(uuid, uuid_str);
    printf("uuid_generate_time_safe: %s\n", uuid_str);


    return 0;
}