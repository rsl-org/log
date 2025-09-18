#include <rsl/logging/args.hpp>
#include <rsl/log>

// #include <print>
void foo(int x, bool b) {
  auto args = capture_args();
  // std::println("args: {}", args);
}

auto bar(int x, long f, short s, int i, unsigned u, char c, unsigned char uc){
  // dangerous - might dangle if args are ref qualified
  return capture_args();
}

int main() {
  // foo(42, false);
  // bar(1,2,3,4,5,6,7);
  rsl::info("test {}", 3);

}