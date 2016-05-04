#include <stdio.h>
#include <hwloc.h>

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
    //hwloc_obj_t root;
    /* opal_hwloc_topo_data_t *sum; */

    str[0] = tmp[stmp] = '\0';

    /* if the cpuset is all zero, then not bound */
    if (hwloc_bitmap_iszero(cpuset)) {
        return OPAL_ERR_NOT_BOUND;
    }

    /* if the cpuset includes all available cpus, then we are unbound */
    //root = hwloc_get_root_obj(topo);
    /* if (NULL == root->userdata) { */
    /*     opal_hwloc_base_filter_cpus(topo); */
    /* } else { */
    /*     sum = (opal_hwloc_topo_data_t*)root->userdata; */
    /*     if (NULL == sum->available) { */
    /*        return OPAL_ERROR; */
    /*     } */
    /*     if (0 != hwloc_bitmap_isincluded(sum->available, cpuset)) { */
    /*         return OPAL_ERR_NOT_BOUND; */
    /*     } */
    /* } */

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

char *get_binding_openmpi(hwloc_cpuset_t cpuset) {

    hwloc_topology_t topo;
    hwloc_topology_init(&topo);
    hwloc_topology_load(topo);
    /* hwloc_cpuset_t cpuset = hwloc_bitmap_alloc(); */
    /* hwloc_get_cpubind(topo, cpuset, HWLOC_CPUBIND_THREAD); */

    int str_size = 512;
    char *str = malloc(str_size);
    int rc = opal_hwloc_base_cset2mapstr(str, str_size, topo, cpuset);
    //snprintf(str, str_size, "error...");
    
    if (rc != 0) return NULL;
    return str;
}

int main(int argc, char *argv[]) {
    if (argc <= 1) return 1;
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    int rc = hwloc_bitmap_taskset_sscanf(cpuset, argv[1]);
    if (rc != 0) return 1;
    char *report_binding = get_binding_openmpi(cpuset);
    printf("%s", report_binding);
    return 0;
}
