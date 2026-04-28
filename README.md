# Array-processing-using-ARM-NEON-and-optimizations

---

Цель: освоить возможности ARM-архитектуры на языке C++ с помощью NEON-
интринсиков, условных операций без ветвлений и баррельного шифтера. Написать
векторную реализацию функции, которая вычисляет сумму модифицированных элементов
массива, максимально используя аппаратные особенности ARM.

## Быстрый запуск на ARM без эмуляции (Apple Silicon / Linux ARM)

Если устройство уже ARM (например, Mac с M1/M2/M3), запускайте нативно:

```bash
make native-arm-run
```

Или по шагам:

```bash
make native-arm-build
./go
```

`native-arm-*` цели проверяют архитектуру хоста (`arm64`/`aarch64`) и
используют обычную host-сборку без QEMU.

Если на Apple Silicon команда пишет `Current: x86_64`, терминал запущен через
Rosetta. Откройте обычный ARM-терминал или запустите:

```bash
arch -arm64 zsh
make native-arm-run
```

## Сборка и запуск (ARM через QEMU)

Проект собирается кросс-компилятором `arm-linux-gnueabihf-g++`, исполняемый файл
создаётся в корне проекта как `go`.

```bash
make build
make run
```

`make run` выполняет:

```bash
qemu-arm -L /usr/arm-linux-gnueabihf ./go
```

## Что собирается

- `libarray_processing.a` (статическая библиотека)
- `libarray_processing.so` (динамическая библиотека)
- `go` (исполняемый файл)
