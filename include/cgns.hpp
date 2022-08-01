// Copyright (c) 2022 Pascal Post
// This code is licensed under MIT license (see LICENSE.txt for details)

#pragma once

#include "../include/aux.hpp"
#include <cassert>
#include <cgnslib.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace cgns
{

/// string conversion of given BCType_t
std::string_view
to_string(const BCType_t bc);

/// cgns function call with error handling
template<auto& F, class... Args>
void
cgnsFn(Args&&... args)
{
  if (const int ier = F(args...); ier != CG_OK)
  {
    cg_error_exit();
  }
}

/// represents FamilyBC_t
struct familyBC
{
  std::string name;
  BCType_t bcType;
};

/// represents Family_t
struct family
{
  family(std::string&& name, std::optional<familyBC>&& bc)
    : name{ std::move(name) }
    , bc{ std::move(bc) }
  {
  }

  std::string name;
  std::optional<familyBC> bc;
};

/// represents DataArray_t
template<typename T>
struct dataArray
{
  /// constructor
  dataArray(std::string&& name, std::vector<T>&& data)
    : name{ name }
    , data{ std::move(data) }
  {
  }

  /// name : Data-name identifier or user defined
  std::string name;

  std::vector<T> data;

  DataType_t dataType() const
  {
    if constexpr (std::is_same_v<T, float>)
    {
      return RealSingle;
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      return RealDouble;
    }
    else
    {
      static_assert(always_false<T>::value, "Unknow dataArray data type");
    }
  }
};

/// streaming helper function for dataArray
template<typename T>
std::ostream&
operator<<(std::ostream&, const dataArray<T>&);

using gridCoordinateDataV = std::variant<dataArray<float>, dataArray<double>>;

/// represents GridCoordinates_t
struct gridCoordinatesT
{
  /// constructor
  gridCoordinatesT(std::string&& name,
                   std::vector<gridCoordinateDataV>&& dataArrays)
    : name{ std::move(name) }
    , dataArrays{ std::move(dataArrays) }
  {
  }

  /// name : GridCoordinates or user defined
  std::string name;

  std::vector<gridCoordinateDataV> dataArrays;
};

/// streaming helper function for gridCoordinatesT
std::ostream&
operator<<(std::ostream&, const gridCoordinatesT&);

/// representing Zone_t
struct zone
{
  virtual ~zone() = default;

  /// User defined name
  std::string name;

  /// Label: Zone_t
  static constexpr std::string_view label = "Zone_t";

  std::vector<gridCoordinatesT> gridCoordinates;

protected:
  /// constructor
  zone(std::string&& name, std::vector<gridCoordinatesT>&& gridCoordinates)
    : name(std::move(name))
    , gridCoordinates{ std::move(gridCoordinates) }
  {
  }
};

/// structured Zone_t
struct zoneStructured : zone
{

  /// constructor
  zoneStructured(std::string&& name,
                 std::vector<unsigned>&& nVertex,
                 std::vector<unsigned>&& nCell,
                 std::vector<unsigned>&& nBoundVertex,
                 std::vector<gridCoordinatesT>&& gridCoordinates)
    : zone{ std::move(name), std::move(gridCoordinates) }
    , nVertex{ std::move(nVertex) }
    , nCell{ std::move(nCell) }
    , nBoundVertex{ std::move(nBoundVertex) }
  {
  }

  /// number of vertices in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nVertex;

  /// number of cells in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nCell;

  /// number of boundary vertices in I, J, K (3d) or I, J (2d) direction
  std::vector<unsigned> nBoundVertex;

  static constexpr ZoneType_t zonetype() noexcept { return Structured; }

  /// @brief index dimension for structured zones is the base cell dimension
  unsigned indexDimension() const
  {
    assert(nVertex.size() == nCell.size() &&
           nVertex.size() == nBoundVertex.size());
    return nVertex.size();
  }
};

/// streaming helper function for zoneStructured
std::ostream&
operator<<(std::ostream&, const zoneStructured&);

/// unstructured Zone_t
struct zoneUnstructured : zone
{

  /// constructor
  zoneUnstructured(std::string&& name,
                   const unsigned nVertex,
                   const unsigned nCell,
                   const unsigned nBoundVertex,
                   std::vector<gridCoordinatesT>&& gridCoordinates)
    : zone{ std::move(name), std::move(gridCoordinates) }
    , nVertex{ nVertex }
    , nCell{ nCell }
    , nBoundVertex{ nBoundVertex }
  {
  }

  unsigned nVertex;
  unsigned nCell;
  unsigned nBoundVertex;

  static constexpr ZoneType_t zonetype() noexcept { return Unstructured; }

  /// index dimension for unstructured zone is always 1
  static constexpr unsigned indexDimension = 1;
};

/// streaming helper function for zoneUnstructured
std::ostream&
operator<<(std::ostream&, const zoneUnstructured&);

using zoneV = std::variant<zoneStructured, zoneUnstructured>;

// representing CGNSBase_t
struct base
{

  /// constructor
  base(std::string&& name,
       const unsigned cellDimension,
       const unsigned physicalDimension,
       std::vector<zoneV>&& zones = {},
       std::vector<family>&& families = {})
    : name{ std::move(name) }
    , cellDimension{ cellDimension }
    , physicalDimension{ physicalDimension }
    , zones{ std::move(zones) }
    , families{ std::move(families) }
  {
  }

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

  std::vector<family> families;
};

/// streaming helper function for base
std::ostream&
operator<<(std::ostream&, const base&);

/// root of a cgns mesh
struct root
{
  std::vector<base> bases;
};

/// enum mapping for file modes
enum class fileMode
{
  read = CG_MODE_READ,
  write = CG_MODE_WRITE,
  modify = CG_MODE_MODIFY
};

/// cgns file (pure virtual base class)
struct file
{

  /// destructor closing the file
  virtual ~file();

protected:
  /// constructor
  file(const std::string& path, fileMode);

  /// cgns file handle
  int _handle;
};

/// cgns read file
struct fileIn : file
{

  /// construct a new file based on the path
  fileIn(const std::string& path);

  /// read base information
  std::vector<base> readBaseInformation() const;

  /// read base information
  std::vector<zoneV> readZoneInformation(const int B) const;

  /// read Zone Grids Coordinates
  /// nVertex.size() = 1 : Unstructured
  /// nVertex.size() = 2 : 2D Structured
  /// nVertex.size() = 3 : 3D Structured
  std::vector<gridCoordinatesT> readZoneGridCoordinates(
    const int B,
    const int Z,
    const std::vector<unsigned>& nVertex) const;

  /// read Family Definition
  std::vector<family> readFamilyDefinition(const int B) const;

  /// read Family Boundary Condition
  familyBC readFamilyBoundaryCondition(const int B, const int Fam) const;
};

/// cgns read file
struct fileOut : file
{

  /// construct a new file based on the path
  fileOut(const std::string& path);

  /// write base information of root to file
  void writeBaseInformation(root) const;

  /// write base information
  void writeZoneInformation(const int B, const zoneV&) const;

  /// write zone grid coordinates
  void writeZoneGridCoordinates(const int B,
                                const int Z,
                                const gridCoordinatesT&) const;

  void writeDataArray() const;

  /// write zone grid coordinate data
  void writeZoneGridCoordinateData(const int B,
                                   const int Z,
                                   const gridCoordinateDataV& data) const;

  /// write family definition including the optional BC
  void writeFamilyDefinition(const int B, const family& family) const;
};

/// parse file and return a root to the cgns hirarchy
root
parse(const std::string& path);

/// write cgns hirachy to the give file path
void
writeFile(const std::string& path, root);

} // namespace cgns