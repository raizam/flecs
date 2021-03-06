#include <string.h>
#include <assert.h>
#include "include/util/time.h"
#include "include/util/stats.h"
#include "include/private/flecs.h"


/* -- Global array parameters -- */

const ecs_vector_params_t table_arr_params = {
    .element_size = sizeof(ecs_table_t)
};

const ecs_vector_params_t handle_arr_params = {
    .element_size = sizeof(ecs_entity_t)
};

const ecs_vector_params_t stage_arr_params = {
    .element_size = sizeof(ecs_stage_t)
};

const ecs_vector_params_t char_arr_params = {
    .element_size = sizeof(char)
};


/* -- Global variables -- */

ecs_type_t TEcsComponent;
ecs_type_t TEcsTypeComponent;
ecs_type_t TEcsPrefab;
ecs_type_t TEcsPrefabParent;
ecs_type_t TEcsPrefabBuilder;
ecs_type_t TEcsRowSystem;
ecs_type_t TEcsColSystem;
ecs_type_t TEcsId;
ecs_type_t TEcsHidden;
ecs_type_t TEcsContainer;

const char *ECS_COMPONENT_ID =      "EcsComponent";
const char *ECS_TYPE_COMPONENT_ID = "EcsTypeComponent";
const char *ECS_PREFAB_ID =         "EcsPrefab";
const char *ECS_PREFAB_PARENT_ID =  "EcsPrefabParent";
const char *ECS_PREFAB_BUILDER_ID = "EcsPrefabBuilder";
const char *ECS_ROW_SYSTEM_ID =     "EcsRowSystem";
const char *ECS_COL_SYSTEM_ID =     "EcsColSystem";
const char *ECS_ID_ID =             "EcsId";
const char *ECS_HIDDEN_ID =         "EcsHidden";
const char *ECS_CONTAINER_ID =      "EcsContainer";


/** Comparator function for handles */
static
int compare_handle(
    const void *p1,
    const void *p2)
{
    return *(ecs_entity_t*)p1 - *(ecs_entity_t*)p2;
}

/** Bootstrap builtin component types and commonly used types */
static
void bootstrap_types(
    ecs_world_t *world)
{
    ecs_stage_t *stage = &world->main_stage;
    TEcsComponent = ecs_type_register(world, stage, EEcsComponent, NULL);
    TEcsTypeComponent = ecs_type_register(world, stage, EEcsTypeComponent, NULL);
    TEcsPrefab = ecs_type_register(world, stage, EEcsPrefab, NULL);
    TEcsPrefabParent = ecs_type_register(world, stage, EEcsPrefabParent, NULL);
    TEcsPrefabBuilder = ecs_type_register(world, stage, EEcsPrefabBuilder, NULL);
    TEcsRowSystem = ecs_type_register(world, stage, EEcsRowSystem, NULL);
    TEcsColSystem = ecs_type_register(world, stage, EEcsColSystem, NULL);
    TEcsId = ecs_type_register(world, stage, EEcsId, NULL);
    TEcsHidden = ecs_type_register(world, stage, EEcsHidden, NULL);
    TEcsContainer = ecs_type_register(world, stage, EEcsContainer, NULL);    

    world->t_component = ecs_type_merge(world, stage, TEcsComponent, TEcsId, 0);
    world->t_type = ecs_type_merge(world, stage, TEcsTypeComponent, TEcsId, 0);
    world->t_prefab = ecs_type_merge(world, stage, TEcsPrefab, TEcsId, 0);
    world->t_row_system = ecs_type_merge(world, stage, TEcsRowSystem, TEcsId, 0);
    world->t_col_system = ecs_type_merge(world, stage, TEcsColSystem, TEcsId, 0);
}

/** Initialize component table. This table is manually constructed to bootstrap
 * flecs. After this function has been called, the builtin components can be
 * created. */
static
ecs_table_t* bootstrap_component_table(
    ecs_world_t *world)
{
    ecs_stage_t *stage = &world->main_stage;
    ecs_table_t *result = ecs_vector_add(&stage->tables, &table_arr_params);
    ecs_vector_t *type = ecs_map_get(stage->type_index, world->t_component);
    result->type_id = world->t_component;
    result->type = type;
    result->frame_systems = NULL;
    result->columns = ecs_os_malloc(sizeof(ecs_table_column_t) * 3);
    ecs_assert(result->columns != NULL, ECS_OUT_OF_MEMORY, NULL);

    result->columns[0].data = ecs_vector_new(&handle_arr_params, 12);
    result->columns[0].size = sizeof(ecs_entity_t);
    result->columns[1].data = ecs_vector_new(&handle_arr_params, 12);
    result->columns[1].size = sizeof(EcsComponent);
    result->columns[2].data = ecs_vector_new(&handle_arr_params, 12);
    result->columns[2].size = sizeof(EcsId);

    uint32_t index = ecs_vector_get_index(
        stage->tables, &table_arr_params, result);

    ecs_assert(index == 0, ECS_INTERNAL_ERROR, "first table index must be 0");

    ecs_map_set64(stage->table_index, world->t_component, 1);

    return result;
}

