#ifndef _rip_col_
#define _rip_col_

#define WARN(format, ...)                                                      \
  std::println("\033[1;33m[WARN] " format "\033[0m" __VA_OPT__(, ) __VA_ARGS__);

#define SUCCESS(format, ...)                                                   \
  std::println("\033[1;32m[SUCCESS] " format "\033[0m" __VA_OPT__(, )          \
                   __VA_ARGS__)

#define ERROR(format, ...)                                                     \
  std::println("\033[1;31m[ERROR] " format "\033[0m" __VA_OPT__(, ) __VA_ARGS__)

#define INFO(format, ...)                                                      \
  std::println("[INFO] " format "\033[0m" __VA_OPT__(, ) __VA_ARGS__)

#define OK(format, ...)                                                        \
  std::println("[OK] " format "\033[0m" __VA_OPT__(, ) __VA_ARGS__)

#define GENERAL(format, ...) std::println(format __VA_OPT__(, ) __VA_ARGS__)

#endif // !_rip_col_
