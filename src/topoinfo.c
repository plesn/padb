/*
 *
 * Copyright (c) 2016      Bull SAS
 *
 * Copyrights from openmpi opal/mca/hwloc/base/hwloc_base_util.c :
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2013-2014 Intel, Inc. All rights reserved.
 * Copyright (c) 2015-2016 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <hwloc.h>



/* function taken from Openmpi */

#define OPAL_SUCCESS 0
#define OPAL_ERROR 1
#define OPAL_ERR_NOT_BOUND 2
/*
 * Make a prettyprint string for a cset in a map format.
 * Example: [B./..]
 * Key:  [] - signifies socket
 *        / - divider between cores
 *        . - signifies PU a process not bound to
 *        B - signifies PU a process is bound to
 */
int opal_hwloc_base_cset2mapstr(char *str, int len,
                                hwloc_topology_t topo,
                                hwloc_cpuset_t cpuset)
{
    char tmp[BUFSIZ];
    int core_index, pu_index;
    const int stmp = sizeof(tmp) - 1;
    hwloc_obj_t socket, core, pu;
    hwloc_obj_t root;
    /* opal_hwloc_topo_data_t *sum; */

    str[0] = tmp[stmp] = '\0';

    /* if the cpuset is all zero, then not bound */
    if (hwloc_bitmap_iszero(cpuset)) {
        return OPAL_ERR_NOT_BOUND;
    }

    /* if the cpuset includes all available cpus, then we are unbound */
    root = hwloc_get_root_obj(topo);
    /* if (NULL == root->userdata) { */
    /*     /\* opal_hwloc_base_filter_cpus(topo); *\/ */
    /*     printf("error: opal_hwloc_base_filter_cpus(topo)\n"); */
    /*     exit(1); */
    /* } else { */
    /*     sum = (opal_hwloc_topo_data_t*)root->userdata; */
    /*     if (NULL == sum->available) { */
    /*        return OPAL_ERROR; */
    /*     } */
    /*     if (0 != hwloc_bitmap_isincluded(sum->available, cpuset)) { */
    /*         return OPAL_ERR_NOT_BOUND; */
    /*     } */
    /* } */
    hwloc_cpuset_t avail = hwloc_bitmap_alloc();
    hwloc_bitmap_and(avail, root->online_cpuset, root->allowed_cpuset);
    if (0 != hwloc_bitmap_isincluded(avail, cpuset)) {
        return OPAL_ERR_NOT_BOUND;
    }

    /* Iterate over all existing sockets */
    for (socket = hwloc_get_obj_by_type(topo, HWLOC_OBJ_SOCKET, 0);
         NULL != socket;
         socket = socket->next_cousin) {
        strncat(str, "[", len - strlen(str));

        /* Iterate over all existing cores in this socket */
        core_index = 0;
        for (core = hwloc_get_obj_inside_cpuset_by_type(topo,
                                                        socket->cpuset,
                                                        HWLOC_OBJ_CORE, core_index);
             NULL != core;
             core = hwloc_get_obj_inside_cpuset_by_type(topo,
                                                        socket->cpuset,
                                                        HWLOC_OBJ_CORE, ++core_index)) {
            if (core_index > 0) {
                strncat(str, "/", len - strlen(str));
            }

            /* Iterate over all existing PUs in this core */
            pu_index = 0;
            for (pu = hwloc_get_obj_inside_cpuset_by_type(topo,
                                                          core->cpuset,
                                                          HWLOC_OBJ_PU, pu_index);
                 NULL != pu;
                 pu = hwloc_get_obj_inside_cpuset_by_type(topo,
                                                          core->cpuset,
                                                          HWLOC_OBJ_PU, ++pu_index)) {

                /* Is this PU in the cpuset? */
                if (hwloc_bitmap_isset(cpuset, pu->os_index)) {
                    strncat(str, "B", len - strlen(str));
                } else {
                    strncat(str, ".", len - strlen(str));
                }
            }
        }
        strncat(str, "]", len - strlen(str));
    }

    return OPAL_SUCCESS;
}

int topo_init(hwloc_topology_t *topo_p, char* topofile) {
    hwloc_topology_init(topo_p);

    /* hwloc_topology_ignore_type(*topo_p, HWLOC_OBJ_CACHE); */
    hwloc_topology_ignore_type(*topo_p, HWLOC_OBJ_MISC);
    /* hwloc_topology_ignore_type(*topo_p, HWLOC_OBJ_NODE); */
    hwloc_topology_ignore_type(*topo_p, HWLOC_OBJ_MACHINE);
    hwloc_topology_ignore_type(*topo_p, HWLOC_OBJ_SYSTEM);
    hwloc_topology_set_flags(*topo_p, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);

    if (topofile != NULL) {
        if (0 != hwloc_topology_set_xml(*topo_p, topofile)) {
            fprintf(stderr, "topofile failed\n");
        }
    }

    return hwloc_topology_load(*topo_p);
}

