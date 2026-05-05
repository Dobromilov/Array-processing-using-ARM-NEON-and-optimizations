# Array-processing-using-ARM-NEON-and-optimizations

---

Цель: освоить возможности ARM-архитектуры на языке C++ с помощью NEON-
интринсиков, условных операций без ветвлений и баррельного шифтера. Написать
векторную реализацию функции, которая вычисляет сумму модифицированных элементов
массива, максимально используя аппаратные особенности ARM.

## Установка необходимых пакетов

```bash
sudo apt update
sudo apt install cmake pkg-config
sudo apt install g++-arm-linux-gnueabihf gcc-arm-linux-gnueabihf qemu-user
```

Для GUI-запуска на настоящем ARM-устройстве нужны системные библиотеки окна и
OpenGL:

```bash
sudo apt install libglfw3-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

Если GUI собирается с x86_64 под `armhf` и запускается через QEMU, эти же
библиотеки должны быть установлены в `armhf` варианте, например
`libglfw3-dev:armhf` и `libgl1-mesa-dev:armhf`.

## Сборка и запуск GUI

```bash
make host-run
```

На ARM-машине эта команда соберёт и запустит GUI нативно. На x86_64 она
собирает host-версию.

Если устройство использует OpenGL ES вместо desktop OpenGL:

```bash
make host-run CMAKE_ARGS=-DARRAY_USE_GLES=ON
```

## ARM через QEMU

GUI ARM-сборка:

```bash
make armhf-run
```

Эта команда собирает `build-armhf/array_benchmark` кросс-компилятором
`arm-linux-gnueabihf-g++` и запускает его через:

```bash
qemu-arm -L /usr/arm-linux-gnueabihf build-armhf/array_benchmark
```

Если на x86_64 нет armhf OpenGL/GLFW библиотек, можно проверить NEON-часть без
GUI:

```bash
make armhf-cli-run
```

## Что собирается

- `array_processing` (статическая библиотека с базовой и NEON-реализациями)
- `array_benchmark` (GUI на ImGui/GLFW)
- `array_benchmark_cli` (консольная ARM/QEMU smoke-проверка)