/** Bootstrap the EcsComponent component */
static
void bootstrap_component(
    ecs_world_t *world,
    ecs_table_t *table,
    ecs_entity_t entity,
    const char *id,
    size_t size)
{
    ecs_stage_t *stage = &world->main_stage;

    /* Insert row into table to store EcsComponent itself */
    int32_t index = ecs_table_insert(world, table, table->columns, entity);

    /* Create record in entity index */
    ecs_row_t row = {.type_id = world->t_component, .index = index};
    ecs_map_set64(stage->entity_index, entity, ecs_from_row(row));

    /* Set size and id */
    EcsComponent *component_data = ecs_vector_first(table->columns[1].data);
    EcsId *id_data = ecs_vector_first(table->columns[2].data);
    
    component_data[index - 1].size = size;
    id_data[index - 1] = id;
}

static
void notify_create_table(
    ecs_world_t *world,
    ecs_vector_t *systems,
    ecs_table_t *table)
{
    ecs_entity_t *buffer = ecs_vector_first(systems);
    uint32_t i, count = ecs_vector_count(systems);

    for (i = 0; i < count; i ++) {
        ecs_col_system_notify_of_table(world, buffer[i], table);
    }
}

void ecs_notify_systems_of_table(
    ecs_world_t *world,
    ecs_table_t *table)
{
    notify_create_table(world, world->pre_update_systems, table);
    notify_create_table(world, world->post_update_systems, table);
    notify_create_table(world, world->on_load_systems, table);
    notify_create_table(world, world->post_load_systems, table);
    notify_create_table(world, world->pre_store_systems, table);
    notify_create_table(world, world->on_store_systems, table);
    notify_create_table(world, world->on_validate_systems, table);
    notify_create_table(world, world->on_update_systems, table);
    notify_create_table(world, world->inactive_systems, table);
    notify_create_table(world, world->on_demand_systems, table);
}

/** Create a new table and register it with the world and systems. A table in
 * flecs is equivalent to an archetype */
static
ecs_table_t* create_table(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_type_t type_id)
{
    /* Add and initialize table */
    ecs_table_t *result = ecs_vector_add(&stage->tables, &table_arr_params);
    
    result->type_id = type_id;

    if (ecs_table_init(world, stage, result) != 0) {
        return NULL;
    }

    uint32_t index = ecs_vector_get_index(stage->tables, &table_arr_params, result);
    ecs_map_set64(stage->table_index, type_id, index + 1);

    if (stage == &world->main_stage && !world->is_merging) {
        ecs_notify_systems_of_table(world, result);
    }

    assert(result != NULL);

    return result;
}

#ifndef NDEBUG
static
void no_threading(
    const char *function)
{
    ecs_os_dbg("threading unavailable: %s not implemented", function);
}

static
void no_time(
    const char *function)
{
    ecs_os_dbg("time management: %s not implemented", function);
}
#endif

/* -- Private functions -- */

/** Get pointer to table data from type id */
ecs_table_t* ecs_world_get_table(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_type_t type_id)
{
    ecs_stage_t *main_stage = &world->main_stage;
    uint32_t table_index = ecs_map_get64(main_stage->table_index, type_id);

    if (!table_index && world->in_progress) {
        assert(stage != NULL);
        table_index = ecs_map_get64(stage->table_index, type_id);
        if (table_index) {
            return ecs_vector_get(
                stage->tables, &table_arr_params, table_index - 1);
        }
    }

    if (table_index) {
        return ecs_vector_get(
            main_stage->tables, &table_arr_params, table_index - 1);
    } else {
        return create_table(world, stage, type_id);
    }

    return NULL;
}

static
ecs_vector_t** frame_system_array(
    ecs_world_t *world,
    EcsSystemKind kind)
{
    if (kind == EcsOnUpdate) {
        return &world->on_update_systems;
    } else if (kind == EcsOnValidate) {
        return &world->on_validate_systems;
    } else if (kind == EcsPreUpdate) {
        return &world->pre_update_systems;
    } else if (kind == EcsPostUpdate) {
        return &world->post_update_systems;
    } else if (kind == EcsPostLoad) {
        return &world->post_load_systems;
    } else if (kind == EcsOnLoad) {
        return &world->on_load_systems;
    } else if (kind == EcsPreStore) {
        return &world->pre_store_systems;        
    } else if (kind == EcsOnStore) {
        return &world->on_store_systems;
    } else if (kind == EcsManual) {
        return &world->on_demand_systems;
    } else {
        ecs_abort(ECS_INTERNAL_ERROR, 0);
    }

    return NULL;
}

/** Inactive systems are systems that either:
 * - are not enabled
 * - matched with no tables
 * - matched with only empty tables.
 *
 * These systems are not considered in the main loop, which can speed up things
 * when applications contain large numbers of disabled systems.
 */
