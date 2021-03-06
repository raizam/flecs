# Flecs Manual

## Contents

- [Design goals](#design-goals)
  - [Portability](#portability)
  - [Reusability](#reusability)
  - [Clean interfaces](#clean-interfaces)
  - [Performance](#performance)
- [API design](#api-design)
  - [Naming conventions](#naming-conventions)
  - [Handles](#handles)
  - [Error handling](#error-handling)
  - [Memory management](#memory-management)
- [Good practices](#good-practices)
  - [Minimize usage of ecs_get, ecs_set](#minimize-the-usage-of-ecs_get-ecs_set)
  - [Never compare types with anything](#never-compare-entity-types-with-anything)
  - [Write logic in systems](#write-logic-in-systems)
  - [Organize code in modules](#organize-code-in-modules)
  - [Do not depend on systems in other modules](#do-not-depend-on-systems-in-other-modules)
  - [Expose features, not systems in modules](#expose-features-not-systems-in-modules)
  - [Use types where possible](#use-types-where-possible)
  - [Use declarative statements for static data](#use-declarative-statements-for-static-data)
  - [Create entities in bulk](#create-entities-in-bulk)
  - [Limit usage of ecs_lookup](#limit-usage-of-ecs_lookup)
  - [Use ecs_quit to signal that your application needs to exit](#use-ecs_quit-to-signal-that-your-application-needs-to-exit)
  - [Update components proportionally to delta_time](#update-components-proportionally-to-delta_time)
  - [Set a target FPS for applications](#set-a-target-fps-for-applications)
  - [Never store pointers to components](#never-store-pointers-to-components)
- [Entities](#entities)
  - [Entity lifecycle](#entity-lifecycle)
    - [Creating entities](#creating-entities)
    - [Create entities in bulk](#create-entities-in-bulk)
    - [Deleting entities](#deleting-entities)
  - [Containers](#containers)
    - [The EcsContainer tag](#the-ecscontainer-tag)
    - [Creating child entities](#creating-child-entities)
    - [Adopting entities](#adopting-entities)
    - [Orphaning entities](#orphaning-entities)
  - [Prefabs](#prefabs)
    - [Overriding prefab components](#overriding-prefab-components)
    - [Prefabs and types](#prefabs-and-types)
    - [Prefab variants](#prefab-variants)
    - [Nested prefabs](#nested-prefabs)    
- [Components and Types](#components-and-types)
  - [Owned components](#owned-components)
    - [Add components](#add-components)
    - [Remove components](#remove-components)
    - [Set components](#set-components)
  - [Shared components](#shared-components)
  - [Tags](#tags)
- [Systems](#systems)
   - [System queries](#system-queries)
     - [Column operators](#column-operators)
       - [OR operator](#or-operator)
       - [NOT operator](#not-operator)
       - [Optional operator](#optional-operator)
     - [Column source modifiers](#column-source-modifiers)
       - [SELF modifier](#id-modifier)
       - [ID modifier](#id-modifier)
       - [CONTAINER modifier](#container-modifier)
       - [CASCADE modifier](#cascade-modifier)
       - [SYSTEM modifier](#system-modifier)
       - [SINGLETON modifier](#singleton-modifier)
       - [ENTITY modifier](#entity-modifier)
   - [System API](#system-api)
     - [The ECS_COLUMN macro](#the-ecs_column-macro)
     - [The ECS_SHARED macro](#the-ecs_shared-macro)
     - [The ECS_COLUMN_COMPONENT macro](#the-ecs_column_component-macro)
     - [The ECS_COLUMN_ENTITY macro](#the-ecs_column_entity-macro)
     - [The ecs_field function](#the-ecs_field-function)
     - [The ECS_COLUMN_TEST and ECS_SHARED_TEST macro's](#the-ecs_column_test-and-ecs_shared_test-macros)
   - [System phases](#system-phases)
     - [The EcsOnLoad phase](#the-ecsonload-phase)
     - [The EcsPostLoad phase](#the-ecspostload-phase)
     - [The EcsPreUpdate phase](#the-ecspreupdate-phase)
     - [The EcsOnUpdate phase](#the-ecsonupdate-phase)
     - [The EcsOnValidate phase](#the-ecsonvalidate-phase)
     - [The EcsPostUpdate phase](#the-ecspostupdate-phase)
     - [The EcsPreStore phase](#the-ecsprestore-phase)
     - [The EcsOnStore phase](#the-ecsonstore-phase)
     - [System phases example](#system-phases-example)
   - [Reactive systems](#reactive-systems)
     - [EcsOnAdd event](#ecsonadd-event)
     - [EcsOnRemove event](#ecsonremove-event)
     - [EcsOnSet event](#ecsonset-event)
   - [Features](#features)
- [Staging](#staging)
  - [Staged vs. inline modifications](#staged-vs-inline-modifications)
  - [Staging and ecs_get_ptr](#staging-and-ecs_get_ptr)
  - [Overwriting the stage](#overwriting-the-stage)
  - [Staging and EcsOnAdd, EcsOnSet and EcsOnRemove](#staging-and-ecsonadd-ecsonset-and-ecsonremove)
  - [Staging and system phases](#staging-and-system-phases)
  - [Staging and threading](#staging-and-threading)
  - [Manually merging stages](#manually-merging-stages)
  - [Limitations of staging](#limitations-of-staging)
- [Modules](#modules)
  - [Importing modules](#importing-modules)
    - [Module content handles](#module-content-handles)
    - [Dynamic imports](#dynamic-imports)
  - [Creating modules](#creating-modules)
- [Operating system abstraction API](#operating-system-abstraction-api)

## Design Goals
Flecs is designed with the following goals in mind, in order of importance:

### Portability
Flecs is implemented in C99 which makes it easy to port to a variety of platforms and (legacy) compilers. To further improve portability, Flecs has no mandatory external dependencies. For certain optional features, like threading and running the web-based dashboard, Flecs relies on external libraries like pthreads (or equivalent), civetweb and bake.util, but Flecs can be easily used without them.

### Reusability
Flecs has been designed so that it is easy to package systems and components in a way that can be easily reused across applications. The module design allows applications to import modules with a single line of code, after which the imported components and systems can be immediately used. To facilitate this, Flecs has an architecture that imposes a well-defined order on systems, so that imported systems are always executed in the right order, regardless of in wich order they are imported.

To further improve reusability, Flecs allows for writing code that is agnostic to how data is stored by the framework. While applications may make decisions to optimize storage for a particular usage pattern, Flecs allows systems to be written in a way that works across different storage modes.

### Clean interfaces
Flecs aims to provide clear and simple interfaces, by staying close to the core principles of Entity Component Systems. Someone who has worked with Entity Component Systems before should find it easy to read and write code written with Flecs. Flecs promotes a declarative design, where applications only need to state their intent.

### Performance
Flecs has a design that is optimized for minimizing cache misses by loading only data in cache that is required by the application, while also storing data in arrays (SoA) to ensure that an application makes optimal usage of cache lines. In many cases, applications can access raw arrays directly, wich is as fast as iterating a native array in C and, if the code permits it, lets applications be compiled with Single Instruction, Multiple Data (SIMD) instructions.

Furthermore, Flecs automatically optimizes performance where it can, by removing systems from the critical path if they are unused. This further improves reusability of code, as it lets applications import modules of which only a subset of the systems is used, without increasing overhead of the framework.

## API design

### Naming conventions
The Flecs API adheres to a set of well-defined naming conventions, to make it easier to read and write Flecs code. The basic naming conventions are illustrated in this code example:

```c
// Component names ('Position') use CamelCase
typedef struct Position {
    float x;
    float y; // Component members ('y') use snake_case
} Position;

typedef struct Velocity {
    float x;
    float y;
} Velocity;

// System names ('Move') use CamelCase. Supporting API types use snake_case_t
void Move(ecs_rows_t *rows) {
    // API functions ('ecs_column') use snake_case
    Position *p = ecs_column(rows, Position, 1);
    Velocity *v = ecs_column(rows, Velocity, 2);
    
    for (int i = 0; i < rows->count; i ++) {
        p[i].x += v[i].x;
        p[i].y += v[i].y;
    }
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();
    
    // Declarative macro's use SCREAMING_SNAKE_CASE
    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    
    // Enumeration constants ('EcsOnUpdate') use CamelCase
    ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);
    
    // Entity names use CamelCase
    ECS_ENTITY(world, MyEntity, Position, Velocity);
    
    // Imperative macro's (function wrappers) use snake_case
    ecs_set(world, MyEntity, Position, {10, 20});
    
    return ecs_fini(world);
}
```

#### Handles
The Flecs API creates and uses handles (integers) to refer to entities, systems and components. Most of the times these handles are transparently created and used by the API, and most code can be written without explicitly being aware of how they are managed. However, in some cases the API may need to access the handles directly, in which case it is useful to know their conventions.

The Flecs API has entity handles (of type `ecs_entity_t`) and type handles (of type `ecs_type_t`). Entity handles are used to refer to a single entity. Systems and components (amongst others) obtain identifiers from the same id pool as entities, thus handles to systems and components are also of type `ecs_entity_t`. Types are identifiers that uniquely identify a set of entities (or systems, components). Types are commonly used to add/remove one or more components to/from an entity, or enable/disable one or more systems at once.

Type handles are automatically created by API macro's like `ECS_COMPONENT`, `ECS_TYPE` and `ECS_PREFAB`. To obtain a handle to a type, use the `ecs_type` macro and provide as argument the identifier of the component or entity. Entity handles in most cases have the same identifier that is provided to the macro. For example:

```c
ECS_TYPE(world, MyType, Position);
```

This statement makes the entity handle available as `MyType`. To access the type handle directly, use `ecs_type(MyType)`. There is one exception to this rule, which is components. Entity handles of components are prefixed, so that the names do not collide with the component type name. To obtain the entity handle of a component, use the `ecs_entity` function. For example:

```c
ECS_COMPONENT(world, Position);
```

This statement makes the entity handle available as `ecs_entity_of(Position)`, and the type handle as `ecs_type_of(Position)`, where `ecs_entity_of` and `ecs_type_of` again are the macro's that translate from the component type name to the respective entity and type handles. If one were to fully write out what the `ECS_COMPONENT` macro does, it would look like this (replace 'Type' with a C type):

```c
ecs_entity_t ecs_entity(Type) = ecs_new_component(world, "Type", sizeof(Type));
ecs_type_t ecs_type(Type) = ecs_type_from_entity(ecs_entity(Type));
```

The `ecs_type_from_entity` function is an API function that can obtain a type handle from any entity handle.

### Error handling
The API has been designed in a way where operations have no preconditions on the (ECS) state of the application. Instead, they only ensure that a post condition of an operation is fulfilled. In practice this means that an operation _cannot_ fail unless invalid input is provided (e.g. a `NULL` pointer as world parameter). Take for example this code example:

```c
ecs_add(world, e, Position);
ecs_add(world, e, Position);
```

The `ecs_add` function has no precondition on the entity not having the component. The only thing that matters is that _after_ the operation is invoked, the entity has the `Position` component, which for both invocations is the case, thus the API will not throw an error. Another example:

```c
ecs_delete(world, e);
ecs_delete(world, e);
```

The post condition of `ecs_delete` is that the provided entity is deleted. In both invocations this is the case, thus the second time the `ecs_delete` operation is invoked is not an error. Another, slightly more interesting example:

```c
ecs_delete(world, e);
ecs_add(world, e, Position);
```

This, perhaps surprisingly, also does not result in an error. The reason is that entities in Flecs are never really deleted, they are only _emptied_. A deleted entity in Flecs is equivalent to an empty entity. Thus the post condition of `ecs_delete` is actually that the entity is empty. Adding `Position` subsequently to `e` is no different than adding `Position` to an empty entity, which is also not an error.

This does not mean that the API cannot fail. It relies on mechanisms like memory allocation and thread creation amongst others which can fail. It is also possible that an application corrupts memory, or Flecs contains a bug, which can also result in errors. In any of these situations, Flecs is unable to fulfill the post condition of an operation **and will assert or abort**, resulting in the termination of the application. Let me repeat that:

**Flecs will terminate your application when it encounters an error**.

This is a very conscious decision: rather than relying on the application to check (or not check) the return code of an operation, and making a decision based on incomplete information, the API does the only sensible thing it can do, which is stop. Note that this will _never_ happen as a result of a regular operation, but is _always_ the result of Flecs being in a state from which it cannot (or does not know how to) recover. It should be noted that explicit checks (asserts) are disabled when Flecs is built in release mode.

As a result of this API design, application code can be written declaratively and without error handling. Furthermore, return values of functions can actually be used to return useful information, which allows for a clean API, and results in concise code.

### Memory Management
The Flecs API has been designed to be ultra-simple and safe when it comes to managing memory (pointers):

**The API _never_ assumes ownership of passed in pointers & _never_ requires freeing a returned pointer**. 

This approach makes it unlikely for Flecs applications to run into memory corruption issues as a result of API misuse. There is only one exception to this rule, which is the creation / deletion of the world:

```c
ecs_world_t *world = ecs_init();

ecs_fini(world);
```

This also means that the application is fully responsible for ensuring that if a component contains pointers, these pointers are kept valid, and are cleaned up. In some cases this is straightforward, if the memory outlives a component as is often the case with entity identifiers:

```c
ecs_set(world, e, EcsId, {"MyEntity"}); 
```

This sets the `EcsId` component on an entity which is used by Flecs to assign names to entities. The `"MyEntity"` string is a literal and will certainly outlive the lifespan of the component, as it is tied to the lifecycle of the process, therefore it is safe to assign it like this. It can subsequently be obtained with this function:

```c
const char *id = ecs_get_id(world, e);
```

This function returns the verbatim address that is stored in the `EcsId` component, and thus should not be freed.

If memory is tied to the lifecycle of a component, applications can use `OnAdd` and `OnRemove` components to initialize and free the memory when components are added/removed. This example shows how to create two systems for a dynamic buffer that automatically allocate/free the memory for the dynamic buffer when it is added to an entity:

```c
typedef ecs_vector_t *DynamicBuffer;

ecs_vector_params_t params = {.element_size = sizeof(int)};

void InitDynamicBuffer(ecs_rows_t *rows) {
    DynamicBuffer *data = ecs_column(rows, DynamicBuffer, 1);
    for (int i = rows->begin; i < rows->end; i ++) {
        data[i] = ecs_vector_new(&params, 0);
    }
}

void DeinitDynamicBuffer(ecs_rows_t *rows) {
    DynamicBuffer *data = ecs_column(rows, DynamicBuffer, 1);
    for (int i = rows->begin; i < rows->end; i ++) {
        ecs_vector_free(data[i]);
    }
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();
    
    ECS_COMPONENT(world, DynamicBuffer);
    ECS_SYSTEM(world, InitDynamicBuffer, EcsOnAdd, DynamicBuffer);
    ECS_SYSTEM(world, DeinitDynamicBuffer, EcsOnRemove, DynamicBuffer);
    
    // This adds DynamicBuffer, and invokes the InitDynamicBuffer system
    ecs_entity_t e = ecs_new(world, DynamicBuffer);
    
    // This removes DynamicBuffer, and invokes the DeinitDynamicBuffer system
    ecs_delete(world, e);
    
    return ecs_fini(world);
}
```

## Good Practices
Flecs is an Entity Component System, and it is important to realize that ECS is probably quite different from how you are used to write code. Thus when you are just getting started with Flecs, you may run into some unforeseen problems, and you may wonder more than once how something is supposed to work. Additionally, Flecs also has its own set of rules and mechanisms that require getting used to. This section is not a comprehensive guide into writing code "the ECS way", but intends to provide a few helpful tips to guide you on the way.

### Minimize the usage of ecs_get, ecs_set
An ECS framework is only as efficient as the way it is used, and the most inefficient way of accomplishing something in an ECS framework is by extensively using `ecs_get` and `ecs_set`. This always requires the framework to do lookups in the set of components the entity has, which is quite slow. It also is an indication that code relies to much on individual entities, whereas in ECS it is more common practice to write code that operates on entity collections. The preferred way to do this in Flecs is with [_systems_](#systems), which can access components directly, without requiring a lookup.

### Never compare entity types with anything
The [type](#types) (of type `ecs_type_t`) of an entity is a handle that uniquely identifies the components an entity has. Even though the API provides a function to get the type of a specific entity (`ecs_get_type`) it is big code smell to compare this type for anything other than for debugging. In ECS applications the type of an entity may change any moment, and directly comparing the entity type is almost guaranteed to break at some point. Instead, use `ecs_has` if you want to check whether an entity has a component (or set of components, by providing a type to `ecs_has`).

### Write logic in systems
If you find yourself writing lots of code in the main loop of your application that is not executed in a system, it could be a sign of code smell. Logic in ECS runs best when it is part of a system. A system ensures that your code has a clear interface, which makes it easy to reuse the system in other applications. Flecs adds additional benefits to using systems like being able to automatically or manually (remotely!) enable/disable them, and schedule them to run on different threads.

### Organize code in modules
For small applications it is fine to create a few systems in your main source file, but for larger projects you will want to organize your systems and components in modules. Flecs has a module system that lets you easily import systems and components that are defined in other files in your project, or even other libraries. Ideally, the main function of your application only consists of importing modules and the creation of entities.

### Do not depend on systems in other modules
Flecs has been designed to allow applications define logic in a way that can be easily reused across a wide range of projects. This however only works if a small but important set of guiding principles is followed. As a general rule of thumb, a system should never depend on a _system_ that is not in the same module. Modules may import other modules, but should never directly refer systems from that module. The reason is, that individual systems are often small parts of a bigger whole that implements a certain _capability_. While it is fine to rely on the module implementing that capability, systems should not rely on _how_ that capability is implemented.

If you find that your application has this need and you cannot work around it, this could be an indication of modules that have the wrong granularity, or missing information in components shared between the modules. 

### Expose features, not systems in modules
Modules need to expose handles to the things they implement, to let the user interact with the contents of a module. A module should however be designed in such a way that it only exposes things that will not break compatibility between the application and the module in the future. When a module implements components, this is often not a problem, as component definitions are unlikely to change. When a module implements systems, this is less straightforward.

Modules often organize and define systems in a way so that multiple systems achieve a single feature. Only in the case of simple features, will a feature map to a single system. The way that systems are mapped to features is implementation specific, and subject to change as the module evolves. Therefore, explicitly exposing system handles in a module is generally not a good idea.

Instead, a module can expose [features](#features) which allow a module to group the systems that belong to a certain feature. This way a module can expose a single feature (for example, `CollisionDetection`) and group all systems that belong to that feature (for example, `AddCollider`, `TestColliders`, `CleanCollisions`) under that feature. Features are unlikely to change, and thus this ensures that future versions of a module won't break compatibility with the applications that uses it.

### Use types where possible
The sooner you can let Flecs know what components you will be setting on an entity, the better. Flecs can add/remove multiple components to/from your entity in a single `ecs_add` or `ecs_remove` call with types (see `ECS_TYPE`), and this is much more efficient than calling these operations for each individual component. It is even more efficient to specify a type with `ecs_new`, as Flecs can take advantage of the knowledge that the entity to which the component is going to be added is empty.

### Use declarative statements for static data
Declarative statements in Flecs can be recognized by their capitalized names. Declarative statements define the "fabric" of your application. They declare components, systems, prefabs, types and can even define entities. They improve the readability of applications as someone only needs to quickly glance over the code to get a good impression of which components, entities and systems an application has. An example of a declarative piece of code is:

```c
ECS_COMPONENT(world, Position);
ECS_COMPONENT(world, Velocity);
ECS_SYSTEM(world, Move, EcsOnFrame, Position, Velocity);
ECS_ENTITY(world, Player, Position, Velocity);
```

You may find that certain things cannot be expressed through declarative statements yet, like setting component values on individual entities (you need `ecs_set` for that). These use cases represent areas in the API that we want to improve. Eventually, we would like applications to be able to define applications fully declaratively (except for the system implementations, of course!). 

### Create entities in bulk
It is much more efficient to [create entities in bulk](#create-entities-in-bulk) (using the `ecs_new_w_count` function) than it is to create entities individually. When entities are created in bulk, memory for N entities is reserved in one operation, which is much faster than repeatedly calling `ecs_new`. What can provide an even bigger performance boost is that when entities are created in bulk with an initial set of components, the `EcsOnAdd` handler for initializing those components is called with an array that contains the new entities vs. for each entity individually. If your application heavily relies on `EcsOnAdd` systems to initialize data, bulk creation is the way to go!

### Limit usage of ecs_lookup
You can use `ecs_lookup` to find entities, components and systems that are named (that have the `EcsId` component). This operation is however not cheap, and you will want to limit the amount of times you call it in the main loop, and preferably avoid it alltogether. A better alternative to `ecs_lookup` is to specify entities in your system expression with the `ID` modifier, like so:

```c
ECS_SYSTEM(world, MySystem, EcsOnUpdate, Position, ID.MyEntity);
```

This will lookup the entity in advance, instead of every time the system is invoked. Obtaining the entity from within the system can be done with the `ecs_column_entity` function.

### Use ecs_quit to signal that your application needs to exit
You can use `ecs_progress` to control the flow of your application, by running it in a while loop, and making the result of the function the condition of the while loop. This will keep your application running until you call `ecs_quit`. Using this pattern provides a common approach to signalling your application needs to exit across modules.

### Update components proportionally to delta_time
The Flecs API provides your systems with a `delta_time` variable in the `ecs_rows_t` type wich contains the time passed since the previous frame. This lets you update your entity values proportional to the time that has passed, and is a good idea when you want to decouple the speed at which your logic is running from the FPS of your appplication. You can let Flecs determine `delta_time` automatically, by specifying `0` to `ecs_progress`, or manually by providing a non-zero value to `ecs_progress`.

### Set a target FPS for applications
When you run `ecs_progress` in your main loop, you rarely want to run your application as fast as possible. It is good practice to set a target FPS (a good default is 60) so that your application does not consume all of your CPU bandwidth. When you set a target FPS with the `ecs_set_target_fps` function, the `ecs_progress` function will automatically insert sleeps to make sure your application runs at the specified FPS. It may run slower if not enough CPU bandwidth is available, but it will never run faster than that.

### Never store pointers to components
In ECS frameworks, adding, removing, creating or deleting entities may cause memory to move around. This is it is not safe to store pointers to component values. Functions like `ecs_get_ptr` return a pointer which is guaranteed to remain valid until one of the aforementioned operations happens.

## Entities
Entities are the most important API primitive in any ECS framework, and even more so in Flecs. Entities by themselves are nothing special, just a number that identifies a specific "thing" in your application. What makes entities useful, is that they can be composed out of multiple _components_, which are data types describing the various _aspects_ or capabilities of an entity.

In Flecs, entities take on an even more prominent role, as Flecs internally uses entities to store many of the framework primitives, like components and systems. How this works exactly is an advanced topic, and the only reason it is mentioned here is so that you do not get confused when you see a signature like this:

```c
ecs_set(ecs_world_t *world, ecs_entity_t entity, ecs_entity_t component);
```

The "component" is, unexpectedly, of the `ecs_entity_t` type, and this looks weird until you realize that a component _handle_ is actually an entity with _builtin components_. That is all you need to know about this, unless you want to contribute to Flecs yourself (which is encouraged :-).

The next sections describe how applications can use Flecs to work with entities.

### Entity lifecycle
Entities in Flecs do not have an explicit lifecycle. This may seem confusing when you come from an OOP background, or perhaps even if you worked with other ECS Frameworks. An entity in Flecs is just an integer, nothing more. You can pick any integer you want as your entity identifier, and start assigning components to it. You cannot "create" or "delete" an entity, in the same way you cannot create or delete the number 10. That means in code, you can run this line by itself and it works:

```c
ecs_add(world, 10, Position);
```

This design emphasises the data-oriented nature of an ECS framework. Whereas in object oriented programming code is quite literally built to work with _individial_ objects, in ECS code is built around _a collection_ of components. In Flecs entities are an orthogonally designed feature that is only there to associate components.

Having said that the API does provide `ecs_new` and `ecs_delete` operations, which on the surface seem to imply entity lifecycle. The next sections will describe these operations in more detail, and explain how it fits with the above.

#### Creating entities
In Flecs, the `ecs_new` operation returns an entity handle that is guaranteed to not have been returned before by `ecs_new`. Note how this decsription explicitly avoids terminology such as "create". In Flecs, the total entity addressing space (which is 64 bit) is "alive" at all times. The `ecs_new` operation is merely a utility to obtain unused entity handles in a convenient way. The simplest way to invoke `ecs_new` is like this:

```c
ecs_entity_t e = ecs_new(world, 0);
```

This returns a handle to an empty entity. It is possible to provide an initial component, or type to `ecs_new`.  This is in many ways equivalent to first calling `ecs_new`, followed by `ecs_add`, but has the added advantage that since the API knows the entity is empty, there is no need to lookup the existing set of components, which has better performance. To specify a component to `ecs_new`, do:

```c
ecs_entity_t e = ecs_new(world, Position);
```

As it is functionally equivalent to first calling `ecs_new` followed by `ecs_add`, `EcsOnAdd` systems will be invoked for the `Position` component as a result of this operation.

#### Creating entities in bulk
When creating a large number of new entities, it can be much faster do this them in bulk, especially when adding initial components. This can be achieved with the `ecs_new_w_count` function. This function is equivalent to `ecs_new` but it has an additional `count` parameter which indicates the number of entities to be returned. An application can use it like this:

```c
ecs_entity_t e = ecs_new_w_count(world, Position, 100);
```

This operation is functionally equivalent to calling `ecs_new` 100 times, but has the added benefit that all the resource allocations, updating of the internal administration and invoking of reactive systems can all happen in bulk. The function returns the first of the created entities, and it is guaranteed that the entity identifiers are consecutive, so that if the first entity identifier is 1, the last entity identifier is 101.

#### Deleting entities
The `ecs_delete` operation does not actually delete the entity identifier (as this is impossible) but it guarantees that after the operation, no more resources are being held by flecs that are associated to the entity. After invoking the `ecs_delete` operation, the entity will have no more components, and will not be stored in any internal data structures. An application can use it like this:

```c
ecs_delete(world, e);
```

Since the entity identifier itself is not invalidated after the `ecs_delete`, it is legal to continue using it, like this:

```c
ecs_delete(world, e); // empty entity
ecs_add(world, e, Position); // add Position to empty entity
```

### Containers
Flecs allows applications to create entities that are organized in hierarchies, or more accurately, directed acyclic graphs (DAG). This feature can be used, for example, when transforming entity coordinates to world space. An application can create a parent entity with child entities that specify their position relative to their parent. When transforming coordinates to world space, the transformation matrix can then be cascadingly applied from parents to children, according to the hierarchy.

This is just one application of using entity hierarchies. The `flecs.components.http` and `flecs.systems.civetweb` modules both rely on the usage of parent-child relationships to express a web server (parent) with endpoints (children). Other APIs that feature hierarchical designs can benefit from this feature, like DDS (with `DomainParticipant` -> `Publisher` -> `DataWriter`) or UI development (`Window` -> `Group` -> `Button` -> `Label`). 

The container API in Flecs is fully composed out of existing (public) API functionality. The API operations related to containers are intended to codify a pattern that provides a consistent way to implement parent-child relationships across applications. This approach has as side effect that container/contained entities behave no different from other entities, aside from the fact that they _can_ be accessed in a different way if the application so desires.

The next sections describe how to work with containers.

#### The EcsContainer tag
In Flecs, containers are marked with the `EcsContainer` tag. This is a builtin tag that provides a quick way to test whether an entity can have child entities or not. The `EcsContainer` tag is automatically added to an entity when it _adopts_ another entity. If an application wants to know whether an entity is a container, it can simply do:

```c
if (ecs_has(world, e, EcsContainer)) {
    // ...
}
```

Flecs does not treat entities with the `EcsContainer` component differently from other entities. The only reason the `EcsContainer` tag is added by the API, is to provide a mechanism to discriminate between containers and non-containers. This prevents applications from having to assume that _every_ entity is a container, which could result in unnecessary operations that attempt to iterate over a set of children that does not exist.

The `EcsContainer` component can be used in system queries to obtain various subsets of entities that are useful when working with containers. For example, the following system iterates over all container entities:

```c
ECS_SYSTEM(world, IterateContainers, EcsOnUpdate, EcsContainer);
```

A more common example is to find all the root entities. This is often the first step for applications that want to cascadingly iterate over entities. A system that only wants to iterate over roots can use this query:

```c
ECS_SYSTEM(world, IterateRoots, EcsOnUpdate, !CONTAINER.EcsContainer);
```

To only iterate over entities that are children of a specific entity, an application can create a manual system and provide the container as a filter to the function. For example, it can define the following system:

```c
ECS_SYSTEM(world, IterateChildren, EcsManual, CONTAINER.EcsContainer);
```

Now an application can only iterate over the children for a specific container by using the container as filter:

```c
ecs_run_w_filter(
  world, 
  IterateChildren, // System handle
  delta_time,      // delta time
  0,               // Offset
  0,               // Limit
  my_container,    // Filter (ecs_type_t)
  NULL             // Parameter (void*)
);
```

Note that the system query explicitly only accepts entities that have containers. While the filter would automatically filter out any entities that are not contained by the specified container entity, putting this in the query makes sure that the system only iterates over contained entities, while excluding root entities from the set of filtered entities in advance. This limits the number of entities (or rather, archetypes) to iterate over when filtering which can improve performance, especially if the operation is executed many times per iteration.

#### Creating child entities
Flecs offers an API to create a new entity which also specifies a parent entity. The API can be invoked like this:

```c
ecs_entity_t my_root = ecs_new(world, 0);
ecs_entity_t my_child = ecs_new_child(world, my_root, 0);
```

Any entity can be specifed as a parent entity (`my_root`, in this example). After the `ecs_new_child` operation the `my_root` entity will have the `EcsContainer` tag, so this statement:

```c
ecs_has(world, my_root, EcsContainer);
```

will return true.

#### Adopting entities
The API allows applications to adopt entities by containers after they have been created with the `ecs_adopt` operation. The `ecs_adopt` operation is almost equivalent to an `ecs_add`, with as only difference that it accepts an `ecs_entity_t` (instead of an `ecs_type_t`), and it adds the `EcsContainer` component to the parent if it didn't have it already. The operation can be used like this:

```c
ecs_entity_t my_root = ecs_new(world, 0);
ecs_entity_t e = ecs_new(world, 0);
ecs_adopt(world, e, my_root);
```

After this operation, the `my_root` entity will have the `EcsContainer` tag. If the entity was already a child of the container, the operation has no side effects.

#### Orphaning entities
The API allows applications to orphan entities from containers after they have been created with the `ecs_orphan` operation. The `ecs_orphan` operation is almost equivalent to an `ecs_remove`, with as only difference that it accepts an `ecs_entity_t` (instead of an `ecs_type_t`). The operation can be used like this:

```c
ecs_orphan(world, e, my_root);
```

If the entity was not a child of the container, the operation has no side effects. This operation will not add the `EcsContainer` tag to `my_root`.

### Prefabs
A prefab is a special kind of entity in Flecs whos components are shared with any entity to which the prefab is added. Consider the following code example:

```c
ECS_PREFAB(world, Shape, Square, Color);

ecs_entity_t e1 = ecs_new(world, Position);
ecs_add(world, e1, Shape);

ecs_entity_t e2 = ecs_new(world, Shape);
```

The result of this code will be an entity `e1` which has one owned component (`Position`) and two shared components (`Square`, `Color`), and an entity `e2` which only has two shared components (`Square`, `Color`). Both entities will share `Square`, `Color`, as the prefab has been added to both. Thus changing the `Color` for one entity, will change the color for the other one as well.

In the API this is mostly transparent. Consider this example:

```c
Color *color1 = ecs_get_ptr(world, e1, Color);
Color *color2 = ecs_get_ptr(world, e2, Color);
```

Here, since both `e1` and `e2` share the color from `Shape`, the pointers `color1` and `color2` will point to the same memory. Modifying either values pointed to by them will change the value of the `Shape` entity as well.

A prefab can be used both as a component (as shown in the first example) and as an entity. A prefab is mostly just a regular entity, which means that all the normal API calls like `ecs_add` and `ecs_remove` will work on a prefab as well, as is demonstrated by this example:

```c
ecs_add(world, Shape, Size);
```

After this operation, the `Shape` prefab will have an additional component called `Size`, and all of the entities that added the prefab will now also have `Size` as a shared component.

Systems will treat shared components from prefabs as if they were defined on the entities themselves, thus a system with the query `Square, Color` would match both entities `e1` and `e2` from the example. It should be noted at this point that the system will _not_ match with the `Shape` prefab itself. Prefabs are explicitly excluded from matching with systems, as they should not be evaluated by application logic directly.

#### Overriding prefab components
A typical usage of prefabs is to specify a common value for entities. Often, code that uses prefabs looks something like this:

```c
ECS_PREFAB(world, Shape, Square, Color);
ecs_set(world, Shape, Square, {.size = 50});
ecs_set(world, Shape, Color, {.r = 255, .g = 0, .b = 0});

ecs_entity_t e = ecs_new(world, Shape);
```

However, in some scenarios an entity may want to change the value of one of its shared components, without affecting the others. This is possible by _overriding_ the shared component. To override a component, an application can simply use the `ecs_add` operation:

```c
ecs_add(world, e, Color);
```

After the operation, the component will have turned into an owned component, and the entity can update its value without affecting the value of the `Shape` prefab (and therefore, of the entities that also share the components from `Shape`). Additionally, the _component value_ of the new owned `Color` component will have been initialized with the value of the shared component. This is an important property that enables many interesting design patterns, while also ensuring that a component value does not suddenly go from initialized (shared) to uninitialized (owned).

Consequently, an entity can un-override a component by removing the component with the `ecs_remove` operation:

```c
ecs_remove(world, e, Color);
```

After this operation, the entity will have reverted to using the shared component from the `Shape` prefab. To remove the shared components entirely, the entity can remove the prefab with the `ecs_remove` operation:

```c
ecs_remove(world, e, Shape);
```

After this operation, the shared components will no longer appear on the entity.

#### Prefabs and types
It has been explained how [types](#components-and-types) can be used to create templates for entities, by adding sets of components commonly used together to the same type. When the type is added to the entity, all the components will be added as owned components to the entity. A quick example to recap:

```c
ECS_TYPE(world, Shape, Square, Color);

ecs_entity_t e = ecs_new(world, Shape);
```

A limitation of types is that they cannot contain component values, thus after a type has been added to an entity, the value of the component is still uninitialized. With a prefab, it is possible to also define a set of commonly used components that can be added to an entity, and it _is_ possible to define component values, however with prefabs the components are not owned by the entity. It would be nice if there was a middle-ground, where it was both possible to add owned components, that are also initialized.

It turns out this is achievable by combining the type and prefab features. A type can not only contain components, it can also contain prefabs. By adding both a prefab _and_ the components of a prefab to the same type, using the type will automatically override the components of the prefab, which will initialize them with the prefab values. Getting dizzy? Here is a code example:

```c
ECS_PREFAB(world, ShapePrefab, Square, Color);
ECS_TYPE(world, Shape, ShapePrefab, Square, Color);

ecs_set(world, ShapePrefab, Square, {.size = 50});
ecs_set(world, ShapePrefab, Color, {.r = 255, .g = 0, .b = 0});

ecs_entity_t e = ecs_new(world, Shape);
```

Lets breakdown what happens in this code. First a prefab called `ShapePrefab` is defined as usual with the `Square` and `Color` components. Then, a type called `Shape` is defined with _both_ the `ShapePrefab` _and_ the `Square` and `Color` components. After that, values are assigned to the prefab components, and the type is added to the entity.

When the `Shape` type is added to the entity, first the prefab is added (this is guaranteed, regardless of the order in which the prefab is added to the type). Then, the components from the type are added to the entity. As `Square` and `Color` are already defined as shared components on the entity, adding them again will _override_ the components, just like when they would be overridden when using the `ecs_add` operation. Because overriding a component copies the value from the shared component to the owned component, the new owned components will be initialized with the value from the prefab.

This is a powerful pattern for creating reusable entity templates that result in automatically initialized components, and is one of the preferred ways of instantiating entities.

#### Prefabs variants
Prefab variants are useful when you want to take an existing prefab, and add more components, or change some of its component values. A prefab variant inherits components and values from another prefab, called the base. A prefab can at most have one other prefab as its base. To specify a base, an application can simply specify it in the list of components of the prefab:

```c
ECS_PREFAB(world, Car, Velocity, MaxSpeed);
ecs_set(world, Car, Velocity, {160});

ECS_PREFAB(world, FastCar, Car, Velocity, MaxSpeed); // Car is the base prefab
ecs_set(world, FastCar, MaxSpeed, {240}); // Override MaxSpeed
```

#### Nested prefabs
Prefabs can be created as children of other prefabs. This lets applications create prefab hierarchies that can be instantiated by creating an entity with the top-level prefab. To create a prefab hierarchy, applications must explicitly set the value of the builtin `EcsPrefab` component:

```c
ECS_PREFAB(world, ParentPrefab, EcsPosition2D);
  ECS_PREFAB(world, ChildPrefab, EcsPosition2D);
     ecs_set(world, ChildPrefab, EcsPrefab, {.parent = ParentPrefab);
     
ecs_entity_t e = ecs_new(world, ParentPrefab);
```

After running this example, entity `e` will contain a child entity called `ChildPrefab`. All components of `e` will be shared with `ParentPrefab`, and all components of `e`'s child will be shared with `ChildPrefab`. Just like with regular prefabs, component values can be overridden. To override the component of a child entity, an application can use the following method:

```c
ecs_entity_t child = ecs_lookup_child(world, e, "ChildPrefab"); // Resolve the child entity created by the prefab instantiation
ecs_set(world, child, Position, {10, 20}); // Override the component of the child
```

An application can also choose to instantiate a child prefab directly:

```c
ecs_entity_t e = ecs_new(world, ChildPrefab);
```

Applications may want to compose a prefab out of various existing prefabs. This can be achieved by combining nested prefabs with prefab variants, as is shown in the following example:

```c
// Prefab that defines a wheel
ECS_PREFAB(world, Wheel, EcsPosition2D, EcsCircle2D);
  ECS_PREFAB(world, Tire, EcsPosition2D, EcsCircle2D, Pressure);

// Prefab that defines a car
ECS_PREFAB(world, Car, EcsPosition2D);
  ECS_PREFAB(world, FrontWheel, Wheel);
     ecs_set(world, FrontWheel, EcsPrefab, {.parent = Car});
     ecs_set(world, FrontWheel, EcsPosition, {-100, 0});
  ECS_PREFAB(world, BackWheel, Wheel);
     ecs_set(world, BackWheel, EcsPrefab, {.parent = Car});
     ecs_set(world, BackWheel, EcsPosition, {100, 0});     
```

In this example, the `FrontWheel` and `BackWheel` prefabs of the car inherit from `Wheel`, which is an effective mechanism to compose a prefab out of one or more existing prefabs, as it reuses the components of the prefab base, while also allowing for the possibility to override component values where necessary. Children of the base (`Wheel`) are also automatically inherited by the variants (`FrontWheel`, `BackWheel`), thus for every entity that is created with `Car`, 4 additional child entities will be created:

```c
ecs_entity_t e = ecs_new(world, Car); // Creates FrontWheel, FrontWheel/Tire, BackWheel, BackWheel/Tire
```

## Components and Types
Components, together with entities, are the most important building blocks for applications in Flecs. Like entities, components are orthogonally designed to do a single thing, which is to store data associated with an entity. Components, as with any ECS framework, do not contain any logic, and can be added or removed at any point in the application. A component can be registered like this:

```c
ECS_COMPONENT(world, Position);
```

Here, `Position` has to be an existing type in your application. You can register the same type twice with a typedef. If you have the following types:

```c
struct Vector {
    float x;
    float y;
};

typedef struct Vector Position;
typedef struct Vector Velocity;
```

they can be simply registered like this:

```c
ECS_COMPONENT(world, Position);
ECS_COMPONENT(world, Velocity);
```

With the API, you can even register the same type twice:

```c
ecs_new_component(world, "Position", sizeof(struct Vector));
ecs_new_component(world, "Velocity", sizeof(struct Vector));
```

A _type_ in Flecs is any set of 1..N components. If an entity has components `A` and `B`, then the _type_ of that entity is `A, B`. Types in Flecs are a first class citizen, in that they are just as prominently featured in the API as components, and in many cases they can be used interchangeably. An application can explicitly create a type, like this:

```c
ECS_TYPE(world, Movable, Position, Velocity);
```

This creates a type called `Movable` with the `Position` and `Velocity` components. Components have to be defined with either the `ECS_COMPONENT` macro or the `ecs_new_component` function before they can be used in an `ECS_TYPE` declaration.

Types handles be used interchangeably with components for most API operations. Using types instead of components can be useful for various reasons. A common application is to use a type to define an initial set of components for new entities. A type can be used together with `ecs_new` to accomplish this, like so:

```c
ecs_new(world, Movable);
```

The `ecs_new` operation actually accepts an argument of `ecs_type_t`, which means that even when a single component (like `Position`) is passed to the function, actually a _type_ is passed with only the `Position` component. 

This will create an entity with the `Position` and `Velocity` components, when using the previous definition of `Movable`. This approach is faster than individually adding the `Position` and `Velocity` components. Furthermore it allows applications to define reusable templates that can be managed in one location, as opposed to every location where entities are being created.

Types can also be nested. For example:

```c
ECS_TYPE(world, Movable, Position, Velocity);
ECS_TYPE(world, Unit, HealthPoints, Attack, Defense);
ECS_TYPE(world, Magic, Mana, ManaRecharge);
ECS_TYPE(world, Warrior, Movable, Unit);
ECS_TYPE(world, Wizard, Movable, Unit, Magic);
```

When added to entities, types themselves are not associated with the entity. Instead, only the resulting set of components is. For example, creating an entity with the `Wizard` type would be equivalent to creating an entity and individually adding `Position`, `Velocity`, `Attack`, `Defense`, `Mana` and `ManaRecharge`.

Types can be used with other operations as well, like `ecs_add`, `ecs_remove` and `ecs_has`. Whenever an application wants to add or remove multiple components, it is always faster to do this with a type as they approximate constant time performance (`O(1)`), whereas individually adding components is at least `O(n)`.

Types are not limited to grouping components. They can also group entities or systems. This is a key enabler for powerful features, like [prefabs](#prefabs), [containers](#containers) and [features](#features).

### Owned components
In Flecs, components can either be owned by an entity or shared with other entities. When a component is owned, it means that an entity has a private instance of the component that can be modified individually. Owned components are useful when a component is mutable, and individual entities require the component to have a unique value.

The following sections describe the API operations for working with owned components.

#### Add components
In Flecs, an application can add owned components to an entity with the `ecs_add` operation. The operation accepts an entity and a _type_ handle, and can be used like this:

```c
ecs_add(world, e, Position);
```

After the operation, it is guaranteed that `e` will have the component. It is legal to call `ecs_add` multiple times. When a component is already added, this operation will have no side effects. When the component is added as a result of this operation, any `EcsOnAdd` systems subscribed for `Position` will be invoked.

It is also possible to use types with `ecs_add`. If an application defined a type `Movable` with the `ECS_TYPE` macro, it can be used with `ecs_add` like so:

```c
ecs_add(world, e, Movable);
```

After the operation, it is guaranteed that `e` will have all components that are part of the type. If the entity already had a subset of the components in the type, only the difference is added. If the entity already had all components in the type, this operation will have no side effects.

##### A quick recap on handles
The signature of `ecs_add` looks like this:

```c
void ecs_add(ecs_world_t *world, ecs_entity_t entity, ecs_type_t type);
```

Note that the function accepts a _type handle_ as its 3rd argument. Type handles are automatically defined by the API when an application uses the `ECS_COMPONENT`, `ECS_ENTITY`, `ECS_PREFAB` or `ECS_TYPE` macro's. When a component is defined with the `ECS_COMPONENT` macro a type handle is generated (or looked up, if it already existed) just with that one component. If the set of added components matches any `EcsOnAdd` systems, they will be invoked.

For more details on how handles work, see [Handles](#handles).

#### Remove components
Flecs allows applications to remove owned components with the `ecs_remove` operation. The operation accepts an entity and a _type_ handle, and can be used like this:

```c
ecs_remove(world, e, Position);
```

After the operation, it is guaranteed that `e` will not have the component. It is legal to call `ecs_remove` multiple times. When the component was already removed, this operation will have no side effects. When the component is removed as a result of this operation, any `EcsOnRemove` systems that match with the `Position` component will be invoked.

It is also possible to use types with `ecs_remove`. If an application defined a type `Movable` with the `ECS_TYPE` macro, it can be used with `ecs_remove` like so:

```c
ecs_remove(world, e, Movable);
```

After the operation, it is guaranteed that `e` will not have any of the components that are part of the type. If the entity only had a subset of the types, that subset is removed. If the entity had none of the components in the type, this operation will have no side effects. If the set of components that was part of this operation matched any `EcsOnRemove` systems, they will be invoked.

#### Set components
With the `ecs_set` operation, Flecs applications are able to set a component on an entity to a specific value. Other than the `ecs_add` and `ecs_remove` operations, `ecs_set` accepts a `_component` (entity) handle, as it is only possible to set a single component at the same time. The `ecs_set` operation can be used like this:

```c
ecs_set(world, e, Position, {10, 20});
```

After the operation it is guaranteed that `e` has `Position`, and that it is set to `{10, 20}`. If the entity did not yet have `Position`, it will be added by the operation. If the entity already had `Position`, it will only assign the value. If there are any `EcsOnSet` systems that match with the `Position` component, they will be invoked after the value is assigned.

### Shared components
Shared components in Flecs are components that are shared between multiple entities. Where owned components have a 1..1 relationship between a component and an entity, a shared component has a 1..N relationship between the component and entity. Shared components are only stored once in memory, which can drastically reduce memory usage of an application if the same component can be shared across many entities. Additionally, shared components are a fast way to update a component value in constant time (O(1)) for N entities.

Shared components can be implemented with [prefabs](#prefabs).

### Tags
Tags are components that do not contain any data. Internally it is represented as a component with data-size 0. Tags can be useful for subdividing entities into categories, without adding any data. A tag can be defined with the ECS_TAG macro:

```c
ECS_TAG(world, MyTag);
```

Tags can be added/removed just like regular components with `ecs_new`, `ecs_add` and `ecs_remove`:

```c
ecs_entity_t e = ecs_new(world, MyTag);
ecs_remove(world, e, MyTag);
```

A system can subscribe for instances of that tag by adding the tag to the system signature:

```c
ECS_SYSTEM(world, Foo, EcsOnUpdate, Position, MyTag);
```

This introduces an additional column to the system which has no data associated with it, but systems can still access the tag handle with the `ECS_COLUMN_COMPONENT` marco:

```c
void Foo(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, 1);
    ECS_COLUMN_COMPONENT(rows, MyTag, 2);
    
    for (int i = 0; i < rows->count; i ++) {
        ecs_remove(rows->world, rows->entities[i], MyTag);
    }
}
```

## Systems
Systems let applications execute logic on a set of entities that matches a certain component expression. The matching process is continuous, when new entities (or rather, new _entity types_) are created, systems will be automatically matched with those. Systems can be ran by Flecs as part of the built-in frame loop, or by invoking them individually using the Flecs API.

### System queries
A system query specifies which components the system is interested in. By default, it will match with entities that have all of the specified components in its expression. An example of a valid system query is:

```
Position, Velocity
```

The two elements are the components the system is interested in. Within a query they are called "columns", and they can be thought of as elements in the `SELECT` clause of an SQL query. The order in which components are specified is important, as the system implementation will need to access component in this exact order. Care must be taken that when changing the order of columns, the implementation is updated to reflect this. More on this in "System API".

The system query is the only mechanism for specifying the input for the system. Any information that the system needs to run must therefore be captured in the system query. This strict enforcement of the interface can sometimes feel like a constraint, but it makes it possible to reuse systems across different applications. As you will see, system queries have a number of features that make it easier to specify a range of possible input parameters.

#### Column operators
System queries may contain operators to express optionality or exclusion of components. The most common one is the `,` (comma) which is equivalent to an AND operator. Only if an entity satisfies each of the expressions separated by the `,`, it will be matched with the system. In addition to the `,` operator, queries may contain a number of other operators:

##### OR operator
The OR operator (`|`) allows the system to match with one component in a list of components. An example of a valid query with an OR operator is:

```
Position, Velocity | Rotation
```

Inside of the system implementation, an application has the possibility to determine which component in the OR expression was the one that caused the system to match. An OR expression may contain any number of components.

##### NOT operator
The NOT operator (`!`) allows the system to exclude entities that have a certain component. An example of a valid query with a NOT operator is:

```
Position, !Velocity
```

Inside the system implementation an application will be able to obtain metadata for the column (it will be able to see the component type for `Velocity`), but no actual data will be associated with it.

##### Optional operator
The optional operator (`?`) allows a system to optionally match with a component. An example of a valid query with an optional operator is:

```
Position, ?Velocity
```

Inside the system implementation, an application will be able to check whether the component was available or not.

#### Column source modifiers
System queries can request components from a variety of sources. The most common and default source is from an entity. When a system specifies `Position, Velocity` as its query, it will match _entities_ that have `Position, Velocity`. A system may however require components from other entities than the one being iterated over. To streamline this use case, reduce `ecs_get` API calls within systems, and prevent excessive checking on whether components are available on external entities, the system query can capture these requirements. A query may contain the folllowing modifiers:

##### SELF modifier
This is the default modifier, and is implied when no modifiers are explicitly specified. An example of the `SELF` modifier is:

```
Position, Velocity
```

This system will match with any entities that have the `Position, Velocity` components. The components will be available to the system as owned (non-shared) columns, _except_ when a component is provided by a Prefab, in which case the component will be shared.

##### ID modifier
The `ID` modifier lets an application pass handles to components or other systems to a system. This is frequently useful, as systems may need component handles to add or set components on entities that may not be part of the entity yet. Another use case for this feature is passing a handle to another `EcsManual` system to the system, which the system can then execute. This is frequently used when a system needs to evaluate a set of entities for every matching entity. An example of the `ID` modifier is:

```
Position, ID.Velocity
```

This will match any entity that has the `Position` component, and pass the handle to the `Velocity` component to the system. This allows the system implementation to add or set the `Velocity` component on the matching entities.

`ID` columns have no data, and as such should not be accessed as owned or shared columns. Instead, the system should only attempt to obtain the handle to the component or component type.

##### CONTAINER modifier
The `CONTAINER` modifier allows a system to select a component from the entity that contains the currently iterated over entity. An example of the `CONTAINER` modifier is:

```
CONTAINER.Position, Position, Velocity
```

This will match all entities that have `Position, Velocity`, _and_ that have a container (parent) entity that has the `Position` component. This facilitates building systems that must traverse entities in a hierarchical manner.

`CONTAINER` columns are available to the system as a shared component.

##### CASCADE modifier
The `CASCADE` modifier is similar to an optional `CONTAINER` column, but in addition it ensures that entities are iterated over in the order of the container hierarchy. 

For a hierarchy like this:

```
   A
  / \
 B   C
    / \
   D   E 
```

the order in which entities will be visited by the system will be `A, B, C, D, E`. Note how the system also matches the root entities (`A`) that do not have a container (hence "optional"). An example of a `CASCADE` modifier is:

```
CASCADE.Position, Position, Velocity
```

The order will be determined by the container that has the specified component (`Position` in the example). Containers of the entity that do not have this component will be ignored. 

`CASCADE` columns are available to the system as an optional shared component.

##### SINGLETON modifier
The `SINGLETON` or `$` modifier selects components from the singleton entity. As the name suggests, this allows a system to access a single, global (but world-specific) instance of a component. An example of the `SINGLETON` modifier is:

```
$Position, Position, Velocity
```

This will match all entities that have `Position, Velocity`, and make the `Position` component from the singleton entity available to the system.

`SINGLETON` columns are available to the system as a shared component.

##### SYSTEM modifier
In some cases it is useful to have stateful systems that either track progress in some way, or contain information pointing to an external source of data (like a database connection). The `SYSTEM` modifier allows for an easy way to access data associated with the system. An example of the `SYSTEM` modifier is:

```
SYSTEM.DbConnection, Position, Velocity
```

This will match all entities with `Position, Velocity`, and automatically add the `DbConnection` component to the system. Often this pattern is paired with an `EcsOnAdd` system for the `DbConnection` component, which would be immediately executed when the system is defined, and set up the database connection (or other functionality) accordingly.

`SYSTEM` columns are available to the system as a shared component.

##### ENTITY modifier
In some cases, it is useful to get a component from a specific entity. In this case, the source modifier can specify the name of a named entity (that has the `EcsId` component) to obtain a component from that entity. An example of the `ENTITY` modifier is:

```
Position, SomeEntity.Velocity
```

This will match all antities with the `Position` component, and pass the `Velocity` component of `SomeEntity` to the system. The equivalent of this functionality would be to pass handles to `SomeEntity` and `Velocity` with the `ID` component, and then do an `ecs_get` from within the system, like so:

```
Position, ID.SomeEntity, ID.Velocity
```

Using the `ENTITY` modifier is however much less verbose, and can potentially also be optimized as the framework may use a more efficient version of `ecs_get`.

`ENTITY` columns are available to the system as a shared component.

### System API
Now that you now how to specify system queries, it is time to find out how to use the columns specified in a query in the system itself! First of all, lets take a look at the anatomy of a system. Suppose we define a system like this in our application `main`:

```
ECS_SYSTEM(world, Move, EcsOnUpdate, Position, Velocity);
```

How would this system actually be implemented? First of all, the application needs to define the function itself. The name of the function should exactly match the name in the definition, and should have the following signature:

```c
void Move(ecs_rows_t *rows) {
    for (int i = 0; i < rows->count; i ++) {
    }
}
```

The `rows` parameter provides access to the entities that matched with the system, and a lot of other useful information. This example already has the typical `for` loop that defines many system implementations, and the application can get the number of entities to iterate over from the `rows->count` member.

#### The ECS_COLUMN macro
To actually do something useful with the matched entities, the functino needs to obtain the components. In order to do this, the code needs to look at the system query, which in this case is `Position, Velocity`. This query has two columns, and the components can be accessed from the system by using the corresponding column index:

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN(rows, Velocity, velocity, 2);
    
    for (int i = 0; i < rows->count; i ++) {
    }
}
```

This macro requires the `rows` parameter, to get access to the matched entities, the type of the component (`Position`) a name for the variable which will point to the component array (`position`) and the column index (`1`). Note how the column index starts counting from `1`. This is because column index `0` is reserved for the column that stores the _entity identifiers_. More on that later.

Now the system logic can be written, by using the `position` and `velocity` variables. Because they point to arrays of the component types (`Position`, `Velocity`) the application can simply use them as C arrays, like so:

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN(rows, Velocity, velocity, 2);
    
    for (int i = 0; i < rows->count; i ++) {
        position[i].x += velocity[i].x;
        position[i].y += velocity[i[.y;
    }
}
```

The `ECS_COLUMN` macro provides the fastest, easiest access to components in a system. It can however not be used for any column. The next sections discuss some of the other macro's that can be used to obtain data from within a system.

#### The ECS_SHARED macro
When a component is not stored on a matched entity, it is shared. For a system this means that instead of having an array of component values, there is only a single component instance available for this iteration. Flecs has been designed in such a way that a shared component never changes within an invocation of a system, though a system may be invoked multiple times per run (more on that later). This means that a system can obtain a shared component once.

An example of a shared component is when using the `ENTITY` column source modifier. With this modifier, the application can pass a component from an entity that is not (necessarily) matched with the system, like so:

```
Position, Velocity, Player.Position
```

The `Player.Position` identifies the `Position` component on an entity named `Player`. This entity could have been created like so:

```c
ECS_ENTITY(world, Player, Position);
```

With the above system query, the `Position` component of `Player` will be passed to the system function. To access it, the system should not use the `ECS_COLUMN` macro (attempting this will result in an assert) as this suggests that the variable can be used as an array. Instead, the system should use the `ECS_SHARED` macro, which is invoked similar to `ECS_COLUMN`: 

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN(rows, Velocity, velocity, 2);
    ECS_SHARED(rows, Position, position_player, 3);
    
    for (int i = 0; i < rows->count; i ++) {
        position[i].x = position_player->x + velocity[i].x;
        position[i].y = position_player->x + velocity[i[.y;
    }
}
```

Note how the code uses the `position_player` variable as a pointer, and not as an array, as expected. Shared components are common in Flecs, and you will often find yourself writing systems using both `ECS_COLUMN` and `ECS_SHARED`. Familiarizing yourself with these concepts will go a long way in understanding how to write effective Flecs applications!

#### The ECS_COLUMN_COMPONENT macro
When a system needs a component handle to one of its components, the `ECS_COLUMN_COMPONENT` will declare a handle to the component in the system. This is useful when a system needs to use the Flecs API, like the `ecs_set` function. When the `ecs_set` function is called, it expects the handle for a component to be there. In the application `main` function this happens automatically when a component is defined with `ECS_COMPONENT` or when it is imported with `ECS_IMPORT`, but a system needs to do this itself.

To import a component handle in a system, an application can use the  `ECS_COLUMN_COMPONENT` macro, like so:

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN(rows, Velocity, velocity, 2);
    ECS_COLUMN_COMPONENT(rows, Position, 1);
    
    for (int i = 0; i < rows->count; i ++) {
      ecs_set(rows->world, rows->entities[i], Position, {
          .x = position[i].x + velocity[i].x,
          .y = position[i].y + velocity[i].y
      });
    }
}
```

This code may look a bit weird as it introduces a few things that haven't been covered yet. First of all, note how the `world` object is passed into the system through the `rows` parameter. This lets a system call Flecs API functions, as all functions at least require a reference to an `ecs_world_t` instance. Secondly, note how the system obtains the entity id of the currently iterated over entity with `rows->entities`. Finally, note how the `ecs_set` function is able to use the `Position` component. The function requires a handle to the `Position` component to be defined, or it will result in a compiler error (try removing the `ECS_COLUMN_COMPONENT` macro).

This macro can also be used when a column uses the `ID` source modifier. For example, if a system has the following query:

```
Position, ID.Velocity
```

Then the system can obtain the handle to the `Velocity` component with the following statement:

```c
ECS_COLUMN_COMPONENT(rows, Velocity, 2);
```

#### The ECS_COLUMN_ENTITY macro
Where the `ECS_COLUMN_COMPONENT` macro obtains a _type_ handle (of `ecs_type_t`), the `ECS_COLUMN_ENTITY` macro obtains an _entity_ handle (of type `ecs_entity_t`). This macro is useful when a system needs access to a specific entity, as is shown in this example:

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN(rows, Velocity, velocity, 2);
    ECS_COLUMN_COMPONENT(rows, Position, 1);
    ECS_COLUMN_ENTITY(rows, e, 3);
    
    for (int i = 0; i < rows->count; i ++) {
      if (ecs_has(rows->world, e, Position) {
          position[i].x += velocity[i].x;
          position[i].y += velocity[i].y;
      }
    }
}
```

Aside from this being a highly contrived example, it demonstrates the difference in how entity handles and type handles are used. When unsure about when to use an entity handle or a type handle, refer to the API documentation.

#### The ecs_field function
In some cases, a system may not know whether a component is shared or not. This is particularly the case when a system is generic, and it does not know when an application uses prefabs. While using prefabs for certain components is uncommon (like `Position`), for other components this may be less obvious. In such cases, a system may choose to use the `ecs_field` function, which is a less performant alternative to `ECS_COLUMN` and `ECS_SHARED`, but with the benefit that it is agnostic to whether a component is owned or shared.

This is an example of how to use the `ecs_field` function:

```c
void Move(ecs_rows_t *rows) {    
    for (int i = 0; i < rows->count; i ++) {
        Position *p = ecs_field(rows, Position, i, 1);
        Velocity *v = ecs_field(rows, Velocity, i, 2);
        p->x += v->x;
        p->y += v->y;
    }
}
```

Note how this example does not use any macro's to import the components, but instead uses `ecs_field` to obtain a pointer to the component of the entity that is being iterated over directly. While `ecs_field` is quite fast, it is not as fast as iterating over an array, and a compiler will not be able to vectorize code that uses `ecs_field`.

#### The ECS_COLUMN_TEST and ECS_SHARED_TEST macro's
Both the `ECS_COLUMN` and `ECS_SHARED` macro's have an equivalent macro with the postfix `_TEST`. This macro should be used when a system is unsure about whether a component is available or not. This may be the case with optional columns. For example, when a system has this signature:

```
Position, ?Velocity
```

The components should be retrieved like this:

```c
void Move(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, position, 1);
    ECS_COLUMN_TEST(rows, Velocity, velocity, 2);
    
    for (int i = 0; i < rows->count; i ++) {
      if (velocity) {
          position[i].x += velocity[i].x;
          position[i].y += velocity[i].y;
      }
    }
}
```

Note how if the component is not available, the variable of the optional column (`velocity`) will be set to `NULL`, which can then be tested by the system implementation.

### System phases
Each frame in Flecs is computed in different phases, which define the execution order in which certain systems are executed. Phases are necessary to guarantee interoperability between systems in different modules, as it imposes a high-level order on the systems that need to be executed each frame. A simple example is a system that updates game state, and a system that renders a frame. The first system needs to be executed before the second one, to ensure the correct data is rendered. 

By using phases in the correct way, module developers can ensure that their systems work correctly across applications. There are no strictly enforced rules on how these phases must be used, and an application may decide to not use them at all. However, if an application or module defines logic that must be reused, phases are necessary to ensure systems run at the right moment.

The following sections describe the different phases in the order of execution, and what kind of systems are expected to run in them.

#### The EcsOnLoad phase
The `EcsOnLoad` phase is the first phase that is executed every frame. In this phase any data that needs to be loaded from external sources should be inserted into the ECS world (`ecs_world_t` instance). An example could be a system that streams data from disk based on the game state.

#### The EcsPostLoad phase
The `EcsPostLoad` phase "primes" the loaded data so that it is ready for usage. This often involves automatically adding components to entities that have a certain set of components. For example, a transformation module may automatically add a `TransformMatrix` component to each entity that has a `Position`, `Rotation` or `Scale`. 

#### The EcsPreUpdate phase
The `EcsPreUpdate` phase prepares the frame for the game logic. This phase typically contains systems that cleanup data from the previous frame, or initialize components for the next frame. An example could be a system that resets the transformation matrices to the identity matrix, or remove components that describe collisions detected in the previous frame.

#### The EcsOnUpdate phase
During the `EcsOnUpdate` phase the game logic is executed. In this phase entities are moved, AI is executed, input is processed and so on. Most of the game-specific systems are executed in this phase.

#### The EcsOnValidate phase
The `EcsOnValidate` phase checks if any constrains have been violated during the `EcsOnUpdate` phase. A typical example is an entity that moved out of bounds, or two entities that collided and are now partially overlapping. An example of a system that is executed during the `EcsOnValidate` phase is collision detection.

#### The EcsPostUpdate phase
The `EcsPostUpdate` phase evaluates constraints and corrects entities where required. For example, a system in the `EcsPostUpdate` phase may evaluate collisions detected during the `EcsOnValidate` phase, and correct entity positions so that they are no longer overlapping. Whereas the systems in the `EcsOnValidate` stage are typically from imported modules (like physics) the systems in the `EcsPostUpdate` phase are typically application specific (insert explosion at location of collision).

#### The EcsPreStore phase
The `EcsPreStore` phase allows imported modules a last chance to react to application logic before rendering a frame. For example, if an entity was moved during the `EcsPostUpdate` phase, its transformation matrix should be updated before it is rendered to reflect its latest position.

#### The EcsOnStore phase
During the `EcsOnStore` phase the data computed in the frame is rendered, or stored in an external data source. 

#### System phases example
This is an example of how a typical application that loads data dynamically, uses collision detection and rendering (identifiers between parenthesis are component expressions):

- **EcsOnLoad**
  - StreamFromDisk (`Game`, `Player`)

- **EcsPostLoad**
  - AddTransformMatrix (`Position | Rotation | Scale`, `!TransformMatrix`)
  - CleanCollisions (`Collision`)
  
- **EcsPreUpdate**
  - ResetTransformMatrix (`TransformMatrix`)
  
- **EcsOnUpdate**
  - Move (`Position`, `Velocity`)

- **EcsOnValidate**
  - Translate (`Position`, `TransformMatrix`)
  - Rotate (`Rotation`, `TransformMatrix`)
  - Scale (`Scale`, `TransformMatrix`)
  - TransformColliders (`Collider`, `TransformMatrix`)
  - TestForCollisions (`Collider`) -> `Collision`
  
- **EcsPostFrame**
  - OnCollision (`Collision`)
  
- **EcsPreStore**
  - CorrectTransform (`Collision`, `TransformMatrix`)
  
- **EcsOnStore**
  - Render

### Reactive systems
When a system is assigned to one of the various [system phases](#system-phases) systems are executed every frame (when `ecs_progress` is called), or periodically at a specified time interval. Alternatively, applications can define systems that are ran whenever a specific _event_ occurs. Events that can be intercepted are adding/removing components, and setting a value. The following sections describe these events.

#### EcsOnAdd
The `EcsOnAdd` event is triggered whenever a component is added to an entity. This can happen as a result of an `ecs_new` (when a component is specified), an `ecs_add` or an `ecs_set`. The `EcsOnAdd` event is typically used to build systems that initialize components, as flecs does not automatically initialize components itself.

Systems that respond to `EcsOnAdd` events are executed immediately after the component is added, before any of the aforementioned calls (`ecs_new`, `ecs_add`, `ecs_set`) return. This ensures that right after the component is added, it can be initialized and safe to use.

#### EcsOnRemove event
The `EcsOnRemove` event is triggered whenever a component is removed from an entity. This can happen as a result of an `ecs_remove` or an `ecs_delete` (which removes all components). The `EcsOnRemove` event is typically used to build systems that deinitialize components, which often involves releasing resources or freeing memory.

Systems that respond to `EcsOnRemove` events are normally executed immediately after the component is removed, before any of the aforementioned calls (`ecs_remove`, `ecs_delete`) return. The exception to this rule is when `ecs_remove` is invoked by a system, in which case the system is executed at the end of a phase, after all systems in that phase have been executed.

#### EcsOnSet event
The `EcsOnSet` event is triggered whenever a component is set to a value. This can happen as a result of an `ecs_set`. Systems responding to an `EcsOnSet` typically react to the data in a component, and require data to be initialized. During the lifecycle of a component, multiple `EcsOnSet` events may be generated, for every time the `ecs_set` function is called.

### Features
In many situations, a particular feature is realized by multiple systems. For example, a feature that does matrix transformations may have three systems, for automatically adding the `TransformMatrix` component, resetting it to the identity, and doing the actual transform. The exact details on how a feature is split up between systems highly depends on its implementation, and this makes it more complicated for applications to tell which systems are associated with a feature. An application may, for example, want to enable/disable certain features that it does not need.

To reduce this complexity, and to prevent applications from having to depend on specific implementation details, systems can be organized in features. Features combine multiple systems that fulfill a common goal under a single handle, which can be used to enable/disable all systems at the same time. A feature can be defined with the `ECS_TYPE` macro:

```c
ECS_TYPE(world, MyFeature, SystemA, SystemB);
```

An application can then enable/disable both `SystemA` and `SystemB` with a single call to `ecs_enable`:

```c
ecs_enable(world, MyFeature, false); // disables feature
```

Features can also be nested, like so:

```c
ECS_TYPE(world, ChildFeature1, Foo, Bar);
ECS_TYPE(world, ChildFeature2, Hello, World);
ECS_TYPE(world, ParentFeature, ChildFeature1, ChildFeature2);
```

When designing modules it is good practice to not directly expose handles to systems, but instead only expose feature handles. This decreases the chance that an application has to be modified because the implementation of a module changed.

### Staging
ECS frameworks typically store data in contiguous arrays where systems iterate over these arrays, and Flecs is no exception. A challenge for ECS frameworks is how to allow for mutations, such as adding/removing components and removing entities which alter the array. To address this, ECS frameworks often have limitations on which operations are allowed while iterating. A common solution to this problem is to implemement something called a _command buffer_, which stores a list of operations that are executed after an iteration.

Command buffers however have several disadvantages. First of all, mutations are not visible until the next iteration. When a system adds a component, and subsequently tests if the component has been added, the test would return false, which is not intuitive. Another disadvantage is that applications need a different API while iterating.

Flecs uses an alternative solution called "staging". Staging uses a data structure (called a "stage" or "staging area") which stores the _result_ of operations instead of the operations themselves. The data structure behaves like a "branch" of the main (not staged) data store, and can be queried by the flecs API in much the same way as the main data store can be queried. This allows the Flecs API to mutate data without limitations, while still having access to the mutated data while iterating. This capability is also a key enabling feature for [multithreading](#staging-and-multithreading).

The following code shows an example of a system that relies on staging:

```c
void System(ecs_rows_t *rows) {
    ECS_COLUMN_COMPONENT(rows, Velocity, 2);

    for(int i = 0; i < rows->count; i ++) {
        // Add component Velocity to stage, set its value to {1, 1}
        ecs_set(rows->world, rows->entities[i], Velocity, {1, 1});
        
        // Operation returns true
        ecs_has(rows->world, rows->entities[i], Velocity);
        
        // Operation returns pointer to the initialized Velocity
        ecs_get_ptr(rows->world, rows->entities[i], Velocity);
    }
}

int main(int argc, const char *argv[]) {
    ecs_world_t *world = ecs_init();
    
    ECS_SYSTEM(world, System, EcsOnUpdate, Position, !Velocity);
    
    // API calls done while in ecs_progress are staged
    ecs_progress(world, 1);
    // Stage is merged after executing systems
    
    return ecs_fini();
}
```

Note how you cannot tell from the API calls that staging is used. Flecs "knows" that the application is iterating over data, and when it is in this mode, all operations will automatically query and write to the stage.

#### Staged vs. inline modifications
When a system is iterating, it receives contiguous arrays with component data. These arrays are not staged, as they provide direct access to the components in the main data store. Applications can change the contents of these arrays, which is referred to as "inline modifications". An important decision system implementors have to make is whether to modify data inline, or whether to use staging. The following code example shows the difference between the two:

```c
void System(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, p, 1);
    ECS_COLUMN_COMPONENT(rows, Position, 1);
    ECS_COLUMN(rows, Velocity, v, 2);

    for(int i = 0; i < rows->count; i ++) {
        // Inline modification
        v[i].x *= 0.9;
        v[i].y *= 0.9;
        
        // Staged modification
        ecs_set(rows->world,  rows->entities[i], Position, {p[i].x + v[i].x, p[i].y + v[i].y});
    }
}
```

A staged modification, while much slower than an inline modification, has as advantage that the component is not changed until data is merged, which means that subsequent systems can still have access to the previous value. If a subsequently executed system wants to read the value set in the stage, it has to use the `ecs_get` API.

A system _always_ receives the component arrays from the main data store. If a previously executed system added components to the stage, the only way to access these values is through the `ecs_get` API.

Using `ecs_get` and `ecs_set` has a performance penalty however as nothing beats the raw performance of inline reads/writes on a contiguous array. Expect application performance to take a significant hit when using these API calls versus using inline modifications.

#### Staging and ecs_get
When an application uses `ecs_get` while iterating, the operation may return data from to the main data store or to the stage, depending on whether the component has been added to the stage. Consider the following example:

```c
void System(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, p, 1);
    ECS_COLUMN_COMPONENT(rows, Velocity, 2);

    for(int i = 0; i < rows->count; i ++) {
        // Get data from main Flecs store
        Velocity *v = ecs_get_ptr(rows->world, rows->entities[i]);
        
        // Explicitly set component in the stage
        ecs_set(rows->world, rows->entities[i], Velocity, {1, 1});
        
        // ecs_get_ptr now returns component from the stage
        v = ecs_get_ptr(rows->world, rows->entities[i], Velocity);
        
        // Change value in the stage
        v[i].x ++;
    }
}
```

The `ecs_get_ptr` operation returns a pointer from the main data store _unless_ the component is staged.

#### Overwriting the stage
A single iteration may contain multiple operations on the same component. Consider the following example:

```c
void System(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Position, p, 1);
    ECS_COLUMN_COMPONENT(rows, Velocity, 2);

    for(int i = 0; i < rows->count; i ++) {
        // Set component Velocity in the stage
        ecs_set(rows->world, rows->entities[i], Velocity, {1, 1});
        
        // Overwrite component Velocity in stage
        ecs_set(rows->world, rows->entities[i], Velocity, {2, 2});
    }
}
```

The stage behaves the same as the main data store, in that it stores entities with their type, and tables in which all components for a type are stored. When a component is written twice in an iteration, it just means that its record in the staged table will be overwritten. This example is contrived, but it is not inconceivable that two subsequent systems modify the same component in a single iteration.

#### Staging and EcsOnAdd, EcsOnSet and EcsOnRemove
Systems that are executed after adding, setting and removing components (`EcsOnAdd`, `EcsOnSet`, `EcsOnRemove`) work also on staged components. When a component is added while iterating, the applicable `EcsOnAdd` systems will be called and the staged component will be exposed as a regular `ECS_COLUMN`. Note that this is different from regular (`EcsOnUpdate` etc) systems, where an `ECS_COLUMN` always returns an array from the main data store.

The `EcsOnAdd` and `EcsOnSet` systems are executed by the `ecs_add`/`ecs_set` operation. This means that data is always initialized by an `EcsOnAdd` system before `ecs_add` returns, and that any actions executed by an `EcsOnSet` system are executed before `ecs_set` returns.

`EcsOnRemove` systems however are not immediately ran by `ecs_remove`. Such systems are typically used as destructors which clean up/free any resources that a component owns. As such, `EcsOnRemove` systems can invalidate component values, making accessing a component value after executing an `EcsOnRemove` unsafe. For this reason, `EcsOnRemove` systems are executed during the merge, and not while iterating. This ensures that for the duration of the iteration, it is guaranteed that component values passed into systems are valid.

The folllowing code shows an example of an `EcsOnAdd` and `EcsOnRemove` system, and when they are executed:

```c
void AddVelocity(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Velocity, 1);

    for(int i = 0; i < rows->count; i ++) {
        // ...
    }
}

void RemoveVelocity(ecs_rows_t *rows) {
    ECS_COLUMN(rows, Velocity, 1);

    for(int i = 0; i < rows->count; i ++) {
        // ...
    }
}

void System(ecs_rows_t *rows) {
    ECS_COLUMN_COMPONENT(rows, Velocity, 1);

    for(int i = 0; i < rows->count; i ++) {
        // AddVelocity will be invoked before ecs_add returns
        ecs_add(rows->world, rows->entities[i], Velocity);
        
        // RemoveVelocity will not be invoked until iteration finishes
        ecs_remove(rows->world, rows->entities[i], Velocity);
    }
}

int main(int argc, const char *argv[]) {
    ecs_world_t *world = ecs_init();
    
    ECS_SYSTEM(world, AddVelocity, EcsOnAdd, Velocity);
    ECS_SYSTEM(world, RemoveVelocity, EcsOnRemove, Velocity);
    ECS_SYSTEM(world, System, EcsOnUpdate, Velocity);

    ecs_progress(world, 1);
    
    return ecs_fini();
}
```

#### Staging and system phases
Systems in Flecs can be be assigned to different phases (see [System phases](#system-phases)). To ensure that data is available between phases, a merge is performed between each phase. This means for example that any components added or set during the `EcsPreUpdate` phase, will be merged with the main data store before the `EcsOnUpdate` phase starts. This by default limits the effects of staging to a single phase, and can improve reusability of systems as they do not need to be aware of how systems in another phase used staging.

While merging after each phase is the default behavior, applications can choose to [manually merge data](#manually-merging-stages).

#### Staging and threading
Staging is the key enabler for multithreading in Flecs, as it provides a mechanism for mutating state in a thread-safe way without requiring locking. When an application is ran on multiple threads (using the `ecs_set_threads` operation) each thread receives its own private stage which the thread can read and write without having to synchronize with other threads.

As a result, there are no limitations on adding/removing components, and creating/deleting entities from a thread. Invoking `ecs_add` or `ecs_set` from a thread is not significantly more expensive than calling it from a system executed on a single thread, as both use the same mechanism to prevent operations from mutating the Flecs data store while iterating. At the end of every phase, stages are merged with the main data store just like a normal iteration, with the only difference that Flecs needs to merge N stages as opposed to just a single one.

This capability does not absolve applications from the responsibility of making sure access to component data is thread safe. The component data that systems receive through `ECS_COLUMN` always comes from the main Flecs store, and is shared between threads. To understand when it is safe and when it is not safe to access (and modify) this data, it is important to understand how Flecs splits up the workload for a systems.

Flecs distributes load in jobs, where the number of jobs generated is equal to the number of threads, and where each job will (roughly) process `number of matched entities / number of threads` entities. This distribution is stable, meaning that in the absense of mutations, threads are guaranteed to process the same entities in subsequent iterations.

With this knowledge, applications can write systems that can safely modify component values inline without having to go through the stage, as every job will only read and write the entities that are allocated to it. Things get trickier when systems _in the same phase_ access _and write_ the same components, and have _different component signatures_. Lets unpack this statement. 

Each phase introduces a sychronization point for Flecs. After running systems in a phase, all threads are synchronized and data is merged. Inside the phase however, system execution is not synchronized, and different systems in that phase may (and will) run at any particular point in time. Imagine phase `EcsOnUpdate` defines systems `Foo` and `Bar` in that order, and that the systems are ran on two threads. System `Foo` writes- and system `Bar` reads data from component `A`. Suppose `Foo` subscribes for `A`, while `Bar` subscribes for `A, B`.

Now suppose thread 1 finishes the `Foo` job before thread 2. In that scenario, thread 1 starts processing the job for `Bar` before thread 2 has finished processing the job for `Foo`. Because system `Bar` has a different signature than `Foo`, it is matched to different entities. This means that an entity may be processed on thread 1 for `Foo`, but on thread 2 for `Bar`. As a result the `Bar` job of thread 1 may start reading data from `A` that thread 2 is still writing, which can result in race conditions.

A straightforward solution to this problem could be to put `Foo` and `Bar` in different phases. This guarantees that `Bar` can access data from `Foo` in a reliable way within the same iteration. If `Bar` however should only use data from the main store, this problem could be addressed by making `Foo` write to the stage instead. That way, changes made by `Foo` will not be visible until the next phase, and `Bar` can safely access the data from `A` inline.

To take the most advantage of staging and multithreading, it is important to understand how both mechanisms work. While Flecs makes it easier to write applications in a way that allows for fast processing of data by multiple threads, applications still need to take care to prevent race conditions. Simply adding more threads with `ecs_set_threads` in an application that is not written with multithreading in mind, will almost surely result in undefined behavior.

#### Manually merging stages
By default, Flecs merges data each iteration, for each phase. While a merge is relatively cheap (especially when there is not much to merge) merging may still incur overhead when there is lots of data to merge. For certain applications with high-throughput requirements, an application may want to take control over when Flecs performs a merge.

Note that this feature is still experimental, and that the API for this feature may change in the future.

An application can indicate that it wants to take control of merging by calling this operation:

```c
ecs_set_automerge(world, false);
```

This will cause Flecs to stop performing automatic merges. An application now has to manually perform a merge, which it can do with this operation:

```c
ecs_merge(world);
```

To write an application that only merges each frame (as opposed to each phase) an application could do:

```c
ecs_set_automerge(world, false);

while (ecs_progress(world, 1)) {
    ecs_merge(world);
}
```

It is even possible to merge every N frames, as the stage is retained across iterations. This also applies to applications that use multiple threads, as the stage for each thread is also maintained across iterations. An example:

```c
ecs_set_automerge(world, false);
ecs_set_threads(world, 4);

int count = 0;
while (ecs_progress(world, 1)) {
    // Merge every 10 frames
    if (!(ecs_get_tick(world) % 10)) {
        ecs_merge(world);
    }
}
```

There are no limitations on how the API can be used when performing merges manually, but there are some things to consider. First of all, staged data is not available as a contiguous buffer, which means that applications can only read/write it with `ecs_get` and `ecs_set`, which has a performance penalty. This is no different than when using automatic merging, but application logic has to be robust against large deltas between the main store and the stage.

Another consideration is that while data within a thread can be accessed through the regular API operations, data between threads is not shared until it is merged. Thread stages are _eventually consistent_, and the delta between the thread stages can be large. Application logic will have to be made explicitly robust against this, as this is not automatic. Turning off automatic merging on an existing codebase will likely break an application.

Lastly, Flecs modules rely on automatic merging (data between phases needs to be synchronized). Only if you use Flecs as a standalone library and do not use any modules, manual merging is possible.

#### Limitations of staging
The staging mechanism was adopted because it provides a fast, transparent way to manage mutations while iterating while being zero-cost when there is no data to merge. While there are no limitations on the operations you can do, there are some considerations that are common-sense once you understand the concept, but not necessarily trivial:

**You cannot call ecs_progress from a system**
When you are iterationg, you cannot start another iteration. If you need to recursively call systems, use the `ecs_run` API.

**You cannot merge from a system**
You cannot invoke `ecs_merge` when you are iterating. Merging may only be called outside of a call to `ecs_progress`.

**You cannot set the number of threads from a system**
Just don't do this.

**You cannot create new systems from a system**
Creating a new system updates administration that is used while invoking the systems.

**You cannot enable/disable systems from a system**
For the same reason you cannot create systems, you can also not enable/disable systems. It changes adminstration that is in use while iterating.

**You cannot preallocate memory from a system**
Preallocating memory directly modifies the component buffers of the main store, and thus is not possible while iterating.

**ecs_count may return unreliable numbers while iterating**
The `ecs_count` operation does not take into account staged data and only counts entities from the main store. 

**You cannot make changes from an EcsOnRemove system beyond the to be removed component**
`EcsOnRemove` systems are invoked during merging. As these systems also iterate over components, any mutations performed while in an `EcsOnRemove` system would require nested staging, which would reduce performance and increase complexity by a lot.

## Modules
Modules are a simple concept used to organize components, systems and entities into units that can be easily reused across applications, but also as a way to organize code in large applications. Modules can be located inside a project, inside a statically linked, dynamically linked, or runtime loaded library. See [good practices](#good-practices) for some tips around how to organize code in modules.

### Importing modules
The default way to import an existing module is by using the `ECS_IMPORT` macro. This will invoke the module loader that instantiates all contents of the module in the Flecs data store. After importing a module, components can be used, entities will be created and systems will be matched against any entities in the application. To import a module with `ECS_IMPORT`, do:

```c
ECS_IMPORT(world, MyModule, flags);
```

Here, `MyModule` is the module identifier. The `flags` parameter is an integer that is passed to the module loader, which allows an application to specify whether only specific parts of the module should be included. For example, a module may contain systems for moving entities in both 2D and 3D, and the application may only be interested in the 2D systems. Flecs defines a few flags out of the box, but modules are not required to use them. The application that imports the module should refer to the module documentation to find out which flags are supported. The flags defined by Flecs are:

- ECS_2D
- ECS_3D
- ECS_REFLECTION

The same module may be imported more than once without introducing duplicate definitions into the Flecs world.

#### Module content handles
To interact with module contents, the application needs handles to the content. Handles are needed to, for example, add or remove components (component handle is required) or enabling/disabling systems (system handle is required). When a module is imported, handles are automatically imported into the current application scope. In other words, if a module is imported with the `ECS_IMPORT` macro in the main function, the handles of the contents in the module will be available to code in the main function.

To enable access to module handles from outside of the function in which it was imported, Flecs registers a type with the module handles as a singleton component. To obtain a struct with the module handles, an application can use the `ecs_get_singleton` operation:

```c
ecs_entity_t ecs_type(MyModule) = ecs_lookup(world, "MyModule");
MyModule module_handles = ecs_get_singleton(world, MyModule);
```

Additionally, applications can get access to this struct with the `ecs_module()` macro after an `ECS_IMPORT`:

```c
ecs_module(MyModule); // returns struct with handles
```

The returned `MyModule` struct contains handles to the module components, but for the Flecs API to work correctly, they need to be imported in the current function scope. This will define a local variable for each handle inside the struct, which are used by the Flecs API. To do this, use the module specific `ImportHandles` macro:

```c
MyModuleImportHandles(module_handles);
```

A full code example that uses `ecs_module`:

```c
void do_stuff(ecs_world_t *world, MyModule *handles) {
    MyModuleImportHandles(*handles);
    return ecs_new(world, ComponentFromMyModule);
}

int main(int argc, char *argv[]) {
    ecs_world_t *world = ecs_init();

    ECS_IMPORT(world, MyModule, 0);

    do_stuff(world, &ecs_module(MyModule));

    ecs_fini(world);
}
```

#### Dynamic imports
Modules can be dynamically imported from a shared library. This is only possible when an application does not require compile-time knowledge about the contents of a module, but can be a powerful mechanism for dynamically loading functionality into an application. To dynamically load a module, applications can use the `ecs_import_from_library` function:

```c
ecs_import_from_library(world, "flecs.systems.admin", "FlecsSystemsAdmin");
```

The `flecs.systems.admin` argument is the name of the library or package. The `FlecsSystemsAdmin` argument is the name of the module, as a library may contain more than one module. If the module name is `NULL`, Flecs will automatically attempt to derive the module name from the library name. Currently this operation is only supported when Flecs is built with the bake build system, as Flecs reuses package management functions from the bake runtime. Future versions of Flecs will support this functionality for other build systems as well.

### Creating modules
A few steps are required to create a module from scratch:

- Create a type which contains the _public_ handles
- Create a function that instantiates the module contents
- Create a macro that declares handle variables

These steps all have to conform to a specific naming convention of them to work correctly. Fortunately, For each of these steps, Flecs has convenience macro's that simplify this process.

Suppose a module called `MyTransformModule` defines a component called `Position`. First of all, we have to create a struct that exposes the handle to `Position`. To do this, create the following struct in a publicly available header of the module:

```c
typedef struct MyTransformModuleHandles {
    ECS_DECLARE_COMPONENT(Position);
} MyTransformModuleHandles;
```

Make sure that the struct name is the module name (`MyTransformModule`) plus `Handles`. In that same header, a function must be declared that will be responsible for importing the module contents into Flecs:

```c
void MyTransformModuleImport(ecs_world_t *world, int flags);
```

Again, make sure that the name of the function is the module name plus `Import`, and that it follows this exact function signature. Now we need to create a macro that declares handle variables. This macro is used by the `ECS_IMPORT` macro to make the `Position` handle available to the application function where this module is imported. Creating this macro is straightforward, and looks like this:

```c
#define MyTransformModuleImportHandles(handles)\
    ECS_IMPORT_COMPONENT(handles, Position)
```

When this macro is invoked by `ECS_IMPORT`, it will receive an initialized instance to the `MyTransformModuleHandles` struct.

Lastly, the function that imports the contents into Flecs needs to be created. This function needs to accomplish a few things, outlined in this example:

```c
void MyTransformModuleImport(ecs_world_t *world, int flags)
{
    // Create the module in Flecs
    ECS_MODULE(world, MyTransformModule);

    // Create the component
    ECS_COMPONENT(world, Position);

    // Export the component
    ECS_EXPORT_COMPONENT(Position);
}
```

With that in place, an application can now use `ECS_IMPORT` to import the module:

```c
ECS_IMPORT(world, MyTransformModule, 0);
```

When a module defines other kinds of things besides components, a different set of macro's is used. The following code needs to be added to a header of a module that exports a system:

```c
// To declare a system handle, use ECS_DECLARE_ENTITY
typedef struct MyTransformModuleHandles {
    ECS_DECLARE_COMPONENT(Position);
    ECS_DECLARE_ENTITY(MySystem);
} MyTransformModuleHandles;

// This does not change
void MyTransformModuleImport(ecs_world_t *world, int flags);

// To import a system handle, use ECS_IMPORT_ENTITY 
#define MyTransformModuleImportHandles(handles)\
    ECS_IMPORT_COMPONENT(handles, Position)\
    ECS_IMPORT_ENTITY(handles, MySystem)
```

The `MyTransformModuleImport` function then needs to be changed to this:

```c
void MyTransformModuleImport(ecs_world_t *world, int flags)
{
    // Create the module in Flecs
    ECS_MODULE(world, MyTransformModule);
    
    // Create the component
    ECS_COMPONENT(world, Position);

    // Create the component
    ECS_SYSTEM(world, MySystem, EcsOnUpdate, Position);

    // Export the component
    ECS_EXPORT_COMPONENT(Position);

    // Export the system
    ECS_EXPORT_ENTITY(MySystem);
}
```

Prefabs, types and tags can all be exported with the ECS_EXPORT_ENTITY macro.

### Operating system abstraction API
Flecs relies on functionality that is not standardized across platforms, like threading and measuring high resolution time. While the essential features of Flecs work out of the box, some of its features require additional effort. To keep Flecs as portable as possible, it does not contain any platform-specific API calls. Instead, it requires the application to provide them.

Note that when Flecs is built with bake, it automatically uses the platform specific API from the bake runtime. Applications do not need to manually set these, unless they want to provide their own implementations.

The OS API is defined by a struct in `util/os_api.h`. The default pattern for providing custom abstraction functions is:

```c
// Set default calls, like malloc, free
ecs_os_set_api_defaults();

// Obtain a private writable copy of the os_api struct
ecs_os_api_t os_api = ecs_os_api;

// Override any calls
os_api.thread_new = my_thread_new;
os_api.thread_join = my_thread_join;

// Set the custom API callbacks
ecs_os_set_api(&os_api);
```

These APIs callbacks are set automatically when calling `ecs_os_set_api_defaults`:

- malloc
- free
- realloc
- calloc
- log
- log_error
- log_debug
- abort

When Flecs is built with bake, these additional API callbacks are set to the corresponding functions from the bake runtime:

- thread_new
- thread_join
- mutex_new
- mutex_free
- mutex_lock
- mutex_unlock
- cond_new
- cond_free
- cond_signal
- cond_broadcast
- cond_wait
- sleep
- get_time
