/* Definitions and declarations required to compile autopilot code on a
   i386 architecture. Bindings for OCaml. */

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "std.h"
#include "main_ap.h"
#include "autopilot.h"
#include "modules/gps/gps.h"
#include "generated/settings.h"
#include "firmwares/fixedwing/nav.h"
#include "firmwares/fixedwing/stabilization/stabilization_attitude.h"
#include "firmwares/fixedwing/guidance/guidance_v.h"
#include "modules/core/commands.h"
#include "modules/datalink/datalink.h"
#include "modules/datalink/telemetry.h"
#include "generated/flight_plan.h"

#include "generated/modules.h"

#include <caml/mlvalues.h>
#include <caml/memory.h>


/* Dummy definitions to replace the ones from the files not compiled in the
   simulator */
uint8_t ir_estim_mode;
uint8_t vertical_mode;
uint8_t inflight_calib_mode;
bool rc_event_1, rc_event_2;
uint8_t gps_nb_ovrn, link_fbw_fbw_nb_err, link_fbw_nb_err;
float alt_roll_pgain;
float roll_rate_pgain;

#ifndef SIM_UPDATE_DL
#define SIM_UPDATE_DL TRUE
#endif

uint8_t ac_id;

#if PERIODIC_FREQUENCY != 60
#warning "Simple OCaml sim can currently only handle a PERIODIC_FREQUENCY of 60Hz"
#endif

#if SYS_TIME_FREQUENCY != 120
#warning "Simple OCaml sim can currently only handle a SYS_TIME_FREQUENCY of 120Hz"
#endif

/** needs to be called at SYS_TIME_FREQUENCY */
value sim_sys_time_task(value unit)
{
  sys_tick_handler();
  return unit;
}

value sim_periodic_task(value unit)
{
  main_ap_periodic();
  main_ap_event();
  return unit;
}

float ftimeofday(void)
{
  struct timeval t;
  struct timezone z;
  gettimeofday(&t, &z);
  return (t.tv_sec + t.tv_usec / 1e6);
}

value sim_init(value unit)
{
  modules_mcu_init();
  main_ap_init();

  return unit;
}

value update_bat(value bat)
{
  electrical.vsupply = (float)Int_val(bat) / 10.;
  return Val_unit;
}

value update_dl_status(value dl_enabled)
{
  ivy_tp.ivy_dl_enabled = Int_val(dl_enabled);
  return Val_unit;
}


value get_commands(value val_commands)
{
  int i;

  for (i = 0; i < COMMANDS_NB; i++) {
    Store_field(val_commands, i, Val_int(commands[i]));
  }

  return Val_int(commands[COMMAND_THROTTLE]);
}

value set_datalink_message(value s)
{
  int n = string_length(s);
  char *ss = (char *)String_val(s);
  assert(n <= MSG_SIZE);

  int i;
  for (i = 0; i < n; i++) {
    dl_buffer[i] = ss[i];
  }

  dl_msg_available = true;
  DlCheckAndParse(&(DOWNLINK_DEVICE).device, &ivy_tp.trans_tx, dl_buffer, &dl_msg_available, SIM_UPDATE_DL);

  return Val_unit;
}