void ecs_world_activate_system(
    ecs_world_t *world,
    ecs_entity_t system,
    EcsSystemKind kind,
    bool active)
{
    ecs_vector_t *src_array, *dst_array;

    if (active) {
        src_array = world->inactive_systems;
        dst_array = *frame_system_array(world, kind);
     } else {
        src_array = *frame_system_array(world, kind);
        dst_array = world->inactive_systems;
    }

    uint32_t i, count = ecs_vector_count(src_array);
    for (i = 0; i < count; i ++) {
        ecs_entity_t *h = ecs_vector_get(
            src_array, &handle_arr_params, i);
        if (*h == system) {
            break;
        }
    }

    if (i == count) {
        return; /* System is disabled */
    }

    ecs_vector_move_index(
        &dst_array, src_array, &handle_arr_params, i);

    if (active) {
         *frame_system_array(world, kind) = dst_array;
         qsort(dst_array, ecs_vector_count(dst_array) + 1,
          sizeof(ecs_entity_t), compare_handle);
    } else {
        world->inactive_systems = dst_array;
        qsort(src_array, ecs_vector_count(src_array) + 1,
          sizeof(ecs_entity_t), compare_handle);
    }
}

union RowUnion {
    ecs_row_t row;
    uint64_t value;
};

/** Utility to translate from uint64 to ecs_row_t */
ecs_row_t ecs_to_row(
    uint64_t value)
{
    union RowUnion u = {.value = value};
    return u.row;
}

/** Utility to translate from ecs_row_t to uint64 */
uint64_t ecs_from_row(
    ecs_row_t row)
{
    union RowUnion u = {.row = row};
    return u.value;
}

char* ecs_type_tostr(
    ecs_world_t *world,
    ecs_stage_t *stage,
    ecs_type_t type_id)
{
    ecs_vector_t *type = ecs_type_get(world, stage, type_id);
    ecs_vector_t *chbuf = ecs_vector_new(&char_arr_params, 32);
    char *dst;
    uint32_t len;
    char buf[15];

    ecs_entity_t *handles = ecs_vector_first(type);
    uint32_t i, count = ecs_vector_count(type);

    for (i = 0; i < count; i ++) {
        ecs_entity_t h = handles[i];
        if (i) {
            *(char*)ecs_vector_add(&chbuf, &char_arr_params) = ',';
        }

        const char *str = NULL;
        EcsId *id = ecs_get_ptr(world, h, EcsId);
        if (id) {
            str = *id;
        } else {
            int h_int = h;
            sprintf(buf, "%u", h_int);
            str = buf;
        }
        len = strlen(str);
        dst = ecs_vector_addn(&chbuf, &char_arr_params, len);
        memcpy(dst, str, len);
    }

    *(char*)ecs_vector_add(&chbuf, &char_arr_params) = '\0';

    char *result = strdup(ecs_vector_first(chbuf));
    ecs_vector_free(chbuf);
    return result;
}

ecs_stage_t *ecs_get_stage(
    ecs_world_t **world_ptr)
{
    ecs_world_t *world = *world_ptr;
    if (world->magic == ECS_WORLD_MAGIC) {
        if (world->in_progress) {
            return &world->temp_stage;
        } else {
            return &world->main_stage;
        }
    } else if (world->magic == ECS_THREAD_MAGIC) {
        ecs_thread_t *thread = (ecs_thread_t*)world;
        *world_ptr = thread->world;
        return thread->stage;
    } else {
        ecs_os_err("Invalid world object\n");
        assert(0);
        return NULL;
    }
}

static
void col_systems_deinit(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    uint32_t i, count = ecs_vector_count(systems);
    ecs_entity_t *buffer = ecs_vector_first(systems);

    for (i = 0; i < count; i ++) {
        EcsColSystem *ptr = ecs_get_ptr(world, buffer[i], EcsColSystem);
        ecs_vector_free(ptr->base.columns);
        ecs_vector_free(ptr->components);
        ecs_vector_free(ptr->inactive_tables);
        ecs_vector_free(ptr->jobs);
        ecs_vector_free(ptr->tables);
        ecs_vector_free(ptr->refs);
    }
}

static
void row_systems_deinit(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    uint32_t i, count = ecs_vector_count(systems);
    ecs_entity_t *buffer = ecs_vector_first(systems);

    for (i = 0; i < count; i ++) {
        EcsRowSystem *ptr = ecs_get_ptr(world, buffer[i], EcsRowSystem);
        ecs_vector_free(ptr->base.columns);
        ecs_vector_free(ptr->components);
    }
}

static
void row_index_deinit(
    ecs_map_t *sys_index)
{
    EcsIter it = ecs_map_iter(sys_index);

    while (ecs_iter_hasnext(&it)) {
        ecs_vector_t *v = ecs_iter_next(&it);
        ecs_vector_free(v);
    }

    ecs_map_free(sys_index);
}

static
void deinit_tables(
    ecs_world_t *world)
{
    ecs_table_t *tables = ecs_vector_first(world->main_stage.tables);
    int i, count = ecs_vector_count(world->main_stage.tables);

    for (i = 0; i < count; i ++) {
        ecs_table_deinit(world, &tables[i]);
    }
}



/* -- Public functions -- */

