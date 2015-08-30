/*
 * parse_cmdline.h
 *
 * 11/09/14 DJG Added new command line options
 * 10/24/14 DJG Changed needed to support mfm_emu write buffer

 *  Created on: Dec 21, 2013
 *      Author: djg
 */

#ifndef PARSE_CMDLINE_H_
#define PARSE_CMDLINE_H_

#define MAX_HEAD 16

#define MAX_DRIVES 2
// This is the main structure defining the drive characteristics
typedef struct {
   // The number of cylinders and heads
   int num_cyl;
   int num_head;
   char *filename[MAX_DRIVES];
   int fd[MAX_DRIVES];
   int drive[MAX_DRIVES];  // Drive select number
   EMU_FILE_INFO emu_file_info[MAX_DRIVES];
   int num_drives;
   int initialize;
   int buffer_count;		// Number of track buffers
   float buffer_max_time;	// Max time to delay when last buffer used
   float buffer_time;		// time parameter for delay calculation
   char *cmdline;               // Decode parameters from command line
   char *options;               // Extra options specified for saving in file
   char *note;                  // File information string
   uint32_t sample_rate_hz;
   uint32_t start_time_ns;
} DRIVE_PARAMS;
char *parse_print_cmdline(DRIVE_PARAMS *drive_params, int print);
void parse_cmdline(int argc, char *argv[], DRIVE_PARAMS *drive_params);
#endif /* PARSE_CMDLINE_H_ */
