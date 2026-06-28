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
#if SUCTION_HW_VERIFIED != 0
#error "Host-SIL logical wall profile must not fake SUCTION_HW_VERIFIED"
#endif
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 1
#define APP_WALL_STATE_CAPABLE 1
#else
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 0
#define APP_WALL_STATE_CAPABLE 0
#endif
#else
#define HOST_SIL_LOGICAL_SUCTION_AVAILABLE 0
#define APP_WALL_STATE_CAPABLE SUCTION_HW_VERIFIED
#if APP_WALL_STATE_CAPABLE != SUCTION_HW_VERIFIED
#error "C251 APP_WALL_STATE_CAPABLE must equal SUCTION_HW_VERIFIED"
#endif
#if APP_WALL_STATE_CAPABLE && (SUCTION_HW_VERIFIED == 0)
#error "C251 cannot enable wall states without verified suction hardware"
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