ecs_world_t *ecs_init(void) {
    ecs_os_set_api_defaults();

#ifdef __BAKE__
    ut_init(NULL);
    if (ut_load_init(NULL, NULL, NULL, NULL)) {
        ecs_os_err("warning: failed to initialize package loader");
    }
#endif

    ecs_assert(ecs_os_api.malloc != NULL, ECS_MISSING_OS_API, "malloc");
    ecs_assert(ecs_os_api.realloc != NULL, ECS_MISSING_OS_API, "realloc");
    ecs_assert(ecs_os_api.calloc != NULL, ECS_MISSING_OS_API, "calloc");

#ifndef NDEBUG
    bool thr_ok = true;
    if (!ecs_os_api.mutex_new) {thr_ok = false; no_threading("mutex_new");}
    if (!ecs_os_api.mutex_free) {thr_ok = false; no_threading("mutex_free");}
    if (!ecs_os_api.mutex_lock) {thr_ok = false; no_threading("mutex_lock");}
    if (!ecs_os_api.mutex_unlock) {thr_ok = false; no_threading("mutex_unlock");}
    if (!ecs_os_api.cond_new) {thr_ok = false; no_threading("cond_new");}
    if (!ecs_os_api.cond_free) {thr_ok = false; no_threading("cond_free");}
    if (!ecs_os_api.cond_wait) {thr_ok = false; no_threading("cond_wait");}
    if (!ecs_os_api.cond_signal) {thr_ok = false; no_threading("cond_signal");}
    if (!ecs_os_api.cond_broadcast) {thr_ok = false; no_threading("cond_broadcast"); }
    if (!ecs_os_api.thread_new) {thr_ok = false; no_threading("thread_new");}
    if (!ecs_os_api.thread_join) {thr_ok = false; no_threading("thread_join");}
    if (thr_ok) {
        ecs_os_dbg("threading available");
    }

    bool time_ok = true;
    if (!ecs_os_api.get_time) {time_ok = false; no_time("get_time");}
    if (!ecs_os_api.sleep) {time_ok = false; no_time("sleep");}
    if (time_ok) {
        ecs_os_dbg("time management available");
    }
#endif

    ecs_world_t *world = ecs_os_malloc(sizeof(ecs_world_t));
    ecs_assert(world != NULL, ECS_OUT_OF_MEMORY, NULL);

    world->magic = ECS_WORLD_MAGIC;

    world->on_update_systems = ecs_vector_new(&handle_arr_params, 0);
    world->on_validate_systems = ecs_vector_new(&handle_arr_params, 0);
    world->pre_update_systems = ecs_vector_new(&handle_arr_params, 0);
    world->post_update_systems = ecs_vector_new(&handle_arr_params, 0);
    world->post_load_systems = ecs_vector_new(&handle_arr_params, 0);
    world->on_load_systems = ecs_vector_new(&handle_arr_params, 0);
    world->pre_store_systems = ecs_vector_new( &handle_arr_params, 0);
    world->on_store_systems = ecs_vector_new( &handle_arr_params, 0);
    world->inactive_systems = ecs_vector_new(&handle_arr_params, 0);
    world->on_demand_systems = ecs_vector_new(&handle_arr_params, 0);

    world->add_systems = ecs_vector_new(&handle_arr_params, 0);
    world->remove_systems = ecs_vector_new(&handle_arr_params, 0);
    world->set_systems = ecs_vector_new(&handle_arr_params, 0);
    world->tasks = ecs_vector_new(&handle_arr_params, 0);
    world->fini_tasks = ecs_vector_new(&handle_arr_params, 0);

    world->type_sys_add_index = ecs_map_new(0);
    world->type_sys_remove_index = ecs_map_new(0);
    world->type_sys_set_index = ecs_map_new(0);
    world->type_handles = ecs_map_new(0);
    world->prefab_index = ecs_map_new(0);
    world->prefab_parent_index = ecs_map_new(0);

    world->worker_stages = NULL;
    world->worker_threads = NULL;
    world->jobs_finished = 0;
    world->threads_running = 0;
    world->valid_schedule = false;
    world->quit_workers = false;
    world->in_progress = false;
    world->is_merging = false;
    world->auto_merge = true;
    world->measure_frame_time = false;
    world->measure_system_time = false;
    world->last_handle = 0;
    world->should_quit = false;
    world->should_match = false;

    world->frame_start = (ecs_time_t){0, 0};
    world->frame_time = 0;
    world->merge_time = 0;
    world->system_time = 0;
    world->target_fps = 0;
    world->fps_sleep = 0;
    world->tick = 0;

    world->context = NULL;

    world->arg_fps = 0;
    world->arg_threads = 0;

    ecs_stage_init(world, &world->main_stage);
    ecs_stage_init(world, &world->temp_stage);

    /* Initialize families for builtin types */
    bootstrap_types(world);

    /* Create table that will hold components (EcsComponent, EcsId) */
    ecs_table_t *table = bootstrap_component_table(world);
    assert(table != NULL);

    /* Create records for internal components */
    bootstrap_component(world, table, EEcsComponent, ECS_COMPONENT_ID, sizeof(EcsComponent));
    bootstrap_component(world, table, EEcsTypeComponent, ECS_TYPE_COMPONENT_ID, sizeof(EcsTypeComponent));
    bootstrap_component(world, table, EEcsPrefab, ECS_PREFAB_ID, sizeof(EcsPrefab));
    bootstrap_component(world, table, EEcsPrefabParent, ECS_PREFAB_PARENT_ID, sizeof(EcsPrefabParent));
    bootstrap_component(world, table, EEcsPrefabBuilder, ECS_PREFAB_BUILDER_ID, sizeof(EcsPrefabBuilder));
    bootstrap_component(world, table, EEcsRowSystem, ECS_ROW_SYSTEM_ID, sizeof(EcsRowSystem));
    bootstrap_component(world, table, EEcsColSystem, ECS_COL_SYSTEM_ID, sizeof(EcsColSystem));
    bootstrap_component(world, table, EEcsId, ECS_ID_ID, sizeof(EcsId));
    bootstrap_component(world, table, EEcsHidden, ECS_HIDDEN_ID, 0);
    bootstrap_component(world, table, EEcsContainer, ECS_CONTAINER_ID, 0);

    world->last_handle = EEcsContainer + 1;
    world->max_handle = 0;

    /* Create two systems for initializing and setting EcsPrefab */
    ecs_new_system(world, "EcsInitPrefab", EcsOnAdd, "EcsPrefab", EcsInitPrefab);
    ecs_new_system(world, "EcsSetPrefab", EcsOnSet, "EcsPrefab", EcsSetPrefab);

    return world;
}

