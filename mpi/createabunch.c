/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (c) 2012, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Christopher J. Morrone <morrone2@llnl.gov>
 * LLNL-CODE-468512
 *
 * This file is part of lustre-tools-llnl.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (as published by the
 * Free Software Foundation) version 2, dated June 1991.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the IMPLIED WARRANTY OF
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * terms and conditions of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "mpi.h"

int mpi_rank;
int mpi_size;
struct {
        int file_count;
        bool file_count_set;
        int time_limit; /* in seconds */
        bool time_limit_set;
        char *test_directory_name;
        bool dump_all_tasks_data;
} args;

struct count_log_data {
        unsigned max_entries;
        unsigned entries;
        unsigned *data;
};


void enlarge_log_specific(struct count_log_data *log, unsigned num_entries)
{
        unsigned old_size;
        unsigned new_size;

        if (num_entries <= log->max_entries)
                return;

        old_size = log->max_entries * sizeof(unsigned);
        log->max_entries = num_entries;
        new_size = log->max_entries * sizeof(unsigned);
        log->data = realloc(log->data, (size_t)new_size);
        if (log->data == NULL) {
                fprintf(stderr, "relloc(ptr, %u) failed in enlarge_log: %d\n",
                        new_size, errno);
                MPI_Abort(MPI_COMM_WORLD, 4);
        }
        memset((void *)log->data + old_size, 0, new_size - old_size);
}

void enlarge_log(struct count_log_data *log)
{
        enlarge_log_specific(log, log->max_entries + 900);
}

void usage()
{
        int i;

        if (mpi_rank == 0) {
                printf("Usage: createabunch [-c numberoffiles] [-t timelimit] <directory>\n");
        }

        MPI_Initialized(&i);
        if (i) MPI_Finalize();
        exit(0);
}

unsigned timeindex(time_t start)
{
        return (unsigned)(time(NULL) - start);
}

#define LOG_LEN 3600

void record_entry(struct count_log_data *log, unsigned index, unsigned entry)
{
        while (index >= log->max_entries) 
                enlarge_log(log);
        log->data[index] += entry;
        log->entries = index + 1;
}

void createabunch(struct count_log_data *log)
{
        char filename[1024];
        unsigned int count_recent = 0;
        unsigned int count_total = 0;
        unsigned int count_limit;
        unsigned int count_total_aggregate;
        time_t start;
        unsigned int current_timeindex = 0;
        unsigned int elapsed;
        int fd;

        if (args.file_count_set) {
                count_limit = args.file_count / mpi_size;
                if (mpi_rank < (args.file_count % mpi_size))
                        count_limit += 1;
        }

        MPI_Barrier(MPI_COMM_WORLD);
        start = time(NULL);
        while (1) {
                /* create the file */
                snprintf(filename, 1024, "%s/many-%d-%d",
                         args.test_directory_name, mpi_rank, count_total);
                fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL,
                          S_IRUSR|S_IWUSR);
                close(fd);
                count_recent++;
                count_total++;

                /* log creation count at most once per second */
                current_timeindex = timeindex(start);
                if (current_timeindex >= log->entries) {
                        record_entry(log, current_timeindex, count_recent);
                        count_recent = 0;
                }

                /* end conditions */
                if (args.time_limit_set && current_timeindex > args.time_limit)
                        break;
                if (args.file_count_set && count_total >= count_limit) {
                        /* tally left over counts */
                        record_entry(log, current_timeindex, count_recent);
                        break;
                }
        };

        MPI_Barrier(MPI_COMM_WORLD);
        elapsed = timeindex(start);
        MPI_Reduce(&count_total, &count_total_aggregate,
                   1, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
        if (mpi_rank == 0) {
                printf ("Created %d total files in %d secs (%d per sec)\n",
                        count_total_aggregate, elapsed,
                        count_total_aggregate/elapsed);
        }

}

/* print one line per second, with one total value for all creates made
 * in that second (the aggregate) */
