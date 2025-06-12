#pragma once

#include <TopoDS_Shape.hxx>
#include <string>

namespace IntuiCAM {
namespace Geometry {

class IStepLoader {
public:
  virtual ~IStepLoader() = default;
  virtual TopoDS_Shape loadStepFile(const std::string &filename) = 0;
  virtual std::string getLastError() const = 0;
  virtual bool isValid() const = 0;
};

} // namespace Geometry
} // namespace IntuiCAM