#define ARG(short, long, action)\
    if (i < argc) {\
        if (argv[i][0] == '-') {\
            if (argv[i][1] == '-') {\
                if (long && !strcmp(&argv[i][2], long ? long : "")) {\
                    action;\
                    parsed = true;\
                }\
            } else {\
                if (short && argv[i][1] == short) {\
                    action;\
                    parsed = true;\
                }\
            }\
        }\
    }

ecs_world_t* ecs_init_w_args(
    int argc,
    char *argv[])
{
    /* First parse debug argument so logging is enabled while initializing world */ 
    int i;
    for (i = 1; i < argc; i ++) {
        if (argv[i][0] == '-') {
            bool parsed = false;

            ARG(0, "debug", ecs_os_enable_dbg(true));

            /* Ignore arguments that were not parsed */
            (void)parsed;
        } else {
            /* Ignore arguments that don't start with '-' */
        }
    }

    ecs_world_t *world = ecs_init();

    /* Parse remaining arguments */
    for (i = 1; i < argc; i ++) {
        if (argv[i][0] == '-') {
            bool parsed = false;
            
            ARG(0, "threads", 
                world->arg_threads = atoi(argv[i + 1]); 
                ecs_set_threads(world, world->arg_threads);
                i ++;
            );

            ARG(0, "fps", 
                ecs_set_target_fps(world, atoi(argv[i + 1]));
                world->arg_fps = world->target_fps; 
                i ++);

            ARG(0, "admin", 
				ecs_enable_admin(world, atoi(argv[i + 1]));
                i ++);

            /* Ignore arguments that were not parsed */
            (void)parsed;
        } else {
            /* Ignore arguments that don't start with '-' */
        }
    }
    
    return world;
}

int ecs_fini(
    ecs_world_t *world)
{
    assert(world->magic == ECS_WORLD_MAGIC);
    assert(!world->in_progress);
    assert(!world->is_merging);

    uint32_t i, system_count = ecs_vector_count(world->fini_tasks);
    if (system_count) {
        ecs_entity_t *buffer = ecs_vector_first(world->fini_tasks);
        for (i = 0; i < system_count; i ++) {
            ecs_run_task(world, buffer[i]);
        }
    }

    if (world->worker_threads) {
        ecs_set_threads(world, 0);
    }

    deinit_tables(world);

    col_systems_deinit(world, world->on_update_systems);
    col_systems_deinit(world, world->on_validate_systems);
    col_systems_deinit(world, world->pre_update_systems);
    col_systems_deinit(world, world->post_update_systems);
    col_systems_deinit(world, world->on_load_systems);
    col_systems_deinit(world, world->post_load_systems);
    col_systems_deinit(world, world->pre_store_systems);
    col_systems_deinit(world, world->on_store_systems);
    col_systems_deinit(world, world->on_demand_systems);
    col_systems_deinit(world, world->inactive_systems);

    row_systems_deinit(world, world->add_systems);
    row_systems_deinit(world, world->remove_systems);
    row_systems_deinit(world, world->set_systems);

    row_index_deinit(world->type_sys_add_index);
    row_index_deinit(world->type_sys_remove_index);
    row_index_deinit(world->type_sys_set_index);

    ecs_stage_deinit(world, &world->main_stage);
    ecs_stage_deinit(world, &world->temp_stage);

    ecs_vector_free(world->on_update_systems);
    ecs_vector_free(world->on_validate_systems);
    ecs_vector_free(world->pre_update_systems);
    ecs_vector_free(world->post_update_systems);
    ecs_vector_free(world->on_load_systems);
    ecs_vector_free(world->post_load_systems);
    ecs_vector_free(world->pre_store_systems);
    ecs_vector_free(world->on_store_systems);

    ecs_vector_free(world->inactive_systems);
    ecs_vector_free(world->on_demand_systems);
    ecs_vector_free(world->tasks);
    ecs_vector_free(world->fini_tasks);

    ecs_vector_free(world->add_systems);
    ecs_vector_free(world->remove_systems);
    ecs_vector_free(world->set_systems);

    ecs_map_free(world->prefab_index);
    ecs_map_free(world->type_handles);

    world->magic = 0;

    ecs_os_free(world);

#ifdef __BAKE__
    ut_deinit();
#endif    

    return 0;
}

