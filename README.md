# lite_fnds

**lite_fnds** — a lightweight C++14 foundations library for zero-overhead, composable system components.

This project started as a personal experiment to understand the fundamentals of C++ systems programming — and has grown into a small but expressive collection of low-level building blocks: memory primitives, lock-free concurrency pieces, task abstractions, and a static “Flow” pipeline engine.

---

## 🧩 Modules

| Category | Components |
|-----------|-------------|
| **Base** | `inplace_base`, `traits`, `type_erase_base` |
| **Memory** | `inplace_t`, `either_t`, `result_t`, `static_mem_pool`, `hazard_ptr` |
| **Concurrency** | `spsc_queue`, `mpsc_queue`, `mpmc_queue` |
| **Utility** | `compressed_pair`, `callable_wrapper`, `static_list` |
| **Task** | `task_core`, `future_task`, `task_wrapper` |
| **Flow** | `flow_blueprint`, `flow_node`, `flow_runner` `flow_aggregator` |

---

# 🚀 Flow — Static Execution Blueprints (Core Highlight)

`lite_fnds::flow` is a **zero-overhead, compile-time optimized execution pipeline**, designed around three ideas:

## **1. Blueprint — Pipelines as Static Structures**

Instead of building dynamic chains (`std::function`/virtual dispatch),  
Flow compiles the entire pipeline **into a static blueprint**:

- All nodes exist as a `std::tuple`
- Node types are known at compile time
- The pipeline is fully type-checked and exception-safe
- No heap allocations
- No dynamic polymorphism

A blueprint is simply:

```cpp
auto bp = make_blueprint<int>()
        | transform(...)
        | then(...)
        | on_error(...)
        | via(...)
        | end(...);
```

## **2. Node Fusion — Compile-Time Optimization**

Flow distinguishes two kinds of nodes:
* calc nodes (value transformations)
* control nodes (thread dispatch, scheduling, fan-out)

During blueprint construction, Flow performs automatic fusion:
* Multiple calc nodes merge into one giant callable (zero dispatch overhead)
* Control nodes override previous control points to avoid nonsensical chains
* The final blueprint is minimized and efficient

This means:
- A pipeline:
 <br/> `calc + calc + calc + control + calc + calc + control ... + 1 end`
 <br/> becomes 
 <br/> `calc + control + calc + ... + 1 end`.

Execution stays fast and stack-friendly.

## **3. Runner — A Tiny Execution Engine**

A flow_runner only stores:
* pointer to the blueprint
* pointer to the controller (for cancellation)

It is extremely lightweight:

```cpp
auto runner = make_runner(bp);
runner(42);  // start execution
```

✔ Soft & Hard cancellation
* Soft cancel: finishes the current stage and terminates at the next control boundary
* Hard cancel: jumps directly to the end node with a cancel error

✔ End node = result sink
* `end(...)` is the only place side-effects are guaranteed.
* You can branch/fan-out here safely without corrupting blueprint semantics.

✔ Exception-safe pipeline
Any exception inside a node is captured as std::exception_ptr and forwarded.
```cpp
#include <iostream>

#include "flow/flow_blueprint.h"
#include "flow/flow_node.h"
#include "flow/flow_runner.h"

struct fake_executor {
    void dispatch(lite_fnds::task_wrapper_sbo sbo) noexcept {
        sbo();
    }
};

int main(int argc, char *argv[]) {
    using std::cout;
    using std::endl;

    using lite_fnds::result_t;
    using lite_fnds::value_tag;
    using lite_fnds::error_tag;

    using lite_fnds::make_blueprint;
    using lite_fnds::via;
    using lite_fnds::transform;
    using lite_fnds::then;
    using lite_fnds::end;
    using lite_fnds::on_error;
    using lite_fnds::make_runner;
    using lite_fnds::catch_exception;
    using E = std::exception_ptr;

    fake_executor executor;

    int v = 100;
    auto bp = make_blueprint<int>()
         | via(&executor)
         | transform([&v] (int x) noexcept{
             return v += 10, (double)v + x;
         })
         | then([](result_t<double, E> f) {
             std::cout << f.value() << std::endl;
             if (f.value() > 120) {
                 throw std::logic_error("exception on then node error");
             }
             f.value() += 10;
             return f;
         })
         | on_error([&](result_t<double, E> f) {
             try {
                 std::rethrow_exception(f.error());
             } catch (const std::logic_error& e) {
                 std::cout << e.what() << std::endl;
                 // return result_t<double, E>(value_tag, 1.0);
                 throw e;
             } catch (...) {
                 return result_t<double, E>(error_tag, std::current_exception());
             }
         })
         | catch_exception<std::logic_error>([](const std::logic_error& e) {
                std::cout << e.what() << endl;
                return 3.0;
            })
         | end([](result_t<double, E> f) {
             if (f.has_value()) {
                 std::cout << "finaly value is: " << f.value() << std::endl;
             }
             return f;
         });

    using bp_t = decltype(bp);
    std::shared_ptr<bp_t> bp_ptr = std::make_shared<bp_t>(std::move(bp));
    auto runner = make_runner(bp_ptr);

    runner(10);
    cout << "V become after one shot of bp:" << v << endl;
    cout << endl;

    /*
    * 120
    * finaly value is: 130
    * become after one shot of bp:110
    */

    runner(10);
    cout << "V become after one shot of bp:" << v << endl;
    cout << endl;

    /*
     * 130
     * exception on then node error
     * exception on then node error
     * finaly value is: 3
     * V become after one shot of bp:120
     */
    return 0;
}
```

⚙️ Build
Almost header-only — no special build steps.
 * Requires C++14
 * No dependencies other than the C++ Standard Library

💡 Design Philosophy

lite_fnds aims to stay:

* ***Lightweight*** — minimal runtime overhead and zero heap allocation where possible
* ***Safe*** — strong ownership, clear invariants, explicit lifetime control
* ***Composable*** — modules work independently or combine naturally
* ***Predictable*** — no surprising behavior, strong noexcept discipline
* ***Executable*** — Flow blueprints encode logic at compile time, runners execute with minimal runtime cost

📄 License

MIT License © 2025 Nathan