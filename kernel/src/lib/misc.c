#include <lib/misc.k.h>

__attribute__((used, section(".limine_requests")))
volatile struct limine_executable_file_request limine_executable_file_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST,
    .revision = 0
};