void ecs_dim(
    ecs_world_t *world,
    uint32_t entity_count)
{
    assert(world->magic == ECS_WORLD_MAGIC);
    ecs_map_set_size(world->main_stage.entity_index, entity_count);
}

void _ecs_dim_type(
    ecs_world_t *world,
    ecs_type_t type,
    uint32_t entity_count)
{
    assert(world->magic == ECS_WORLD_MAGIC);
    if (type) {
        ecs_table_t *table = ecs_world_get_table(world, &world->main_stage, type);
        if (table) {
            ecs_table_dim(table, entity_count);
        }
    }
}

ecs_entity_t ecs_lookup_child(
    ecs_world_t *world,
    ecs_entity_t parent,
    const char *id)
{
    ecs_table_t *tables = ecs_vector_first(world->main_stage.tables);
    uint32_t t, count = ecs_vector_count(world->main_stage.tables);

    for (t = 0; t < count; t ++) {
        int16_t column_index;

        if ((column_index = ecs_type_index_of(tables[t].type, EEcsId)) == -1) {
            continue;
        }

        if (parent && ecs_type_index_of(tables[t].type, parent) == -1) {
            continue;
        }

        ecs_table_column_t *column = &tables[t].columns[column_index + 1];
        EcsId *buffer = ecs_vector_first(column->data);
        uint32_t i, count = ecs_vector_count(column->data);
        
        for (i = 0; i < count; i ++) {
            if (!strcmp(buffer[i], id)) {
                return *(ecs_entity_t*)ecs_vector_get(
                    tables[t].columns[0].data, &handle_arr_params, i);
            }
        }
    }

    return 0;    
}

ecs_entity_t ecs_lookup(
    ecs_world_t *world,
    const char *id)
{
    return ecs_lookup_child(world, 0, id);
}

static
void rematch_system_array(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    uint32_t i, count = ecs_vector_count(systems);
    ecs_entity_t *buffer = ecs_vector_first(systems);

    for (i = 0; i < count; i ++) {
        ecs_entity_t system = buffer[i];
        ecs_rematch_system(world, system);

        if (system != buffer[i]) {
            /* It is possible that rematching a system caused it to be activated
             * or deactived. In that case, reevaluate the current element again,
             * as it will now contain a different system. */
            i --;
            count = ecs_vector_count(systems);
        }
    }
}

static
void rematch_systems(
    ecs_world_t *world)
{
    rematch_system_array(world, world->on_load_systems);
    rematch_system_array(world, world->post_load_systems);
    rematch_system_array(world, world->pre_update_systems);
    rematch_system_array(world, world->on_update_systems);
    rematch_system_array(world, world->on_validate_systems);
    rematch_system_array(world, world->post_update_systems);
    rematch_system_array(world, world->pre_store_systems);
    rematch_system_array(world, world->on_store_systems);    
    rematch_system_array(world, world->inactive_systems);   
}

static
void revalidate_system_array(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    uint32_t i, count = ecs_vector_count(systems);
    ecs_entity_t *buffer = ecs_vector_first(systems);

    for (i = 0; i < count; i ++) {
        ecs_entity_t system = buffer[i];
        ecs_revalidate_system_refs(world, system);
    }
}

static
void revalidate_system_refs(
    ecs_world_t *world)
{
    revalidate_system_array(world, world->on_load_systems);
    revalidate_system_array(world, world->post_load_systems);
    revalidate_system_array(world, world->pre_update_systems);
    revalidate_system_array(world, world->on_update_systems);
    revalidate_system_array(world, world->on_validate_systems);
    revalidate_system_array(world, world->post_update_systems);
    revalidate_system_array(world, world->pre_store_systems);
    revalidate_system_array(world, world->on_store_systems);    
    revalidate_system_array(world, world->inactive_systems);   
}

static
void run_single_thread_stage(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    uint32_t i, system_count = ecs_vector_count(systems);

    if (system_count) {
        ecs_entity_t *buffer = ecs_vector_first(systems);

        world->in_progress = true;

        for (i = 0; i < system_count; i ++) {
            ecs_run(world, buffer[i], world->delta_time, NULL);
        }

        if (world->auto_merge) {
            world->in_progress = false;
            ecs_merge(world);
            world->in_progress = true;
        }
    }
}

