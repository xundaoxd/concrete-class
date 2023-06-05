#include <iostream>

class Base {
public:
  void virtual print() = 0;
};

class Base1 {
public:
  virtual void print1() = 0;
};

class Demo : public Base, public Base1 {};

int main(int argc, char *argv[]) {
  std::cout << "hello" << std::endl;
  return 0;
}
