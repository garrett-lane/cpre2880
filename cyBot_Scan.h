#ifndef CYBOT_SCAN_H_
#define CYBOT_SCAN_H_

// Author: Phillip Jones
// Data: Updated 10/27/2021
// Version: 10 27 2021  (you can use cyBOT_scan_version() to confirm version)


// Scan and object still rely on this struct even though we're no long use cyBot_Scan.lib, so we keep using the header file
// Scan value
typedef struct{
    float sound_dist;  // Distance from Ping Sensor (cyBOT_Scan returns -1.0 if PING is not enabled)
    int IR_raw_val;    // Raw ADC value from IR sensor (cyBOT_Scan return -1 if IR is not enabled)
} cyBOT_Scan_t;

#endif /* CYBOT_SCAN_H_ */
