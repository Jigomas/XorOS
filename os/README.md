# XorOS

Минимальная bare-metal операционная система для rv32i симулятора.

![C11](https://img.shields.io/badge/C-C11-blue)
![RISC-V](https://img.shields.io/badge/arch-RISC--V%20RV32IA-green)
![tests](https://img.shields.io/badge/tests-27%20passing-brightgreen)

---

## О проекте

Ядро написано на C11 и ассемблере, компилируется без libc в плоский бинарник.
Симулятор загружает его по адресу `0x0` и начинает исполнение с PC=0.
Загрузчик (`boot.S`) инициализирует стек, устанавливает mtvec, через `mret` остаётся
в M-mode и передаёт управление `kernel_main`. Покрыта тестами.

```plaintext
Адресное пространство (64 KiB):

  0xFFFF  ┤ конец памяти
  0xF010  ├─ CLINT mtimecmp hi (MMIO)
  0xF00C  ├─ CLINT mtimecmp lo (MMIO)
  0xF008  ├─ CLINT mtime hi    (MMIO)
  0xF004  ├─ CLINT mtime lo    (MMIO)
  0xF000  ├─ UART TX           (MMIO · write → stdout)
          │
  0xA000  ┤ ...свободно...
          │
  0x6000  ├─ heap     (kalloc bump · 4 страницы × 4 KiB)
  0x5000  ├─ l0_pt    (Sv32 second-level PT · 4 KiB)
  0x4000  ├─ root_pt  (Sv32 root page table · 4 KiB)
          │
  0x2000  ├─ SP       (стек растёт вниз ↓)
          │
  0x0000  ├─ _start   (код · flat binary загружается сюда)
```

### Архитектурные решения

**Flat binary вместо ELF.** ОС компонуется с флагами `-Ttext=0x0 -e _start` в сырой
бинарник без заголовков. Симулятор копирует его в начало памяти напрямую — никакого
ELF-загрузчика. Стек ядра в `0x2000`, растёт вниз в пределах 64 KiB.
Стеки процессов встроены в PCB (по 512 байт каждый). Нижнее слово каждого стека
содержит канарейку `0xDEADBEEF` — перед каждым переключением контекста планировщик
проверяет её целостность и выводит `panic: stack overflow in pid N` при порче.

**ECALL через fireTrap.** Системные вызовы (`sys_putchar`, `sys_exit`) реализованы
через `ecall` — симулятор не перехватывает их в C++, а кидает `fireTrap(EXC_ECALL_U/S/M)`
в зависимости от текущего уровня привилегий. Управление передаётся на `mtvec`,
`trap.S` сохраняет caller-saved регистры и вызывает `trap_handler`. Тот читает `a7` из
кадра стека, диспетчеризует системный вызов и возвращается через `mret`. Параллельно
работает MMIO UART: `uart.c` пишет байт по адресу `0xF000`, симулятор перехватывает
запись и выводит символ в stdout без системного вызова.

**Без stdlib.** Флаги `-nostdlib -ffreestanding` — никаких зависимостей от libc.
Всё, что нужно ядру, реализовано вручную или через MMIO симулятора.

**Spinlock через A-ext.** `spinlock.h` реализует lock/unlock через `amoswap.w.aq`/`amoswap.w.rl`
(RISC-V A-расширение). Ключевая деталь: constraint `"=&r"` (early-clobber) гарантирует,
что GCC не алиасирует `rd` и `rs2` в одном регистре — без этого симулятор зависает
на бесконечном цикле lock.

---

## Структура проекта

```plaintext
os/
├── include/
│   ├── ecall.h       — системные вызовы: sys_putchar / sys_exit через inline asm ecall
│   ├── csr.h         — адреса CSR-регистров и коды исключений (включая CAUSE_ECALL_U/S/M)
│   ├── trap.h        — trap_fn_t (принимает *frame), trap_handler, trap_register_exc/int
│   ├── kalloc.h      — физический аллокатор страниц (PAGE_SIZE, kalloc)
│   ├── spinlock.h    — spinlock_t: lock/unlock через amoswap.w (A-ext)
│   ├── process.h     — PCB: proc_state_t, context_t, proc_stack_t (с канарейкой)
│   ├── scheduler.h   — API планировщика: sched_init/spawn/yield/exit
│   ├── pipe.h        — pipe_t: кольцевой буфер; pipe_init/write/read
│   ├── uart.h        — MMIO UART: uart_putchar/puts → 0xF000
│   ├── clint.h       — CLINT MMIO 0xF004-0xF010; clint_init(interval), clint_set_next()
│   └── vmem.h        — Sv32 виртуальная память: vmem_map/vmem_enable, флаги PTE
├── src/
│   ├── boot.S          — _start: SP, GP, mtvec, mret (MPP=M), вызов kernel_main
│   ├── kernel_main.c   — демо: task_a пишет в pipe, task_b читает и печатает; clint_init(1000); timer_handler вызывает sched_yield
│   ├── trap.S          — trap_entry: сохранение всех 32 регистров в *frame (sp через mscratch);
│   │                     mepc += 4 для ecall-причин перед восстановлением регистров
│   ├── trap.c          — ecall_handler (a7=1 → uart, a7=10 → halt); диспетчер mcause;
│   │                     паника через inline MMIO; do_halt через .word 0x0
│   ├── kalloc.c        — bump allocator: статический heap[4×4KiB], без free
│   ├── sched_switch.S  — context_switch: сохранение/восстановление callee-saved
│   ├── scheduler.c     — round-robin планировщик, трамплин, проверка канарейки
│   ├── pipe.c          — реализация кольцевого буфера (non-blocking)
│   ├── uart.c          — uart_putchar/puts через MMIO (0xF000)
│   ├── clint.c         — clint_init: устанавливает mtimecmp, включает MTIE (csrrs mie)
│   └── vmem.c          — Sv32 page tables: root_pt/l0_pt, vmem_map, vmem_enable
├── tests/
│   └── test_ecall.c  — 27 тестов: sys_putchar, арифметика, Sv32 vmem, stack canary, pipe, kalloc, spinlock
├── CMakeLists.txt
└── README.md
```

---

## Требования

| Инструмент              | Версия    |
|-------------------------|-----------|
| CMake                   | ≥ 3.16    |
| riscv64-unknown-elf-gcc | 13.2.0    |
| rv32i_cpu (симулятор)   | из rv32i/ |

Установка кросс-компилятора:

```bash
sudo apt-get install gcc-riscv64-unknown-elf
```

---

## Сборка

OS собирается автоматически из корневого CMake:

```bash
# Из корня репозитория:
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Результат: `os/build/xoros.bin` и `os/build/xoros_tests.bin` — flat binary.

---

## Запуск

```bash
# Запустить ядро
./build/rv32i/rv32i_cpu os/build/xoros.bin

# Запустить тесты OS через CMake
cmake --build os/build --target run_tests
```

Ожидаемый вывод ядра:

```plaintext
Hello from XorOS!
hi
kernel: all done

cache: 9901 hits / 2606 misses | 79.2% hit rate
```

Ожидаемый вывод тестов:

```plaintext
=== XorOS ecall tests ===
[PASS] sys_putchar does not crash
[PASS] arithmetic: 1+1==2
[PASS] arithmetic: 10-3==7
[PASS] arithmetic: 10/3==3
[PASS] arithmetic: 10%3==1

--- vmem ---
[PASS] code runs after vmem_enable
[PASS] stack read after vmem_enable
[PASS] write+read via virtual addr
[PASS] arithmetic after vmem_enable: 6*7=42

--- canary ---
[PASS] STACK_CANARY == 0xDEADBEEF
[PASS] canary set on spawn
[PASS] canary intact after normal exit

--- pipe ---
[PASS] pipe_init: count == 0
[PASS] pipe_write returns 0
[PASS] pipe_read returns 0
[PASS] pipe_read correct byte
[PASS] pipe_read empty returns -1
[PASS] pipe_write full returns -1
[PASS] pipe wrap-around write ok
[PASS] pipe wrap-around read ok

--- kalloc ---
[PASS] kalloc returns non-NULL
[PASS] kalloc page-aligned
[PASS] two allocs differ
[PASS] pages are PAGE_SIZE apart

--- spinlock ---
[PASS] spinlock_init: unlocked
[PASS] spinlock_aquire: locked
[PASS] spinlock_release: unlocked

passed: 27
failed: 0

cache: 357940 hits / 169674 misses | 67.8% hit rate
```

---

## Системные вызовы (`ecall.h`)

Соглашение: номер syscall в `a7`, аргументы в `a0`–`a6`, результат в `a0`.
`ecall` вызывает `fireTrap` в симуляторе → OS trap_handler → `ecall_handler` в `trap.c`.

| Номер (`a7`) | Имя           | Аргумент (`a0`) | Описание               |
|--------------|---------------|-----------------|------------------------|
| 1            | `SYS_PUTCHAR` | символ (char)   | Вывод символа в stdout |
| 10           | `SYS_EXIT`    | —               | Остановка симулятора   |

---

## CSR-регистры (`csr.h`)

Заголовок содержит адреса Machine-level CSR и коды mcause, используемые в `trap.c`.

| Макрос         | Адрес  | Назначение                        |
|----------------|--------|-----------------------------------|
| `CSR_MSTATUS`  | 0x300  | Статус машинного режима           |
| `CSR_MIE`      | 0x304  | Разрешение прерываний             |
| `CSR_MTVEC`    | 0x305  | Адрес обработчика trap            |
| `CSR_MSCRATCH` | 0x340  | Scratch-регистр для trap-хендлера |
| `CSR_MEPC`     | 0x341  | PC момента исключения             |
| `CSR_MCAUSE`   | 0x342  | Причина trap                      |
| `CSR_MTVAL`    | 0x343  | Доп. информация (адрес/инструкция)|
| `CSR_MIP`      | 0x344  | Ожидающие прерывания              |
| `CSR_SATP`     | 0x180  | Управление трансляцией (Sv32)     |
| `CSR_MHARTID`  | 0xF14  | ID аппаратного потока (всегда 0)  |

---

## Дорожная карта

- [ ] idle task — pid=0 с wfi; sched_yield без PROC_READY молча возвращается
- [ ] kfree() + свободный список страниц — kalloc сейчас bump без освобождения
- [ ] per-process page tables — изоляция процессов; требует kfree
- [ ] U-mode процессы — запускать задачи через mret в PrivMode::U; инфраструктура уже есть
- [ ] расширить таблицу ecall — sys_yield, sys_getpid для U-mode задач
- [ ] блокирующий pipe + BLOCKED-состояние — сейчас дропает при переполнении
- [ ] pipe + spinlock — pipe_write/read не thread-safe при вытесняющем планировщике
- [ ] fork + wait
- [ ] exec
- [ ] sbrk(n) syscall
- [ ] COW-форк — ref-count в kalloc, sc.w для fault handler
- [ ] динамический список процессов — array-of-slots без malloc (на основе [Jigomas/List](https://github.com/Jigomas/List))
- [ ] MLFQ — приоритеты в PCB, понижение при использовании кванта, boost раз в период
- [ ] лотерейный планировщик — tickets в PCB, LCG-генератор
- [ ] демо дедлока — два процесса берут два spinlock в разном порядке; watchdog или детектор
- [ ] CoreMark
