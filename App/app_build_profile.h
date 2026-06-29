#ifndef APP_BUILD_PROFILE_H
#define APP_BUILD_PROFILE_H

#include "app_config.h"

#if defined(APP_WALL_STATE_CAPABLE) || defined(HOST_SIL_LOGICAL_SUCTION_AVAILABLE)
#error "Do not override APP_WALL_STATE_CAPABLE or HOST_SIL_LOGICAL_SUCTION_AVAILABLE outside app_build_profile.h"
#endif

#if defined(HOST_SIL_GUARD_PROFILE) && defined(HOST_SIL_LOGICAL_WALL_PROFILE)
#error "Select only one Host-SIL profile"
#endif

#if defined(HOST_SIL_LOGICAL_WALL_PROFILE) && !defined(HOST_SIL)
#error "HOST_SIL_LOGICAL_WALL_PROFILE is allowed only in Host-SIL builds"
#endif

#if defined(HOST_SIL_GUARD_PROFILE) && !defined(HOST_SIL)
#error "HOST_SIL_GUARD_PROFILE is allowed only in Host-SIL builds"
#endif

#if defined(HOST_SIL)
#if !defined(HOST_SIL_GUARD_PROFILE) && !defined(HOST_SIL_LOGICAL_WALL_PROFILE)
#error "Host-SIL builds must select HOST_SIL_GUARD_PROFILE or HOST_SIL_LOGICAL_WALL_PROFILE explicitly"
#endif
#if defined(HOST_SIL_LOGICAL_WALL_PROFILE)
#if FAN_ESC_PHYSICAL_OUTPUT_ENABLE != 0
#error "Host-SIL logical wall profile must not enable physical fan output"
#endif
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 1
#define APP_WALL_STATE_CAPABLE 1
#else
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 0
#define APP_WALL_STATE_CAPABLE 0
#endif
#else
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 0
#define APP_WALL_STATE_CAPABLE WALL_RUN_ENABLE
#if APP_WALL_STATE_CAPABLE && (FAN_ESC_PHYSICAL_OUTPUT_ENABLE == 0)
#error "C251 cannot enable wall states while FAN_ESC_PHYSICAL_OUTPUT_ENABLE is 0"
#endif
#endif

#if HOST_SIL_LOGICAL_SUCTION_AVAILABLE && !defined(HOST_SIL_LOGICAL_WALL_PROFILE)
#error "Logical suction can only be enabled by the explicit Host-SIL logical wall profile"
#endif

#define APP_LOGICAL_SUCTION_PRECHARGE_REQUEST 1100u
#define APP_LOGICAL_SUCTION_HOLD_REQUEST 1200u
#define APP_LOGICAL_SUCTION_BOOST_REQUEST 1300u
#define APP_LOGICAL_SUCTION_EMERGENCY_REQUEST 1200u

#endif
