# 错误记录

> 记录踩过的坑，防止同一个错误犯两次。
> 规则：**发现一个就记一个**，写清「位置 / 现象 / 原因 / 正确写法」，修完把状态改成 ✅。

## 状态总览

| 编号 | 一句话 | 位置 | 状态 |
|---|---|---|---|
| M1 | 对 `SDL_GetBasePath()` 的返回值调 `SDL_free`（内部缓存，不能释放） | main.cpp:56 | ❌ |
| M2 | `SDL_GetBasePath()` 没判空 | main.cpp:54 | ❌ |
| M3 | 把 `SDL_free_func`（回调类型）误当成 `SDL_free` 的参数 | 概念 | 📖 已澄清 |
| M4 | 重构后残留 `kLines` / `kLineCount` | main.cpp:89,103,106,107 | ❌ |
| M5 | `createLineTexture` 调用少传 `&dst` | main.cpp:106 | ❌ |
| M6 | `std::string` 直接当 `const char*` 传 | main.cpp:88 | ❌ |
| M7 | 空剧本没有检查，`script[cursor]` 越界 | main.cpp:86-88 | ❌ |
| M8 | 字体路径硬编码 `C:/Windows/Fonts/msyh.ttc` | main.cpp:77 | ⚠️ 可移植性 |
| M9 | （AI 出的馊主意）让我"把 const 删掉"，反而 C2440 | main.cpp:54 | 📖 已澄清 |

当前 `cmake --build build --config Debug` 结果：**失败**，报错即 M1、M4、M5、M6。

---

## M1 对 `SDL_GetBasePath()` 的返回值调了 `SDL_free`（不该释放）

- 位置：`src/main.cpp:54-56`
- 现象一（编译期）：

  ```
  error C2664: "void SDL_free(void *)": 无法将参数 1 从"const char *"转换为"void *"
  ```

- 现象二（更严重）：如果用 `const_cast` 强行编过，运行期就变成**释放别人的内存**——典型表现是退出时堆损坏（`0xC0000374`），或者后面再调用 `SDL_GetBasePath()` 拿到悬空指针。
- 原因：SDL3 里这个函数返回的是**内部缓存**，所有权在 SDL：

  ```c
  // src/filesystem/SDL_filesystem.c (release-3.4.18)
  const char *SDL_GetBasePath(void)
  {
      if (!CachedBasePath) {
          CachedBasePath = SDL_SYS_GetBasePath();
      }
      return CachedBasePath;
  }
  ```

  返回 `const char *`（`SDL_filesystem.h:101`）就是在暗示"**这串不是给你的**"。
- 对比 SDL2：`char *SDL_GetBasePath(void)`，文档明确要求调用方 `SDL_free()`。**大版本升级时所有权变了**，SDL2 的代码不能照搬。
- 正确写法：保留 `const`，**不 free**：

  ```cpp
  const char* base = SDL_GetBasePath();   // const 是有意为之，别删
  if (!base) { /* 见 M2 */ }
  std::string scriptPath = std::string(base) + "assets/story.txt";
  // 没有 SDL_free(base) 这一行
  ```

- 同类坑：`SDL_GetExeName()`、`SDL_GetUserFolder()` 也是缓存返回，同样**不要 free**。相反 `SDL_GetPrefPath()` 返回 `char *` 且要求 free —— 判断依据就是**返回类型带不带 const** + 文档有没有写 "should be freed"。
- 教训：**代码没错，是"用完要还"这个直觉错了**。拿指针时先问三句：谁分配的？const 吗？文档说要还吗？

## M2 `SDL_GetBasePath()` 返回值没判空

- 位置：`src/main.cpp:54`
- 现象：函数失败时返回 `NULL`，`std::string(nullptr)` 是未定义行为，通常直接崩在构造函数里，且崩的位置离真正原因很远。
- 正确写法：

  ```cpp
  const char* base = SDL_GetBasePath();
  if (!base) {
      printf("GetBasePath failed: %s\n", SDL_GetError());
      return 1;
  }
  std::string scriptPath = std::string(base) + "assets/story.txt";
  ```

- 教训：**返回指针的 API 一律先判空**；文档里写了 "or NULL on failure" 就要当真。

## M3 以为 `SDL_free` 需要一个「函数」

- 澄清：`SDL_free(void *mem)` 只收一个**指针**，传指针这个动作是对的；错在这个指针**根本不该释放**（见 M1）。
- 那个"要传函数"的 API 是**另一个东西**：

  ```c
  typedef void (SDLCALL *SDL_free_func)(void *mem);      // SDL_stdinc.h:1506

  bool SDL_SetMemoryFunctions(SDL_malloc_func malloc_func,
                              SDL_calloc_func calloc_func,
                              SDL_realloc_func realloc_func,
                              SDL_free_func free_func);  // SDL_stdinc.h:1581
  ```

  这是**替换整个 SDL 分配器**时才用的（比如接自己的内存池 / 检测泄漏），它才要传 `free` 一类的函数指针。
- 另外 `SDL_stdinc.h:6041` 的 `#define SDL_free free` 只在 `__clang_analyzer__` 下生效，不是真实定义，别被它误导。
- 教训：**同名关键字搜索时，先看清是 `SDL_free` 还是 `SDL_free_func` / `SDL_SetMemoryFunctions`。**

## M4 重构后残留旧全局变量

