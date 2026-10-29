# Digital Puzzle Solver

This program finds the longest possible sequence from given numerical fragments. Fragments can be connected if the last two digits of one fragment match the first two digits of the next.

The project is implemented in **C++** and solves the NP-hard longest path problem in a graph using:

* Depth-First Search (DFS).

* Dynamic Programming with Bitmask (Bitmask DP) and memoization.

* A custom high-speed cache and the Xorshift hash function.

* Multithreading to maximize CPU core utilization.

## Requirements

* A compiler supporting the **C++17** standard (GCC or Clang).

* A text file named `data.txt` in the root directory of the project, containing the puzzle fragments (separated by spaces or newlines).

## How to compile and run

1. Ensure that your source code is in the `main.cpp` file and the input data is in the `data.txt` file within the same directory.

2. Open a terminal in the project directory.

3. Run the following command to compile the program (this includes maximum optimization `-O3` and thread support):

```bash
g++ -O3 -pthread -std=c++17 main.cpp -o puzzle
```

4. After successful compilation, run the executable file:

```bash
./puzzle
```

---

# Розв'язувач цифрового пазла (Digital Puzzle Solver)

Ця програма знаходить найдовшу можливу послідовність із заданих числових фрагментів. Фрагменти з'єднуються між собою, якщо останні дві цифри одного фрагмента збігаються з першими двома цифрами наступного.

Проєкт реалізовано на **C++** і він вирішує NP-складну задачу пошуку найдовшого шляху в графі, використовуючи:

* Алгоритм пошуку в глибину (DFS).

* Динамічне програмування з бітовими масками (Bitmask DP) та мемоїзацією.

* Власний високошвидкісний кеш і хеш-функцію Xorshift.

* Багатопотоковість для максимального використання всіх ядер процесора.

## Вимоги

* Компілятор із підтримкою стандарту **C++17** (GCC або Clang).

* Текстовий файл `data.txt` у кореневій папці проєкту, який містить фрагменти пазла (по одному або через пробіл/новий рядок).

## Як скомпілювати та запустити

1. Переконайтеся, що ваш вихідний код знаходиться у файлі `main.cpp`, а дані для обробки — у файлі `data.txt` у тій самій папці.

2. Відкрийте термінал у папці з проєктом.

3. Виконайте наступну команду для компіляції програми (включено максимальну оптимізацію `-O3` та підтримку потоків):

```bash
g++ -O3 -pthread -std=c++17 main.cpp -o puzzle
```

4. Після успішної компіляції запустіть створений файл:

```bash
./puzzle
```