char *binding_openmpi_tostr(hwloc_cpuset_t cpuset, hwloc_topology_t topo) {
    int n_pus = hwloc_get_nbobjs_by_type (topo, HWLOC_OBJ_PU);

    int str_size = 4*n_pus + 512; /* to be sure */
    char *str = malloc(str_size);
    int rc = opal_hwloc_base_cset2mapstr(str, str_size, topo, cpuset);
    switch (rc) {
    case OPAL_ERR_NOT_BOUND:
        snprintf(str, str_size, "not bound (or bound to all available processors)");
        return str;
    case OPAL_SUCCESS:
        return str;
    default:
        free(str);
        return NULL;
    }
}

char *binding_obj_tostr(hwloc_cpuset_t cpuset, hwloc_topology_t topo) {
    int str_size = 512;
    char *str = malloc(str_size);
    hwloc_obj_t obj = hwloc_get_first_largest_obj_inside_cpuset(topo, cpuset);
    while (obj->type == HWLOC_OBJ_CACHE && obj->arity == 1) {
        obj = obj->first_child;
    }

    int verbose = 0;
    hwloc_obj_snprintf(str, str_size, topo, obj, NULL, verbose);
    return str;
}


void print_binding(hwloc_topology_t topo, int pid) {
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_get_proc_cpubind(topo, pid, cpuset, 0);

    char *report_binding = binding_openmpi_tostr(cpuset, topo);
    char *objstr = binding_obj_tostr(cpuset, topo);
    char *maskstr;
    hwloc_bitmap_asprintf(&maskstr, cpuset);

    printf("%d, %s, %s, %s\n", pid, maskstr, report_binding, objstr);

    free(report_binding);
    free(objstr);
    free(maskstr);
    hwloc_bitmap_free(cpuset);
}

char *memory_to_string(hwloc_uint64_t memory) {
    static char *units[] = {"B", "KB" "MB", "GB", "TB", "PB", NULL};
    unsigned int i = 0;
    for (i=0; memory > 1024 && i < sizeof(units) ; i++) {
        memory /= 1024;
    }
    char *str;
    asprintf(&str, "%lu %s", memory, units[i]);
    return str;
}

void print_node_info(hwloc_topology_t topo) {
    hwloc_obj_t machine = hwloc_get_obj_by_type (topo, HWLOC_OBJ_MACHINE, 0);
    hwloc_obj_t socket0 = hwloc_get_obj_by_type (topo, HWLOC_OBJ_SOCKET, 0);
    const char *host_name = hwloc_obj_get_info_by_name(machine, "HostName");
    const char *product_version = hwloc_obj_get_info_by_name(machine, "DMIProductVersion");
    hwloc_uint64_t total_memory = machine->memory.total_memory;
    const char *cpu_model = hwloc_obj_get_info_by_name(socket0, "CPUModel");
    printf("%s, %s, %s, %s RAM, ", host_name, product_version, cpu_model,
           memory_to_string(total_memory));

    int depth = hwloc_topology_get_depth(topo); 
    int d;
    int d_printed=0;
    for (d=1; d<depth; d++) {
        int w = hwloc_get_nbobjs_by_depth(topo, d);
        int w_next = hwloc_get_nbobjs_by_depth(topo, d+1);
        int w_prev = hwloc_get_nbobjs_by_depth(topo, d-1);
        hwloc_obj_type_t t = hwloc_get_depth_type(topo, d);
        if (t == HWLOC_OBJ_CACHE && !((w != w_next) && (w != w_prev))) continue;
        
        if (d_printed != 0) {
            printf(".");
        }
        printf("%s:%d", hwloc_obj_type_string(t), w);
        d_printed++;
        if (w_next == w) { d++; };
    }
    printf("\n");
}

void usage(char *prog) {
    fprintf(stderr, "usage: %s [-tbh] [pid]+  \n", prog);
    fprintf(stderr, " [-b] [pid]+   : print binding of pids\n");
    fprintf(stderr, " -t            : print node and topology info\n");
    fprintf(stderr, " -h            : print this help\n");
}

int main(int argc, char *argv[]) {
    int opt;
    enum {BINDING, TOPOINFO} action = BINDING;
    while ((opt = getopt(argc, argv, "bth")) != -1) {
        switch(opt) {
        case 'b': action=BINDING; break;
        case 't': action=TOPOINFO; break;
        case 'h':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    hwloc_topology_t topo;
    topo_init(&topo, NULL);

    if (action == TOPOINFO) {
        print_node_info(topo);
    } else if (action == BINDING) {
        if (argc - optind < 1) {
            usage(argv[0]);
            return 1;
        }

        int i;
        for (i=optind; i<argc; i++) {
            int pid = atoi(argv[i]);
            print_binding(topo, pid);
        }
    }
    return 0;
}



