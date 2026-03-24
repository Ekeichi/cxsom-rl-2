#include <cxsomData.hpp>
#include <cxsomVariable.hpp>
#include <filesystem>
#include <iostream>

int main() {
  cxsom::symbol::Variable var{"in", "error"};
  cxsom::data::File file(std::filesystem::path("root-dir"), var);

  file.realize(cxsom::type::make("Scalar"), 2, 100, true);

  //   auto d = cxsom::data::make(cxsom::type::make("Scalar"));
  //   *std::static_pointer_cast<cxsom::data::Scalar>(d) = 3.14;

  //   file.write(0, d);

  auto r_first = cxsom::data::make(cxsom::type::make("Scalar"));
  auto r_last = cxsom::data::make(cxsom::type::make("Scalar"));

  auto [first, last] = file.get_time_range();

  if (first == cxsom::data::File::no_time()) {
    std::cout << "File is empty — no data written yet." << std::endl;
    return 0;
  }

  std::cout << "Time range: [" << first << ", " << last << "]" << std::endl;

  auto status_first = file.read(first, r_first);
  auto status_last = file.read(last, r_last);

  std::cout << "status_first = " << status_first << std::endl;
  std::cout << "status_last  = " << status_last << std::endl;

  if (status_first == cxsom::data::FileAvailability::Ready)
    std::cout << "first value = " << *r_first << std::endl;

  if (status_last == cxsom::data::FileAvailability::Ready)
    std::cout << "last value  = " << *r_last << std::endl;

  return 0;
}
