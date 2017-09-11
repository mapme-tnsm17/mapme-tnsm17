#ifndef UTIL_HPP
#define UTIL_HPP

#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

/* Units */
#define S(x)                (x)
#define MS(x)               (x * 0.001)
#define S_TO_MS(x)          (x * 1000)
#define BYTES(x)            (x * 8)
#define BITS_TO_BYTES(x)    (x / 8)
#define BITS_S(x)           (x)
#define KBITS_S(x)          (x * 1000)
#define MBITS_S(x)          (x * 1000000)

int 
_mkdir(const char *dir, mode_t mode);

namespace ns3 {
namespace ndn {

void
TrackProgress(uint32_t duration, std::string filename, bool quiet);

void
TrackTermination(std::string filename);

} // namespace ndn
} // namespace ns3

#endif // UTIL_HPP
