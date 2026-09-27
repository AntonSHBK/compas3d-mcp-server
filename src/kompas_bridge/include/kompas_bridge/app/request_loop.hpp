#pragma once

namespace kompas_bridge {
class Transport;
class RequestDispatcher;

/** @brief Обслуживает запросы последовательно до EOF. */
void run_request_loop(
  Transport& transport,
  const RequestDispatcher& dispatcher
);

}  // namespace kompas_bridge