void dump_aggregate_log_data(struct count_log_data *log)
{
        unsigned int max_entries;
        unsigned int *log_data_aggregate;

        MPI_Allreduce(&log->entries, &max_entries,
                      1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
        enlarge_log_specific(log, max_entries);
        if (mpi_rank == 0) {
                log_data_aggregate = (unsigned *)malloc(max_entries
                                                        * sizeof(unsigned));
                if (log_data_aggregate == NULL) {
                        fprintf(stderr, "dump_log_data, malloc failed: %d\n",
                                errno);
                        MPI_Abort(MPI_COMM_WORLD, 4);
                }
        }

        MPI_Reduce(log->data, log_data_aggregate,
                   max_entries, MPI_UNSIGNED, MPI_SUM, 0, MPI_COMM_WORLD);
        if (mpi_rank == 0) {
                FILE *f;
                int i;

                printf("Logging aggregate create data to \"createabunch.log\"\n");
                fflush(stdout);
                f = fopen("createabunch.log", "w");
                for (i = 0; i < max_entries; i++) {
                        fprintf(f, "%d %d\n", i, log_data_aggregate[i]);
                }
                fflush(f);
                fclose(f);
        }
}

/* print one line per second listing all tasks' create counts */
void dump_all_log_data(struct count_log_data *log)
{
        unsigned int max_entries;
        unsigned int *all_tasks_data = NULL;
        FILE *f;
        int i, j;

        MPI_Allreduce(&log->entries, &max_entries,
                      1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
        enlarge_log_specific(log, max_entries);

        if (mpi_rank == 0) {
                printf("Logging ALL create data to \"createabunch_all.log\"\n");
                fflush(stdout);
                f = fopen("createabunch_all.log", "w");
                all_tasks_data = malloc(sizeof(unsigned) * mpi_size);
                if (all_tasks_data == NULL) {
                        fprintf(stderr, "malloc mpi_size array failed: %d\n",
                                errno);
                        MPI_Abort(MPI_COMM_WORLD, 5);
                }
        }

        for (i = 0; i < max_entries; i++) {
                MPI_Gather(&log->data[i], 1, MPI_UNSIGNED,
                           all_tasks_data, 1, MPI_UNSIGNED,
                           0, MPI_COMM_WORLD);
                if (mpi_rank == 0) {
                        fprintf(f, "%d", i);
                        for (j = 0; j < mpi_size; j++)
                                fprintf(f, " %u", all_tasks_data[j]);
                        fprintf(f, "\n");
                }
        }

        if (mpi_rank == 0) {
                fflush(f);
                fclose(f);
        }
}

/*
 * Recursively create directory path
 * Returns 0 on success
 * NOTE: modifies path while in use
 */
int recursive_mkdir(char *dir)
{
        char *slash;
        int rc = 0;
        struct stat stat_buf;

        /* check if dir already exists */
        rc = stat(dir, &stat_buf);
        if (rc == 0) {
                if (S_ISDIR(stat_buf.st_mode)) {
                        return 0;
                } else {
                        fprintf(stderr,
                                "\"%s\" exists, but is not a directory\n", dir);
                        return 1;
                }
        }

        /* recursively check if parent directories exist */
        slash = strrchr(dir, '/');
        if (slash != NULL) {
                *slash = '\0';
                rc = recursive_mkdir(dir);
                *slash = '/';
                if (rc != 0)
                        return rc;
        }

        /* make directory */
        rc = mkdir(dir, 0777);
        if (rc != 0)
                fprintf(stderr, "mkdir(\"%s\", 0777) failed: %d\n", dir, errno);

        return rc;
}

void parse_command_line(int argc, char **argv)
{
        int c;

        /* Parse command line options */
        while (1) {
                c = getopt(argc, argv, "ac:t:");
                if (c == -1)
                        break;

                switch (c) {
                case 'a':
                        args.dump_all_tasks_data = 1;
                        break;
                case 'c':
                        args.file_count = atoi(optarg);
                        args.file_count_set = true;
                        break;
                case 't':
                        args.time_limit = atoi(optarg);
                        args.time_limit_set = true;
                        break;
                case 'h':
                default:
                        usage();
                        break;
                }
        }

        /* The is one required field at the end: the directory name */
        if (optind != (argc - 1)) {
                usage();
        }
        if (!args.file_count_set && !args.time_limit_set && mpi_rank == 0) {
                fprintf(stderr, "One of either \"-c\" or \"-t\" parameters must be specified.\n");
                MPI_Abort(MPI_COMM_WORLD, 2);
        }

        args.test_directory_name = malloc(strlen(argv[optind])+1);
        strcpy(args.test_directory_name, argv[optind]);
}

int main(int argc, char **argv)
{
        int i;
        struct count_log_data log = {0, 0, NULL};

        /* Check for -h parameter before MPI_Init */
        for (i = 1; i < argc; i++) {
                if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
                        usage();
                }
        }

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

        if (mpi_rank == 0) {
                printf("createabunch is running with %d process(es)\n", mpi_size);
                fflush(stdout);
        }


        parse_command_line(argc, argv);
        if (mpi_rank == 0 && recursive_mkdir(args.test_directory_name) != 0) {
                MPI_Abort(MPI_COMM_WORLD, 3);
        }
        enlarge_log(&log);
        createabunch(&log);
        dump_aggregate_log_data(&log);
        if (args.dump_all_tasks_data)
                dump_all_log_data(&log);

        MPI_Finalize();
        exit(0);
}
