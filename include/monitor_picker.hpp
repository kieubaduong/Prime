#pragma once

#include <Windows.h>

namespace monitor_picker {

struct MonitorSelection {
  UINT adapterIndex;
  UINT monitorIndex;
};

MonitorSelection choose();

}  // namespace monitor_picker