static
void run_multi_thread_stage(
    ecs_world_t *world,
    ecs_vector_t *systems)
{
    /* Run periodic table systems */
    uint32_t i, system_count = ecs_vector_count(systems);
    if (system_count) {
        bool valid_schedule = world->valid_schedule;
        ecs_entity_t *buffer = ecs_vector_first(systems);

        world->in_progress = true;

        for (i = 0; i < system_count; i ++) {
            if (!valid_schedule) {
                ecs_schedule_jobs(world, buffer[i]);
            }
            ecs_prepare_jobs(world, buffer[i]);
        }
        ecs_run_jobs(world);

        if (world->auto_merge) {
            world->in_progress = false;
            ecs_merge(world);
            world->in_progress = true;
        }
    }
}

static
void run_tasks(
    ecs_world_t *world)
{
    /* Run periodic row systems (not matched to any entity) */
    uint32_t i, system_count = ecs_vector_count(world->tasks);
    if (system_count) {
        world->in_progress = true;

        ecs_entity_t *buffer = ecs_vector_first(world->tasks);
        for (i = 0; i < system_count; i ++) {
            ecs_run_task(world, buffer[i]);
        }
    }
}

static
float start_measure_frame(
    ecs_world_t *world,
    float user_delta_time)
{
    float delta_time = 0;

    if (world->measure_frame_time || !user_delta_time) {
        if (world->frame_start.sec) {
            delta_time = ecs_time_measure(&world->frame_start);
        } else {
            ecs_time_measure(&world->frame_start);
            if (world->target_fps) {
                delta_time = 1.0 / world->target_fps;
            } else {
                delta_time = 1.0 / 60.0; /* Best guess */
            }
        }
    }

    return delta_time;
}

static
void stop_measure_frame(
    ecs_world_t *world,
    float delta_time)
{
    if (world->measure_frame_time) {
        ecs_time_t t = world->frame_start;
        world->frame_time += ecs_time_measure(&t);
        world->tick ++;

        /* Sleep if processing faster than target FPS */
        float target_fps = world->target_fps;
        if (target_fps) {
            float sleep = (1.0 / target_fps) - delta_time + world->fps_sleep;
            if (sleep < 0) {
                sleep = 0;
            }
            world->fps_sleep = sleep;

            if (sleep > 0.005) {
                ecs_sleepf(sleep);
            }
        }
    }
}

bool ecs_progress(
    ecs_world_t *world,
    float user_delta_time)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    ecs_assert(user_delta_time || ecs_os_api.get_time, ECS_MISSING_OS_API, "get_time");

    /* Start measuring total frame time */
    float delta_time = start_measure_frame(world, user_delta_time);

    if (!user_delta_time) {
        user_delta_time = delta_time;
    }

    world->delta_time = user_delta_time;
    world->merge_time = 0;

    bool has_threads = ecs_vector_count(world->worker_threads) != 0;

    if (world->should_match) {
        rematch_systems(world);
        world->should_match = false;
    }

    if (world->should_resolve) {
        revalidate_system_refs(world);
        world->should_resolve = false;
    }

    /* -- System execution starts here -- */

    run_single_thread_stage(world, world->on_load_systems);
    run_single_thread_stage(world, world->post_load_systems);

    if (has_threads) {
        run_multi_thread_stage(world, world->pre_update_systems);
        run_multi_thread_stage(world, world->on_update_systems);
        run_multi_thread_stage(world, world->on_validate_systems);
        run_multi_thread_stage(world, world->post_update_systems);
    } else {
        run_single_thread_stage(world, world->pre_update_systems);
        run_single_thread_stage(world, world->on_update_systems);
        run_single_thread_stage(world, world->on_validate_systems);
        run_single_thread_stage(world, world->post_update_systems);
    }

    run_tasks(world);

    run_single_thread_stage(world, world->pre_store_systems);
    run_single_thread_stage(world, world->on_store_systems);

    /* -- System execution stops here -- */

    stop_measure_frame(world, delta_time);

    /* Time spent on systems is time spent on frame minus merge time */
    world->system_time = world->frame_time - world->merge_time;

    world->in_progress = false;

    return !world->should_quit;
}

float ecs_get_delta_time(
    ecs_world_t *world)
{
    return world->delta_time;
}

void ecs_quit(
    ecs_world_t *world)
{
    world->should_quit = true;
}

void ecs_merge(
    ecs_world_t *world)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    assert(world->is_merging == false);

    bool measure_frame_time = world->measure_frame_time;

    world->is_merging = true;

    ecs_time_t t_start;
    if (measure_frame_time) {
        ecs_os_get_time(&t_start);
    }

    ecs_stage_merge(world, &world->temp_stage);

    uint32_t i, count = ecs_vector_count(world->worker_stages);
    if (count) {
        ecs_stage_t *buffer = ecs_vector_first(world->worker_stages);
        for (i = 0; i < count; i ++) {
            ecs_stage_merge(world, &buffer[i]);
        }
    }

    if (measure_frame_time) {
        world->merge_time += ecs_time_measure(&t_start);
    }

    world->is_merging = false;
}

void ecs_set_automerge(
    ecs_world_t *world,
    bool auto_merge)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    world->auto_merge = auto_merge;
}

void ecs_measure_frame_time(
    ecs_world_t *world,
    bool enable)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    ecs_assert(ecs_os_api.get_time != NULL, ECS_MISSING_OS_API, "get_time");
    if (!world->target_fps || enable) {
        world->measure_frame_time = enable;
    }
}

