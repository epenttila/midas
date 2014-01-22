#include "binary_io.h"

std::unique_ptr<FILE, int (*)(FILE*)> binary_open(const std::string& filename, const std::string& mode)
{
    return std::unique_ptr<FILE, int (*)(FILE*)>(std::fopen(filename.c_str(), mode.c_str()), std::fclose);
}
