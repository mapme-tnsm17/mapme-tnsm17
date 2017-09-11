#include "util.hpp"
#include <cstdlib>
#include <iostream>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <fstream>

using namespace std;

#include "ns3/simulator.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"

int _mkdir(const char *dir, mode_t mode) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, mode);
            *p = '/';
        }
    return mkdir(tmp, mode);
}

namespace ns3 {
namespace ndn {

void
TrackProgress(uint32_t duration, std::string filename, bool quiet)
{
  std::cout << "track progress" << std::endl;
  // special value 0 for duration marks the end of the simulation
  static clock_t begin = clock();
  double elapsed;
  double now = Simulator::Now().ToDouble(Time::S);
  double sleep;

  elapsed = double(clock() - begin) / CLOCKS_PER_SEC;
  int remaining = (duration == 0) ? 0 : int(elapsed * (duration - now) / now);
  if (!quiet && duration != 0) {
    std::cerr << "Time: " << now << " / " << duration << " s (ETA: " << remaining << " s)" << "\r";
  }

  if (!filename.empty()) {
    ofstream statusfile;
    int percentage = (duration == 0) ? 100 : int(now * 100 / duration);

    statusfile.open(filename);
    statusfile << now << " " << duration << " " << elapsed << " " << percentage << " " << remaining << std::endl;
    statusfile.close();
  }

  if (duration == 0)
    return;

  sleep = (duration > 100) ? duration / 100 : 1;
  Simulator::Schedule(Seconds(sleep), &TrackProgress, duration, filename, quiet);
}

} // namespace ndn
} // namespace ns3