- 位置：`src/main.cpp:89, 103, 106, 107`
- 现象：

  ```
  error C2065: "kLineCount": 未声明的标识符
  error C2065: "kLines": 未声明的标识符
  ```

- 原因：从"编译期常量数组"改成"运行时 `std::vector<std::string> script`"之后，只改了上半截，下半截还在用 `kLines[cursor]` / `kLineCount`。
- 正确写法：统一换成 `script`：

  ```cpp
  printf("[%d/%d] %s\n", cursor + 1, (int)script.size(), script[cursor].c_str());
  ...
  if (cursor + 1 < (int)script.size()) { ... }
  ```

  注意 `size()` 是 `size_t`（无符号），和 `int cursor` 比较会报警告，显式 `(int)` 转换最省事。
- 教训：**改数据结构时，用编译器把旧名字全部炸出来**，别靠眼睛扫。改完立刻编一次。

## M5 `createLineTexture` 少传参数

- 位置：`src/main.cpp:106`
- 现象：`error C2660: "createLineTexture": 函数不接受 3 个参数`（声明是 4 个）
- 正确写法：

  ```cpp
  tex = createLineTexture(ren, font, script[cursor].c_str(), &dst);
  ```

- 教训：和 M4 同源——**只改了一半**。函数签名变了，所有调用点都要跟着改。

## M6 `std::string` 当 `const char*` 用

- 位置：`src/main.cpp:88`
- 现象：`error C2664: 无法将参数 3 从"std::string"转换为"const char *"`
- 正确写法：`script[cursor].c_str()`
- 教训：`std::string` **不会**隐式转成 `const char*`，`.c_str()` 是必须的。注意 `c_str()` 的指针在 string 被修改/销毁后失效，别存起来。

## M7 剧本为空时越界

- 位置：`src/main.cpp:86-88`
- 现象：`assets/story.txt` 目前是 **0 字节**；`loadScript` 对空文件能正常打开、逐行读完、返回空 vector，然后 `script[cursor]` 直接越界 → 崩溃（Debug 下可能报 "vector subscript out of range"）。
- 原因：`loadScript` 里"打不开"和"打开了但是空的"是两种失败，代码只处理了前者；调用方也没检查返回值。
- 正确写法：

  ```cpp
  std::vector<std::string> script = loadScript(scriptPath);
  if (script.empty()) {
      printf("剧本为空：%s\n", scriptPath.c_str());
      return 1;
  }
  ```

- 教训：**`operator[]` 不做边界检查**。凡是"从文件/网络读来的东西"，长度都可能为 0，先判空再取。

## M8 字体路径硬编码

- 位置：`src/main.cpp:77`：`TTF_OpenFont("C:/Windows/Fonts/msyh.ttc", 40.0f)`
- 风险：换台机器/换系统就没有微软雅黑；打包给别人也跑不起来。
- 建议：把 ttf 放进 `assets/`，用 M2 里拿到的 `basePath` 拼相对路径。
- 教训：**资源路径一律相对 exe，不写绝对路径。**

---

## 备忘：SDL 指针所有权规则

| 函数 | 返回类型 | 要 free 吗 |
|---|---|---|
| `SDL_GetBasePath()` | `const char *` | ❌ 内部缓存，**不 free** |
| `SDL_GetExeName()` | `const char *` | ❌ 内部缓存 |
| `SDL_GetUserFolder()` | `const char *` | ❌ 内部缓存 |
| `SDL_GetPrefPath()` | `char *` | ✅ 要 `SDL_free` |
| `SDL_GetError()` | `const char *` | ❌ 归 SDL 所有 |

判断口诀：**返回 const 指针 ⇒ 别动它；返回 `char *` 且文档写 should be freed ⇒ 用完 `SDL_free`。**
- SDL 返回的"只读"字符串（如 `SDL_GetError()`）**不要 free**，它归 SDL 所有。
- 一句话：**谁分配谁释放；SDL 分配的东西只能用 SDL_free，不能 delete / free；但不是所有 SDL 给的指针都归你。**

## M9（AI 建议出错）"把 const 删掉" → 反而报 C2440

- 出处：我建议 `char* base = SDL_GetBasePath();`，理由是"C++ 不允许 const char* → void*"。
- 你的现象：删掉 const 后，`SDL_GetBasePath` 下面立刻出现红色波浪线：

  ```
  error C2440: 无法从"const char *"转换为"char *"
  IntelliSense 对应 E0144
  ```

- 真相：SDL3 的 `SDL_GetBasePath` **本身返回 `const char *`**（`SDL_filesystem.h:101`）。删 const 是拿更严的类型往更松的里塞，必然失败。原 C2664 的根源**不是"多了个 const"**，而是"**这个指针不该 free**"。
- 正解：保留 const，**删掉 `SDL_free(base);`**（见 M1）。
- 教训：
  1. **编译器只说"两边对不上"，没说该改哪边。** 报类型错误时先列两种以上修法再选。
  2. 遇到 const 报错，先确认 const 是 API 给的还是自己写的。API 给的 const，基本都意味着"只读/所有权"，不是"需要强转"。
  3. 改之前先看红波浪线指哪一端——改调用方还是改类型，代价不一样。

## 更新日志

- 2026-10-24：首次建立，收录 M1–M8。
- 2026-10-24：修正 M1/M2——`SDL_GetBasePath()` 是缓存返回，不能 free；新增 M9（把 const 删掉的错误建议）。