void ecs_measure_system_time(
    ecs_world_t *world,
    bool enable)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    ecs_assert(ecs_os_api.get_time != NULL, ECS_MISSING_OS_API, "get_time");
    world->measure_system_time = enable;
}

void ecs_set_target_fps(
    ecs_world_t *world,
    float fps)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    ecs_assert(ecs_os_api.get_time != NULL, ECS_MISSING_OS_API, "get_time");
    ecs_assert(ecs_os_api.sleep != NULL, ECS_MISSING_OS_API, "sleep");

    if (!world->arg_fps) {
        ecs_measure_frame_time(world, true);
        world->target_fps = fps;
    }
}

/* Spoof EcsAdmin type (needed until we have proper reflection) */
typedef uint16_t EcsAdmin;

int ecs_enable_admin(
	ecs_world_t* world,
	uint16_t port)
{
    if (ecs_import_from_library(
        world, "flecs.systems.civetweb", NULL, 0) == ECS_INVALID_ENTITY) 
    {
        return 1;
    }
    
    if (ecs_import_from_library(
        world, "flecs.systems.admin", NULL, 0) == ECS_INVALID_ENTITY)
    {
        return 2;
    }

    /* Enable monitoring */
    ecs_measure_frame_time(world, true);
    ecs_measure_system_time(world, true);

    /* Create admin instance */
    ecs_entity_t admin = ecs_lookup(world, "EcsAdmin");
    ecs_type_t TEcsAdmin = ecs_type_from_entity(world, admin);
    ecs_set(world, 0, EcsAdmin, {port});

    ecs_os_log("Admin is running on port %d", port);

    return 0;
}

void* ecs_get_context(
    ecs_world_t *world)
{
    ecs_get_stage(&world);
    return world->context;
}

uint32_t ecs_get_tick(
    ecs_world_t *world)
{
    ecs_get_stage(&world);
    return world->tick;    
}

void ecs_set_context(
    ecs_world_t *world,
    void *context)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    world->context = context;
}

void ecs_set_entity_range(
    ecs_world_t *world,
    ecs_entity_t id_start,
    ecs_entity_t id_end)
{
    ecs_assert(world->magic == ECS_WORLD_MAGIC, ECS_INVALID_FROM_WORKER, NULL);
    ecs_assert(!id_end || id_end > id_start, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(!id_end || id_end > world->last_handle, ECS_INVALID_PARAMETERS, NULL);

    if (world->last_handle < id_start) {
        world->last_handle = id_start;
    }

    world->max_handle = id_end;
}

ecs_entity_t _ecs_import(
    ecs_world_t *world,
    ecs_module_init_action_t module,
    const char *module_name,
    int flags,
    void *handles_out,
    size_t handles_size)
{
    ecs_entity_t e = ecs_lookup(world, module_name);
    if (!e) {
        /* Load module */
        module(world, flags);

        /* Lookup module component (must be registered by module) */
        e = ecs_lookup(world, module_name);

        /* Copy value of module component in handles_out parameter */
        if (handles_size && handles_out) {
            ecs_type_t t = ecs_type_from_entity(world, e);
            void *module_ptr = _ecs_get_ptr(world, 0, t);
            memcpy(handles_out, module_ptr, handles_size);
        }

    /* If module was already loaded, copy module component into handles_out */
    } else if (handles_size) {
        ecs_type_t t = ecs_type_from_entity(world, e);
        void *handles_ptr = _ecs_get_ptr(world, 0, t);
        memcpy(handles_out, handles_ptr, handles_size);
    }

    return e;
}

ecs_entity_t ecs_import_from_library(
    ecs_world_t *world,
    const char *library_name,
    const char *module_name,
    int flags)
{
#ifdef __BAKE__
    char *module = (char*)module_name; /* safe */

    /* If no module name is specified, try default naming convention for loading
     * the main module from the library */
    if (!module) {
        module = ecs_os_malloc(strlen(library_name) + strlen("Import") + 1);
        const char *ptr;
        char ch, *bptr = module;
        bool capitalize = true;
        for (ptr = library_name; (ch = *ptr); ptr ++) {
            if (ch == '.') {
                capitalize = true;
            } else {
                if (capitalize) {
                    *bptr = toupper(ch);
                    bptr ++;
                    capitalize = false;
                } else {
                    *bptr = tolower(ch);
                    bptr ++;
                }
            }
        }
        *bptr = '\0';
        strcat(bptr, "Import");
    }

    /* Find civetweb module & entry point */
    ecs_module_init_action_t action = (ecs_module_init_action_t)ut_load_proc(
            library_name, NULL, module);
    if (!action) {
        ecs_os_err("failed to load the %s module from library %s",
            module, library_name);
        ut_raise();
        return ECS_INVALID_ENTITY;
    }

    /* Do not free id, as it will be stored as the component identifier */
    return _ecs_import(world, action, module, flags, NULL, 0); 
#else
    ecs_os_err(
        "sorry, loading libraries is only possible if flecs is built with bake :(");
    return ECS_INVALID_ENTITY;
#endif
}
