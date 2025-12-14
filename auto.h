/*
 * auto.h
 *
 *  Created on: Dec 2, 2025
 *      Author: gthomp
 */

#ifndef AUTO_H_
#define AUTO_H_

#include "cyBot_Scan.h"
#include "open_interface.h"

// Autonomous navigation mode for CyBot
// Scans for objects, finds gaps, and navigates autonomously
int run_auto(oi_t *data, cyBOT_Scan_t *scan_data);

#endif /* AUTO_H_ */
