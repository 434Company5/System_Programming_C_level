////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloudsim.c
//  Description    : This is the main program for the CMPSC311 programming
//                   assignment #2 (beginning of LionCLoud interface).
//
//   Author        : Patrick McDaniel
//   Last Modified : Fri 10 Jan 2020 01:34:33 PM EST
//

// Include Files
#include <cmpsc311_assocarr.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <cmpsc311_workload.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Project Includes
#include <lcloud_controller.h>
#include <lcloud_filesys.h>
#include <lcloud_support.h>

// Defines
#define LCLOUD_ARGUMENTS "hvl:x:"
#define USAGE                                                       \
    "USAGE: lcloud_sim [-h] [-v] [-l <logfile>] <workload-file>\n"  \
    "\n"                                                            \
    "where:\n"                                                      \
    "    -h - help mode (display this message)\n"                   \
    "    -v - verbose output\n"                                     \
    "    -l - write log messages to the filename <logfile>\n"       \
    "\n"                                                            \
    "    <workload-file> - file contain the workload to simulate\n" \
    "\n"

//
// Global Data
int verbose;

//
// Functional Prototypes

int simulateLionCloud(char* wload); // LionCloud simulation

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the LCLOUD simulator
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main(int argc, char* argv[])
{

    // Local variables
    int ch, verbose = 0, log_initialized = 0;

    // Process the command line parameters
    while ((ch = getopt(argc, argv, LCLOUD_ARGUMENTS)) != -1) {

        switch (ch) {
        case 'h': // Help, print usage
            fprintf(stderr, USAGE);
            return (-1);

        case 'v': // Verbose Flag
            verbose = 1;
            break;

        case 'l': // Set the log filename
            initializeLogWithFilename(optarg);
            log_initialized = 1;
            break;

        default: // Default (unknown)
            fprintf(stderr, "Unknown command line option (%c), aborting.\n", ch);
            return (-1);
        }
    }

    // Setup the log as needed
    if (!log_initialized) {
        initializeLogWithFilehandle(CMPSC311_LOG_STDERR);
    }
    LcControllerLLevel = registerLogLevel("LCLOUD_CONTROLLER", 0); // Controller log level
    LcDriverLLevel = registerLogLevel("LCLOUD_DRIVER", 0); // Driver log level
    LcSimulatorLLevel = registerLogLevel("LCLOUD_SIMULATOR", 0); // Driver log level
    if (verbose) {
        enableLogLevels(LOG_INFO_LEVEL);
        enableLogLevels(LcControllerLLevel | LcDriverLLevel | LcSimulatorLLevel);
    }

    // The filename should be the next option
    if (argv[optind] == NULL) {
        fprintf(stderr, "Missing command line parameters, use -h to see usage, aborting.\n");
        return (-1);
    }

    // Run the simulation
    if (simulateLionCloud(argv[optind]) == 0) {
        logMessage(LOG_INFO_LEVEL, "LionCloud simulation completed successfully!!!\n\n");
    } else {
        logMessage(LOG_INFO_LEVEL, "LionCloud simulation failed.\n\n");
    }

    // Do some cleanup
    freeLogRegistrations();

    // Return successfully
    return (0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : simulateLionCloud
// Description  : The main control loop for the processing of the LionCloud
//                simulation (which calls the student code).
//
// Inputs       : wload - the name of the workload file
// Outputs      : 0 if successful test, -1 if failure

int simulateLionCloud(char* wload)
{

    /* Local types */
    typedef struct {
        char* filename;
        LcFHandle fhandle;
        int pos;
    } fsysdata;

    /* Local variables */
    workload_state state;
    workload_operation operation;
    LcFHandle fh;
    AssocArray fhTable;
    char buf[LC_MAX_OPERATION_SIZE];
    int opens, reads, writes, seeks, closes;
    fsysdata* fdata;

    /* Init fh table, open the workload for processing */
    init_assoc(&fhTable, stringCompareCallback, pointerCompareCallback);
    if (openCmpsc311Workload(&state, wload)) {
        logMessage(LOG_ERROR_LEVEL, "CMPSC311 lcloud workload: failed opening workload [%s]", wload);
        return (-1);
    }

    /* Loop until we are done with the workload */
    logMessage(LcSimulatorLLevel, "CMPSC311 lcloud : executing workload [%s]", state.filename);
    do {

        /* Get the next operation to process */
        if (readCmpsc311Workload(&state, &operation)) {
            logMessage(LOG_ERROR_LEVEL, "CMPSC311 workload unit test failed at line %d, get op", state.lineno);
            return (-1);
        }

        /* Verbose log the operation */
        if ((operation.op == WL_READ) || (operation.op == WL_WRITE)) {
            logMessage(LcSimulatorLLevel, "CMPSCS311 workload op: %s %s off=%d, sz=%d [%.20s]", operation.objname,
                workload_operations_strings[operation.op], operation.pos, operation.size, operation.data);
        } else {
            logMessage(LcSimulatorLLevel, "CMPSCS311 workload op: %s %s", operation.objname,
                workload_operations_strings[operation.op]);
        }

        /* Switch on the operation type */
        switch (operation.op) {

        case WL_OPEN: /* Open the file for reading/writing, check error */

            /* Open the file for reading */
            if ((fh = lcopen(operation.objname)) == -1) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error opening file [%s], aborting", operation.objname);
                return (-1);
            }

            /* Setup the structure */
            fdata = malloc(sizeof(fsysdata));
            fdata->filename = strdup(operation.objname);
            fdata->fhandle = fh;
            fdata->pos = 0;

            /* Insert the file into the table */
            insert_assoc(&fhTable, fdata->filename, fdata);
            logMessage(LcSimulatorLLevel, "Open file [%s]", fdata->filename);
            opens++;
            break;

        case WL_READ: /* Read a block of data from the file */

            /* Find the file for processing */
            if ((fdata = find_assoc(&fhTable, operation.objname)) == NULL) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error reading unknown file [%s], aborting",
                    operation.objname);
                return (-1);
            }

            /* If the position within the file is not a read location, seek */
            if (fdata->pos != operation.pos) {
                if (lcseek(fdata->fhandle, operation.pos) != operation.pos) {
                    logMessage(LOG_ERROR_LEVEL, "CMPSC311 error seek failed [%s, pos=%d], aborting",
                        operation.objname, operation.pos);
                    return (-1);
                }
                fdata->pos = operation.pos;
                seeks++;
            }

            /* Now do the read from the file */
            if (lcread(fdata->fhandle, buf, operation.size) != operation.size) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error read failed [%s, pos=%d, size=%d], aborting",
                    operation.objname, operation.pos, operation.size);
                return (-1);
            }

            /* Compare the data read with that in the workload data */
            if (strncmp(buf, operation.data, operation.size) != 0) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 read data compare failed, aborting");
                logMessage(LOG_ERROR_LEVEL, "Read data     : [%s]", buf);
                logMessage(LOG_ERROR_LEVEL, "Expected data : [%s]", operation.data);
                return (-1);
            }

            /* Now increment the file position, log the data */
            fdata->pos += operation.size;
            logMessage(LcControllerLLevel, "Correctly read from [%s], %d bytes at position %d",
                fdata->filename, operation.size, operation.pos);
            reads++;
            break;

        case WL_WRITE: /* Write a block of data to the file */

            /* Find the file for processing */
            if ((fdata = find_assoc(&fhTable, operation.objname)) == NULL) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error writing unknown file [%s], aborting",
                    operation.objname);
                return (-1);
            }

            /* If the position within the file is not a read location, seek */
            if (fdata->pos != operation.pos) {
                if (lcseek(fdata->fhandle, operation.pos) != operation.pos) {
                    logMessage(LOG_ERROR_LEVEL, "CMPSC311 error seek failed [%s, pos=%d], aborting",
                        operation.objname, operation.pos);
                    return (-1);
                }
                fdata->pos = operation.pos;
                seeks++;
            }

            /* Now do the write to the file */
            if (lcwrite(fdata->fhandle, operation.data, operation.size) != operation.size) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error write failed [%s, pos=%d, size=%d], aborting",
                    operation.objname, operation.pos, operation.size);
                return (-1);
            }

            /* Now increment the file position, log the data */
            fdata->pos += operation.size;
            logMessage(LcControllerLLevel, "Wrote data to file [%s], %d bytes at position %d",
                fdata->filename, operation.size, operation.pos);
            writes++;
            break;

        case WL_CLOSE:

            /* Find the file for processing */
            if ((fdata = find_assoc(&fhTable, operation.objname)) == NULL) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error closing unknown file [%s], aborting",
                    operation.objname);
                return (-1);
            }

            /* Now close the file */
            if (lcclose(fdata->fhandle) != 0) {
                logMessage(LOG_ERROR_LEVEL, "CMPSC311 error write failed [%s, pos=%d, size=%d], aborting",
                    operation.objname, operation.pos, operation.size);
                return (-1);
            }

            /* Remove file from file handle table, clean up structures, log */
            logMessage(LcSimulatorLLevel, "Closed file [%s].", fdata->filename);
            delete_assoc(&fhTable, fdata->filename);
            free(fdata->filename);
            free(fdata);
            closes++;
            break;

        case WL_EOF: // End of the workload file
            lcshutdown();
            logMessage(LcSimulatorLLevel, "End of the workload file (processed)");
            break;

        default: /* Unknown oepration type, bailout */
            logMessage(LOG_ERROR_LEVEL, "CMPSC311 lion clound bad operation type [%d]", operation.op);
            return (-1);
        }

        /* Sanity check the operation state */
        if (operation.op > WL_EOF) {
            logMessage(LOG_ERROR_LEVEL, "CMPSC311 lion clound bad POST HOC op code [%d]", operation.op);
            return (-1);
        }

    } while (operation.op < WL_EOF);

    /* Log, close workload and delete the local file, return successfully  */
    closeCmpsc311Workload(&state);
    return (0);
}
