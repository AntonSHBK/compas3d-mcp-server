#pragma once

namespace kompas_bridge {

/** @brief Точка входа и жизненный цикл процесса bridge. */
class BridgeApplication {
 public:
  /** @brief Запускает bridge в режиме --stdio. */
  int run(
    int argc,
    char* argv[]
  ) const;
};

}  // namespace kompas_bridge
