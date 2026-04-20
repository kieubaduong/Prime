#ifndef RESULT_HPP
#define RESULT_HPP

#include <algorithm>
#include <array>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

template <typename ErrorCodeType = int>
struct Error {
  std::string message{};
  ErrorCodeType error_code{};

  explicit Error(std::string message, ErrorCodeType error_code = ErrorCodeType{}) : message(std::move(message)), error_code(error_code) {
  }
};

template <typename T, typename ErrorCodeType = int>
struct Result {
  using ErrorType = Error<ErrorCodeType>;

  std::variant<T, ErrorType> data;

  [[nodiscard]] bool IsOk() const {
    return std::holds_alternative<T>(data);
  }

  T& GetData() {
    return std::get<T>(data);
  }

  [[nodiscard]] const std::string& GetError() const {
    return std::get<ErrorType>(data).message;
  }

  [[nodiscard]] ErrorCodeType GetErrorCode() const {
    return std::get<ErrorType>(data).error_code;
  }

  static Result Ok(T value) {
    Result result;
    result.data = std::move(value);
    return result;
  }

  static Result Err(std::string error, ErrorCodeType error_code = ErrorCodeType{}) {
    Result result;
    result.data = ErrorType(std::move(error), error_code);
    return result;
  }
};

#endif
