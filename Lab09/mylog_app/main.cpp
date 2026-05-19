#include <mylog/mylog.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>

int main() {
  const char* log_path = std::getenv("LOG_PATH");

  if (log_path != nullptr) {
    std::ofstream file{log_path, std::ios_base::app};
    mylog::Logger logger(file);
    logger.info("application started");
    logger.warn("disk space is low");
    logger.error("network unreachable");
    return 0;
  }

  mylog::Logger logger(std::cout);
  logger.info("application started");
  logger.warn("disk space is low");
  logger.error("network unreachable");
  return 0;
}
