// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include "../include/aux.hpp"
#include <cassert>
#include <cgnslib.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace cgns {

/// cgns function call with error handling
template <auto &F, class... Args> void cgnsFn(Args &&...args) {
  if (const int ier = F(args...); ier != CG_OK) {
    cg_error_exit();
  }
}

/// represents DataArray_t
template <typename T> struct dataArray {

  /// constructor
  dataArray(std::string &&name, std::vector<T> &&data)
      : name{name}, data{std::move(data)} {}

  /// name : Data-name identifier or user defined
  std::string name;

  std::vector<T> data;

  DataType_t dataType() const {
    if constexpr (std::is_same_v<T, float>) {
      return RealSingle;
    } else if constexpr (std::is_same_v<T, double>) {
      return RealDouble;
    } else {
      static_assert(always_false<T>::value, "Unknow dataArray data type");
    }
  }
};

using gridCoordinateDataV = std::variant<dataArray<float>, dataArray<double>>;

/// represents GridCoordinates_t
struct gridCoordinatesT {

  /// constructor
  gridCoordinatesT(std::string &&name, std::vector<gridCoordinateDataV> &&data)
      : name{std::move(name)}, data{std::move(data)} {}

  /// name : GridCoordinates or user defined
  std::string name;

  std::vector<gridCoordinateDataV> data;
};

/// representing Zone_t
struct zone {
  virtual ~zone() = default;

  /// User defined name
  std::string name;

  /// Label: Zone_t
  static constexpr std::string_view label = "Zone_t";

  std::vector<gridCoordinatesT> gridCoordinates;

protected:
  /// constructor
  zone(std::string &&name, std::vector<gridCoordinatesT> &&gridCoordinates)
      : name(std::move(name)), gridCoordinates{std::move(gridCoordinates)} {}
};

/// structured Zone_t
struct zoneStructured : zone {

  /// constructor
  zoneStructured(std::string &&name, std::vector<unsigned> &&nVertex,
                 std::vector<unsigned> &&nCell,
                 std::vector<unsigned> &&nBoundVertex,
                 std::vector<gridCoordinatesT> &&gridCoordinates)
      : zone{std::move(name), std::move(gridCoordinates)}, nVertex{std::move(
                                                               nVertex)},
        nCell{std::move(nCell)}, nBoundVertex{std::move(nBoundVertex)} {}

  /// number of vertices in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nVertex;

  /// number of cells in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nCell;

  /// number of boundary vertices in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nBoundVertex;

private:
  ZoneType_t zonetype() const { return Structured; }

  /// @brief index dimension for structured zones is the base cell dimension
  unsigned indexDimension() const {
    assert(nVertex.size() == nCell.size() &&
           nVertex.size() == nBoundVertex.size());
    return nVertex.size();
  }
};

/// unstructured Zone_t
struct zoneUnstructured : zone {

  /// constructor
  zoneUnstructured(std::string &&name, const unsigned nVertex,
                   const unsigned nCell, const unsigned nBoundVertex,
                   std::vector<gridCoordinatesT> &&gridCoordinates)
      : zone{std::move(name), std::move(gridCoordinates)}, nVertex{nVertex},
        nCell{nCell}, nBoundVertex{nBoundVertex} {}

  unsigned nVertex;
  unsigned nCell;
  unsigned nBoundVertex;

  ZoneType_t zonetype() const { return Unstructured; }

  /// @brief index dimension for unstructured zone is always 1
  unsigned indexDimension = 1;
};

using zoneV = std::variant<zoneStructured, zoneUnstructured>;

// representing CGNSBase_t
struct base {

  /// constructor
  base(std::string &&name, const unsigned cellDimension,
       const unsigned physicalDimension, std::vector<zoneV> &&zones = {})
      : name{std::move(name)}, cellDimension{cellDimension},
        physicalDimension{physicalDimension}, zones{std::move(zones)} {}

  /// User defined name
  std::string name;

  static constexpr std::string_view label = "CGNSBase_t";

  /// @brief dimensionality of a cell in the mesh (i.e., 3 for a volume cell and
  /// 2 for a face cell)
  unsigned cellDimension;

  /// @brief number of indices required to specify a unique physical location in
  /// the field data being recorded
  unsigned physicalDimension;

  std::vector<zoneV> zones;
};

/// streaming helper function for base
std::ostream &operator<<(std::ostream &, const base &);

/// root of a cgns mesh
struct root {
  std::vector<base> bases;
};

/// enum mapping for file modes
enum class fileMode {
  read = CG_MODE_READ,
  write = CG_MODE_WRITE,
  modify = CG_MODE_MODIFY
};

/// cgns file (pure virtual base class)
struct file {

  /// destructor closing the file
  virtual ~file();

protected:
  /// constructor
  file(const std::string &path, fileMode);

  /// cgns file handle
  int _handle;
};

/// cgns read file
struct fileIn : file {

  /// construct a new file based on the path
  fileIn(const std::string &path);

  /// read base information
  std::vector<base> readBaseInformation() const;

  /// read base information
  std::vector<zoneV> readZoneInformation(const int B) const;

  /// read Zone Grids Coordinates
  /// nVertex.size() = 1 : Unstructured
  /// nVertex.size() = 2 : 2D Structured
  /// nVertex.size() = 3 : 3D Structured
  std::vector<gridCoordinatesT>
  readZoneGridCoordinates(const int B, const int Z,
                          std::vector<unsigned> nVertex) const;
};

/// cgns read file
struct fileOut : file {

  /// construct a new file based on the path
  fileOut(const std::string &path);

  /// write base information of root to file
  void writeBaseInformation(root) const;
};

/// parse file and return a root to the cgns hirarchy
root parse(const std::string &path);

/// write cgns hirachy to the give file path
void writeFile(const std::string &path, root);

} // namespace cgns